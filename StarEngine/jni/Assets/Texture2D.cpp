#include "Texture2D.h"

namespace star
{
	//[NOTE]	You're not supposed to make Textures yourself.
	//			Use the TextureManager to load your textures.
	//			This ensures a same texture is not loaded multiple times
#ifdef DESKTOP
	Texture2D::Texture2D(const tstring & pPath):
			mPath(pPath),
			mTextureId(0),
			mFormat(0),
			mWidth(0),
			mHeight(0)
	{
		this->Load();
	}
#else
	Texture2D::Texture2D(const tstring & pPath, android_app* pApplication):
			mResource(pApplication , pPath),
			mPath(pPath),
			mTextureId(0),
			mFormat(0),
			mWidth(0),
			mHeight(0)
	{
		this->Load();
	}

	void Texture2D::CallbackRead(png_structp png, png_bytep data, png_size_t size)
	{
		Resource& lReader = *((Resource*)png_get_io_ptr(png));
		if(!lReader.Read(data,size))
		{
			lReader.Close();
			png_error(png, "Error while reading PNG file");
		}
	}
#endif

	Texture2D::~Texture2D()
	{
		if(mTextureId != 0)
		{
			glDeleteTextures(1, &mTextureId);
			mTextureId = 0;
		}
		mWidth = 0;
		mHeight = 0;
		mFormat = 0;
	}
	
	uint8* Texture2D::ReadPNG()
	{
		png_byte header[8];
		png_structp lPngPtr = NULL;
		png_infop lInfoPtr = NULL;
		png_byte* lImageBuffer = NULL;
		png_bytep* lRowPtrs = NULL;
		png_int_32 lRowSize;
		bool lTransparency;

#ifdef DESKTOP
		FILE *fp;
		tfopen(&fp, mPath.c_str(), _T("rb"));

		if(fp == NULL)
		{ 
			Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : png could not be loaded"));
			return NULL;
		}

		fread(header, 8, 1, fp);
#else
		if(!mResource.Open())
		{
			mResource.Close();
			Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : Could Not Open Resource"));
			return NULL;
		}
		if(!mResource.Read(header, sizeof(header)))
		{
			mResource.Close();
			Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : Could Not Read"));
			return NULL;
		}
#endif

		if(png_sig_cmp(header, 0, 8))
		{
			Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : Not a PNG file"));
			return NULL;
		}

		lPngPtr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
		if(!lPngPtr)
		{
			Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : create struct string failed"));
			return NULL;
		}

		lInfoPtr = png_create_info_struct(lPngPtr);
		if(!lInfoPtr)
		{
			Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : create info failed"));
			return NULL;
		}

#ifdef DESKTOP
		if(setjmp(png_jmpbuf(lPngPtr)))
		{
			Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : Error during init io"));
			return NULL;
		}

		png_init_io(lPngPtr, fp);
#else
		png_set_read_fn(lPngPtr, &mResource, CallbackRead);
		if(setjmp(png_jmpbuf(lPngPtr)))
		{
			mResource.Close();
			Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : Error during init io"));
			return NULL;
		}
#endif
		png_set_sig_bytes(lPngPtr, 8);
		png_read_info(lPngPtr,lInfoPtr);

		png_uint_32 pWidth, pHeight;
		png_int_32 lDepth, lColorType;
		png_get_IHDR(lPngPtr,lInfoPtr,&pWidth,&pHeight,&lDepth,&lColorType, NULL,NULL,NULL);
		mWidth = pWidth;
		mHeight = pHeight;

		lTransparency = false;
		if(png_get_valid(lPngPtr, lInfoPtr, PNG_INFO_tRNS))
		{
			png_set_tRNS_to_alpha(lPngPtr);
			lTransparency=true;

#ifdef ANDROID
			mResource.Close();
#endif
			delete [] lRowPtrs;
			delete [] lImageBuffer;
			if(lPngPtr != NULL)
			{
				png_infop* lInfoPtrP = lInfoPtr != NULL ? &lInfoPtr: NULL;
				png_destroy_read_struct(&lPngPtr, lInfoPtrP, NULL);
			}
			return NULL;
		}

		if(lDepth < 8 )
		{
			png_set_packing(lPngPtr);
		}
		else if(lDepth == 16)
		{
			png_set_strip_16(lPngPtr);
		}

		switch(lColorType)
		{
		case PNG_COLOR_TYPE_PALETTE:
			png_set_palette_to_rgb(lPngPtr);
			mFormat = lTransparency ? GL_RGBA : GL_RGB;
			break;
		case PNG_COLOR_TYPE_RGB:
			mFormat = lTransparency ? GL_RGBA : GL_RGB;
			break;
		case PNG_COLOR_TYPE_RGBA:
			mFormat = GL_RGBA;
			break;
		case PNG_COLOR_TYPE_GRAY:
			png_set_expand_gray_1_2_4_to_8(lPngPtr);
			mFormat = lTransparency ? GL_LUMINANCE_ALPHA : GL_LUMINANCE;
			break;
		case PNG_COLOR_TYPE_GA:
			png_set_expand_gray_1_2_4_to_8(lPngPtr);
			mFormat = GL_LUMINANCE_ALPHA;
			break;
		}

		//mNumber_of_passes = png_set_interlace_handling(mPng_ptr);
		png_read_update_info(lPngPtr,lInfoPtr);

		/*if(setjmp(png_jmpbuf(mPng_ptr)))
		{
			Logger::GetInstance()->Log(LogLevel::Info,_T("PNG : Error during read image"));
			return NULL;
		}*/

		lRowSize = png_get_rowbytes(lPngPtr,lInfoPtr);
		if(lRowSize <= 0)
		{
			Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : png rowsize smaller or equal to 0"));
			return NULL;
		}

		lImageBuffer = new png_byte[lRowSize * pHeight];
		if(!lImageBuffer)
		{
			Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : Error during image buffer creation"));
			return NULL;
		}

		lRowPtrs = new png_bytep[pHeight];
		if(!lRowPtrs)
		{
			Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : Error during row pointer creation"));
			return NULL;
		}

		for(uint32 i = 0; i < pHeight; ++i)
		{
			lRowPtrs[pHeight - (i+1)] = lImageBuffer + i * lRowSize;
		}
		png_read_image(lPngPtr, lRowPtrs);

#ifdef DESKTOP
		fclose(fp);
#else
		mResource.Close();
#endif
		png_destroy_read_struct(&lPngPtr, &lInfoPtr, NULL);
		delete[] lRowPtrs;

#ifdef _DEBUG
		Logger::GetInstance()->Log(LogLevel::Info, _T("PNG : ") + mPath + _T(" Created Succesfull"));
#endif
		return lImageBuffer;

	}

