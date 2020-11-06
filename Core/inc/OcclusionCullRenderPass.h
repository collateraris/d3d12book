#pragma once

#include <RenderPassBase.h>

namespace dx12demo::core
{
	class CommandList;
	class OcclusionCullRenderPass : public RenderPassBase
	{
	public:

		OcclusionCullRenderPass();

		virtual ~OcclusionCullRenderPass();

		virtual void LoadContent(RenderPassBaseInfo*) override;

		virtual void OnUpdate(std::shared_ptr<CommandList>&,UpdateEventArgs& e) override;

		virtual void OnRender(std::shared_ptr<CommandList>&, RenderEventArgs& e) override;
	};
}