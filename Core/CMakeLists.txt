cmake_minimum_required( VERSION 3.18.3 ) # Latest version of CMake when this file was created.

set( HEADER_FILES 
    inc/DX12LibPCH.h
    inc/Application.h
	inc/AtmosphericScatteringSkyDoom.h
	inc/BitmapClouds.h
	inc/BoundingVolumesPrimitive.h
    inc/Buffer.h
    inc/ByteAddressBuffer.h
	inc/Camera.h
    inc/CommandList.h
    inc/CommandQueue.h
    inc/ConstantBuffer.h
	inc/d3dUtil.h
	inc/DebugDepthBufferRenderPass.h
	inc/DepthBufferRenderPass.h
    inc/DescriptorAllocation.h
    inc/DescriptorAllocator.h
    inc/DescriptorAllocatorPage.h
    inc/DynamicDescriptorHeap.h
	inc/EnvironmentMapRenderPass.h
	inc/Frustum.h
	inc/FrustumCullRenderPass.h
    inc/Events.h
	inc/ForwardPlusRenderPass.h
    inc/Game.h
    inc/GenerateMipsPSO.h
	inc/GeometryPrimitive.h
	inc/GridViewFrustums.h
    inc/GUI.h
    inc/Helpers.h
    inc/HighResolutionClock.h
    inc/IndexBuffer.h
    inc/KeyCodes.h
	inc/Light.h
	inc/LightCulling.h
	inc/LightsToView.h
	inc/Material.h
    inc/Mesh.h
	inc/OcclusionCullRenderPass.h
    inc/PanoToCubemapPSO.h
	inc/PerturbedClouds.h
	inc/QuadRenderPass.h
	inc/QuadTree.h
	inc/RenderPassBase.h
    inc/RenderTarget.h
    inc/Resource.h
    inc/ResourceStateTracker.h
    inc/RootSignature.h
    inc/Scene.h
    inc/SceneNode.h
	inc/ShaderCommonInclude.h
	inc/SkyDoom.h
    inc/StructuredBuffer.h
	inc/Terrain.h
    inc/Texture.h
    inc/TextureUsage.h
    inc/ThreadSafeQueue.h
    inc/UploadBuffer.h
	inc/URootObject.h
    inc/VertexBuffer.h
	inc/VolumetricClouds.h
	inc/VoxelGrid.h
	inc/VoxelGridDebugRenderPass.h
    inc/Window.h
    inc/d3dx12.h
)

set( SOURCE_FILES
    src/Application.cpp
	src/AtmosphericScatteringSkyDoom.cpp
	src/BitmapClouds.cpp
    src/Buffer.cpp
    src/ByteAddressBuffer.cpp
	src/Camera.cpp
    src/CommandQueue.cpp
    src/CommandList.cpp
    src/ConstantBuffer.cpp
	src/d3dUtil.cpp
	src/DebugDepthBufferRenderPass.cpp
	src/DepthBufferRenderPass.cpp
    src/DescriptorAllocation.cpp
    src/DescriptorAllocator.cpp
    src/DescriptorAllocatorPage.cpp
    src/DynamicDescriptorHeap.cpp
	src/EnvironmentMapRenderPass.cpp
	src/ForwardPlusRenderPass.cpp
	src/Frustum.cpp
	src/FrustumCullRenderPass.cpp
	src/QuadTree.cpp
    src/Game.cpp
    src/GenerateMipsPSO.cpp
	src/GridViewFrustums.cpp
    src/GUI.cpp
    src/HighResolutionClock.cpp
    src/IndexBuffer.cpp
	src/LightCulling.cpp
	src/LightsToView.cpp
	src/Material.cpp
    src/Mesh.cpp
	src/OcclusionCullRenderPass.cpp
    src/PanoToCubemapPSO.cpp
	src/PerturbedClouds.cpp
	src/QuadRenderPass.cpp
	src/RenderPassBase.cpp
    src/RenderTarget.cpp
    src/Resource.cpp
    src/ResourceStateTracker.cpp
    src/RootSignature.cpp
	src/Scene.cpp
    src/SceneNode.cpp
	src/SkyDoom.cpp
    src/StructuredBuffer.cpp
	src/Terrain.cpp
    src/Texture.cpp
    src/UploadBuffer.cpp
	src/URootObject.cpp
    src/VertexBuffer.cpp
	src/VolumetricClouds.cpp
	src/VoxelGrid.cpp
	src/VoxelGridDebugRenderPass.cpp
    src/Window.cpp
)

