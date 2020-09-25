#pragma once

#include <URootObject.h>
#include <Texture.h>

namespace dx12demo::core
{
	class CommandList;
	class Material : public URootObject
	{
	public:

		Material();
		virtual ~Material();

		void LoadDiffuseTex(std::shared_ptr<CommandList>& commandList, const std::wstring& filePath);
		void LoadAmbientTex(std::shared_ptr<CommandList>& commandList, const std::wstring& filePath);
		void LoadSpecularTex(std::shared_ptr<CommandList>& commandList, const std::wstring& filePath);
		void LoadNormalTex(std::shared_ptr<CommandList>& commandList, const std::wstring& filePath);

		const Texture& GetDiffuseTex() const;
		const Texture& GetAmbientTex() const;
		const Texture& GetSpecularTex() const;
		const Texture& GetNormalTex() const;

	private:

		Texture m_diffuse;
		Texture m_ambient;
		Texture m_specular;
		Texture m_normal;
	};
}