#pragma once

#include <Events.h>
#include <URootObject.h>

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

		virtual void OnUpdate(CommandList& commandList, UpdateEventArgs& e) = 0;

		virtual void OnRender(CommandList& commandList, RenderEventArgs& e) = 0;
	};
}