set( IMGUI_HEADERS
    inc/imgui.h
    inc/imconfig.h
    inc/imgui_impl_win32.h
)

source_group( "Header Files\\ImGUI" FILES ${IMGUI_HEADERS} )

set( IMGUI_SOURCE
    src/imgui.cpp
    src/imgui_demo.cpp
    src/imgui_draw.cpp
    src/imgui_widgets.cpp
    src/imgui_impl_win32.cpp
)

source_group( "Source Files\\ImGUI" FILES ${IMGUI_SOURCE} )

set( CONFIG_SYS_HEADERS
    inc/config_sys/Config.h
    inc/config_sys/XPath.h 
	inc/config_sys/tinyxml2.h
)

source_group( "Header Files\\ConfigSys" FILES ${CONFIG_SYS_HEADERS} )

set( CONFIG_SYS_SOURCE
    src/config_sys/Config.cpp
    src/config_sys/XPath.cpp
	src/config_sys/tinyxml2.cpp
)

source_group( "Source Files\\ConfigSys" FILES ${CONFIG_SYS_SOURCE} )

set( STB_IMAGE_HEADERS
    inc/stb_image/stb_image.h
    inc/stb_image/stb_image_write.h 
)

source_group( "Header Files\\STB_Image" FILES ${STB_IMAGE_HEADERS} )

set( SHADER_FILES
    shaders/GenerateMips_CS.hlsl
	shaders/PanoToCubemap_CS.hlsl
    shaders/ImGUI_VS.hlsl
    shaders/ImGUI_PS.hlsl
	shaders/Skybox_VS.hlsl
    shaders/Skybox_PS.hlsl
	shaders/SkyDoomGradient_PS.hlsl
	shaders/GridViewFrustums_CS.hlsl
	shaders/LightCulling_CS.hlsl
	shaders/LightsToView_CS.hlsl
	shaders/DepthBuffer_PS.hlsl
	shaders/DepthBuffer_VS.hlsl
	shaders/DebugDepthBuffer_PS.hlsl
	shaders/DebugDepthBuffer_VS.hlsl
	shaders/Quad_PS.hlsl
	shaders/Quad_VS.hlsl
	shaders/QuadTC_VS.hlsl
	shaders/ForwardPlus_PS.hlsl
	shaders/ForwardPlus_VS.hlsl
	shaders/VoxelGridFill_VS.hlsl
	shaders/VoxelGridFill_GS.hlsl
	shaders/VoxelGridFill_PS.hlsl
	shaders/VoxelGridDebug_PS.hlsl
	shaders/BitmapClouds_PS.hlsl
	shaders/BitmapClouds_VS.hlsl
	shaders/PerturbedClouds_PS.hlsl
	shaders/AtmosphericScatteringSkyDoom_VS.hlsl
	shaders/AtmosphericScatteringSkyDoom_PS.hlsl
	shaders/VolumetricClouds_PS.hlsl
)

set( SHADER_COMMON_FILES
	shaders/CommonInclude.hlsl
)

add_library( Core STATIC
    ${HEADER_FILES}
    ${SOURCE_FILES}
	${SHADER_FILES}
	${SHADER_COMMON_FILES}
    ${IMGUI_HEADERS} ${IMGUI_SOURCE}
	${CONFIG_SYS_HEADERS} ${CONFIG_SYS_SOURCE}
	${STB_IMAGE_HEADERS}	
)

# Enable C++20 compiler features.
target_compile_features( Core 
	PUBLIC cxx_std_20
)

source_group( "shaders" FILES ${SHADER_FILES} ${SHADER_COMMON_FILES})

