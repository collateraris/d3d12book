#pragma once

enum class TextureUsage
{
    None,
    Albedo,
    Diffuse = Albedo,       // Treat Diffuse and Albedo textures the same.
    Heightmap,
    Depth = Heightmap,      // Treat height and depth textures the same.
    Normalmap,
    RenderTarget,           // Texture is used as a render target.
    Specular,
};
