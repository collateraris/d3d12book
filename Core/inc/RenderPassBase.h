#pragma once

#include <Events.h>
#include <URootObject.h>
#include <memory>

namespace dx12demo::core
{
	struct RenderPassBaseInfo
	{
		RenderPassBaseInfo() = default;

		virtual ~RenderPassBaseInfo() = default;
	};

	class CommandList;

	class RenderPassBase : public URootObject
	{
	public:

		RenderPassBase();

		virtual ~RenderPassBase();

		virtual void LoadContent(RenderPassBaseInfo*) = 0;

		virtual void OnUpdate(std::shared_ptr<CommandList>& commandList, UpdateEventArgs& e) = 0;

		virtual void OnPreRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) {};

		virtual void OnRender(std::shared_ptr<CommandList>& commandList, RenderEventArgs& e) = 0;
	};
}