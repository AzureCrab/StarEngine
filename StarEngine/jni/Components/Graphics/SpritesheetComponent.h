#pragma once

#include "SpriteComponent.h"
#include <list>
#include "../../Helpers/SpriteAnimation.h"
#include "../../Helpers/Spritesheet.h"

namespace star
{
	class SpritesheetComponent final : public SpriteComponent
	{
	public:
		SpritesheetComponent(const tstring& filePath, const tstring& spriteName, const tstring & spritesheet, bool isHUD = false);
		~SpritesheetComponent();

		void Update(const Context& context);

		void Play();
		void Restart();
		void Pause();
		void Stop();

		void PushAnimation(const tstring & animation);
		void PlayAnimation(const tstring & animation);
		void PlayAnimation(const tstring & animation, int32 startFrame);
		void PlayNextAnimation();
		void PlayNextAnimation(int32 startFrame);
		void Reset();

		void SetSpritesheet(const tstring & spritesheet);

		tstring GetCurrentAnimation() const;
		const tstring & GetcurrentSpritesheet() const;

		int32 GetCurrentFrame() const;

		int32 GetFramesHorizontal() const;
		int32 GetFramesVertical() const;

		void SetCallbackAnimations(const std::function<void()> & callback);

		bool IsPlaying();

	protected:
		void InitializeComponent();
		void CreateVertices();
		tstring m_SpritesheetName;
		Spritesheet m_Spritesheet;

		std::list<SpriteAnimation> m_Animations;

	private:
		SpritesheetComponent(const SpritesheetComponent &);
		SpritesheetComponent(SpritesheetComponent &&);
		SpritesheetComponent& operator=(const SpritesheetComponent &);
		SpritesheetComponent& operator=(SpritesheetComponent &&);
	};
}
