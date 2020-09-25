#include <Material.h>

#include <CommandList.h>

using namespace dx12demo::core;

Material::Material()
{

}

Material::~Material()
{

}

void Material::LoadDiffuseTex(std::shared_ptr<CommandList>& commandList, const std::wstring& filePath)
{
	commandList->LoadTextureFromFile(m_diffuse, filePath, TextureUsage::Diffuse);
}

void Material::LoadAmbientTex(std::shared_ptr<CommandList>& commandList, const std::wstring& filePath)
{
	commandList->LoadTextureFromFile(m_ambient, filePath, TextureUsage::Ambient);
}

void Material::LoadSpecularTex(std::shared_ptr<CommandList>& commandList, const std::wstring& filePath)
{
	commandList->LoadTextureFromFile(m_specular, filePath, TextureUsage::Specular);
}

void Material::LoadNormalTex(std::shared_ptr<CommandList>& commandList, const std::wstring& filePath)
{
	commandList->LoadTextureFromFile(m_normal, filePath, TextureUsage::Normalmap);
}

const Texture& Material::GetDiffuseTex() const
{
	return m_diffuse;
}

const Texture& Material::GetAmbientTex() const
{
	return m_ambient;
}

const Texture& Material::GetSpecularTex() const
{
	return m_specular;
}

const Texture& Material::GetNormalTex() const
{
	return m_normal;
}