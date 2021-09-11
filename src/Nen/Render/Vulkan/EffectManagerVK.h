#pragma once
#include "../../Effect/EffectManager.h"
#include <EffekseerRendererVulkan.h>
namespace nen::vk
{
	class EffectManagerVK : public EffectManager
	{
	public:
		EffectManagerVK(class VKRenderer *renderer);
		~EffectManagerVK() override;
		void Init() override;
		void Shutdown() override;
		Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> GetMemoryPool() { return sfMemoryPool; }
		Effekseer::RefPtr<EffekseerRenderer::CommandList> GetCommandList() { return commandList; }

	private:
		class VKRenderer *mRenderer;
		Effekseer::RefPtr<EffekseerRenderer::SingleFrameMemoryPool> sfMemoryPool;
		Effekseer::RefPtr<EffekseerRenderer::CommandList> commandList;
	};
}