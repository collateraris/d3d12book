#include <Material.h>

#include <CommandList.h>

using namespace dx12demo::core;

Material::Material()
{

}

Material::~Material()
{

}

void Material::LoadDiffuseTex(std::shared_ptr<CommandList>& commandList, const std::wstring& fileName)
{
	commandList->LoadTextureFromFile(m_diffuse, fileName, TextureUsage::Diffuse);
}

void Material::LoadSpecularTex(std::shared_ptr<CommandList>& commandList, const std::wstring& fileName)
{
	commandList->LoadTextureFromFile(m_specular, fileName, TextureUsage::Specular);
}

void Material::LoadNormalTex(std::shared_ptr<CommandList>& commandList, const std::wstring& fileName)
{
	commandList->LoadTextureFromFile(m_normal, fileName, TextureUsage::Normalmap);
}

const Texture& Material::GetDiffuseTex() const
{
	return m_diffuse;
}

const Texture& Material::GetSpecularTex() const
{
	return m_specular;
}

const Texture& Material::GetNormalTex() const
{
	return m_normal;
}