target_include_directories( Core
    PUBLIC inc
	PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries( Core
	PUBLIC DirectXTex
    PUBLIC assimp	
    PUBLIC d3d12.lib
    PUBLIC dxgi.lib
    PUBLIC dxguid.lib
)

set_source_files_properties( ${SHADER_FILES}
    PROPERTIES
        VS_SHADER_MODEL 5.1
        VS_SHADER_OUTPUT_HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/%(Filename).h"
        VS_SHADER_DISABLE_OPTIMIZATIONS $<$<CONFIG:Debug>:ON>
        VS_SHADER_ENABLE_DEBUG $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ON>
)

set_source_files_properties( ${SHADER_COMMON_FILES}
    PROPERTIES
        VS_SHADER_MODEL 5.1
		VS_SHADER_TYPE Compute
		VS_SHADER_ENTRYPOINT "main_fake"
        VS_SHADER_OUTPUT_HEADER_FILE "${CMAKE_CURRENT_BINARY_DIR}/%(Filename).h"
        VS_SHADER_DISABLE_OPTIMIZATIONS $<$<CONFIG:Debug>:ON>
        VS_SHADER_ENABLE_DEBUG $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ON>
)

set_source_files_properties( shaders/AtmosphericScatteringSkyDoom_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_AtmosphericScatteringSkyDoom_VS
)

set_source_files_properties( shaders/AtmosphericScatteringSkyDoom_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_AtmosphericScatteringSkyDoom_PS
)

set_source_files_properties( shaders/GenerateMips_CS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Compute
        VS_SHADER_VARIABLE_NAME g_GenerateMips_CS
)

set_source_files_properties( shaders/PanoToCubemap_CS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Compute
        VS_SHADER_VARIABLE_NAME g_PanoToCubemap_CS
)

set_source_files_properties( shaders/ForwardPlus_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_ForwardPlus_VS
)

set_source_files_properties( shaders/ForwardPlus_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_ForwardPlus_PS
)

set_source_files_properties( shaders/DepthBuffer_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_DepthBuffer_VS
)

set_source_files_properties( shaders/DepthBuffer_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_DepthBuffer_PS
)

set_source_files_properties( shaders/DebugDepthBuffer_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_DebugDepthBuffer_VS
)

set_source_files_properties( shaders/DebugDepthBuffer_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_DebugDepthBuffer_PS
)

set_source_files_properties( shaders/Quad_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_Quad_VS
)

set_source_files_properties( shaders/Quad_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_Quad_PS
)

set_source_files_properties( shaders/QuadTC_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_QuadTC_VS
)

set_source_files_properties( shaders/PanoToCubemap_CS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Compute
        VS_SHADER_VARIABLE_NAME g_PanoToCubemap_CS
)

set_source_files_properties( shaders/PerturbedClouds_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_PerturbedClouds_PS
)

set_source_files_properties( shaders/VolumetricClouds_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_VolumetricClouds_PS
)

set_source_files_properties( shaders/GridViewFrustums_CS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Compute
        VS_SHADER_VARIABLE_NAME g_GridViewFrustums_CS
)

set_source_files_properties( shaders/LightCulling_CS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Compute
        VS_SHADER_VARIABLE_NAME g_LightCulling_CS
)

set_source_files_properties( shaders/LightsToView_CS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Compute
        VS_SHADER_VARIABLE_NAME g_LightsToView_CS
)

set_source_files_properties( shaders/ImGUI_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_ImGUI_VS
)

set_source_files_properties( shaders/ImGUI_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_ImGUI_PS
)

set_source_files_properties( shaders/Skybox_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_Skybox_VS
)

set_source_files_properties( shaders/Skybox_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_Skybox_PS
)

set_source_files_properties( shaders/BitmapClouds_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_BitmapClouds_VS
)

set_source_files_properties( shaders/BitmapClouds_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_BitmapClouds_PS
)

set_source_files_properties( shaders/SkyDoomGradient_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_SkyDoomGradient_PS
)

set_source_files_properties( shaders/VoxelGridFill_VS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Vertex
        VS_SHADER_VARIABLE_NAME g_VoxelGridFill_VS
)

set_source_files_properties( shaders/VoxelGridFill_GS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Geometry
        VS_SHADER_VARIABLE_NAME g_VoxelGridFill_GS
)

set_source_files_properties( shaders/VoxelGridFill_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_VoxelGridFill_PS
)

set_source_files_properties( shaders/VoxelGridDebug_PS.hlsl
    PROPERTIES
        VS_SHADER_TYPE Pixel
        VS_SHADER_VARIABLE_NAME g_VoxelGridDebug_PS
)