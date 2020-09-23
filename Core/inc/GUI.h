#pragma once

#include <URootObject.h>
#include "imgui.h"

#include <d3dx12.h>
#include <wrl.h>
#include <memory>

namespace dx12demo::core
{
	class CommandList;
	class Texture;
	class RenderTarget;
	class RootSignature;
	class Window;

	class GUI : public URootObject
	{
	public:
		GUI();
		virtual ~GUI();

		virtual bool Initialize(std::shared_ptr<Window> window);

		// Begin a new frame.
		virtual void NewFrame();
		virtual void Render(std::shared_ptr<CommandList> commandList, const RenderTarget& renderTarget);

		virtual void Destroy();
		void SetScaling(float scale);

	private:

		ImGuiContext* m_pImGuiCtx;
		std::unique_ptr<Texture> m_FontTexture;
		std::unique_ptr<RootSignature> m_RootSignature;
		Microsoft::WRL::ComPtr<ID3D12PipelineState> m_PipelineState;
		std::shared_ptr<Window> m_Window;
	};
}