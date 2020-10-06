#pragma once

#include <RenderPassBase.h>

namespace dx12demo::core
{
	class CommandList;
	class FrustumCullRenderPass : public RenderPassBase
	{
	public:

		FrustumCullRenderPass();

		virtual ~FrustumCullRenderPass();

		virtual void LoadContent(RenderPassBaseInfo*) override;

		virtual void OnUpdate(CommandList&, UpdateEventArgs& e) override;

		virtual void OnRender(CommandList&, RenderEventArgs& e) override;
	};
}