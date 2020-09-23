cmake_minimum_required( VERSION 3.18.2 ) # Latest version of CMake when this file was created.

set( HEADER_FILES 
    inc/DX12LibPCH.h
    inc/Application.h
    inc/Buffer.h
    inc/ByteAddressBuffer.h
	inc/Camera.h
    inc/CommandList.h
    inc/CommandQueue.h
    inc/ConstantBuffer.h
    inc/DescriptorAllocation.h
    inc/DescriptorAllocator.h
    inc/DescriptorAllocatorPage.h
    inc/DynamicDescriptorHeap.h
    inc/Events.h
    inc/Game.h
    inc/GenerateMipsPSO.h
    inc/GUI.h
    inc/Helpers.h
    inc/HighResolutionClock.h
    inc/IndexBuffer.h
    inc/KeyCodes.h
    inc/Mesh.h
    inc/PanoToCubemapPSO.h
    inc/RenderTarget.h
    inc/Resource.h
    inc/ResourceStateTracker.h
    inc/RootSignature.h
    inc/Scene.h
    inc/SceneNode.h
    inc/StructuredBuffer.h
    inc/Texture.h
    inc/TextureUsage.h
    inc/ThreadSafeQueue.h
    inc/UploadBuffer.h
	inc/URootObject.h
    inc/VertexBuffer.h
    inc/Window.h
    inc/d3dx12.h
)

set( SOURCE_FILES
    src/Application.cpp
    src/Buffer.cpp
    src/ByteAddressBuffer.cpp
	src/Camera.cpp
    src/CommandQueue.cpp
    src/CommandList.cpp
    src/ConstantBuffer.cpp
    src/DescriptorAllocation.cpp
    src/DescriptorAllocator.cpp
    src/DescriptorAllocatorPage.cpp
    src/DynamicDescriptorHeap.cpp
    src/Game.cpp
    src/GenerateMipsPSO.cpp
    src/GUI.cpp
    src/HighResolutionClock.cpp
    src/IndexBuffer.cpp
    src/Mesh.cpp
    src/PanoToCubemapPSO.cpp
    src/RenderTarget.cpp
    src/Resource.cpp
    src/ResourceStateTracker.cpp
    src/RootSignature.cpp
    src/SceneNode.cpp
    src/StructuredBuffer.cpp
    src/Texture.cpp
    src/UploadBuffer.cpp
	src/URootObject.cpp
    src/VertexBuffer.cpp
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

set( SHADER_FILES
    shaders/GenerateMips_CS.hlsl
	shaders/PanoToCubemap_CS.hlsl
    shaders/ImGUI_VS.hlsl
    shaders/ImGUI_PS.hlsl
)

add_library( Core STATIC
    ${HEADER_FILES}
    ${SOURCE_FILES}
	${SHADER_FILES}
    ${IMGUI_HEADERS} ${IMGUI_SOURCE}	
)

# Enable C++17 compiler features.
target_compile_features( Core 
	PUBLIC cxx_std_17
)

source_group( "shaders" FILES ${SHADER_FILES} )

target_include_directories( Core
    PUBLIC inc
	PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries( Core
	PUBLIC DirectXTex 
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