	bool Texture2D::Load()
	{
		uint8* lImageBuffer = this->ReadPNG();
		if(lImageBuffer == NULL)
		{
			Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : READING PNG FAILED - NO IMAGE BUFFER"));
			return false;
		}

		glGenTextures(1, &mTextureId);
		glBindTexture(GL_TEXTURE_2D, mTextureId);

		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glTexImage2D(GL_TEXTURE_2D, 0, mFormat, mWidth, mHeight, 0, mFormat, GL_UNSIGNED_BYTE, lImageBuffer);
		delete[] lImageBuffer;

		bool hasError=false;
		GLenum errormsg;
		errormsg = glGetError();
		while(errormsg != GL_NO_ERROR)
		{
			hasError=true;
			switch(errormsg)
			{
			case GL_INVALID_ENUM:
				Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : Unacceptable value for imagebuffer"));
				break;
			case GL_INVALID_VALUE:
				Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : value out of range"));
				break;
			case GL_INVALID_OPERATION:
				Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : Not allowed in current state"));
				break;
			case GL_OUT_OF_MEMORY:
				Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : Out of Memory"));
				break;
			}
			errormsg = glGetError();
		}

		if(hasError)
		{
			Logger::GetInstance()->Log(LogLevel::Error, _T("PNG : Error loading pnginto OpenGl"));
			if(mTextureId != 0)
			{
				glDeleteTextures(1, &mTextureId);
				mTextureId = 0;
			}
			mWidth = 0;
			mHeight = 0;
			mFormat = 0;
			return false;
		}
		return true;
	}

	const tstring & Texture2D::GetPath() const
	{
#ifdef DESKTOP
		return mPath;
#else
		return mResource.GetPath();
#endif
	}

	int32 Texture2D::GetHeight() const
	{
		return mHeight;
	}

	int32 Texture2D::GetWidth() const
	{
		return mWidth;
	}

	GLuint Texture2D::GetTextureID() const
	{
		return mTextureId;
	}
}
