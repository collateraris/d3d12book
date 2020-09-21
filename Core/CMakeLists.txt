cmake_minimum_required( VERSION 3.18.2 ) # Latest version of CMake when this file was created.

set( HEADER_FILES 
    inc/DX12LibPCH.h
    inc/Application.h
    inc/CommandList.h	
    inc/CommandQueue.h
    inc/Game.h
	inc/GenerateMipsPSO.h
	inc/Events.h
    inc/Helpers.h
    inc/HighResolutionClock.h
    inc/Window.h
	inc/UploadBuffer.h
	inc/DescriptorAllocation.h
    inc/DescriptorAllocator.h
    inc/DescriptorAllocatorPage.h
	inc/DynamicDescriptorHeap.h
	inc/Defines.h
    inc/RenderTarget.h
    inc/RootSignature.h	
	inc/Resource.h
    inc/ResourceStateTracker.h
    inc/Texture.h
	inc/URootObject.h
	inc/ThreadSafeQueue.h
    inc/d3dx12.h
)


set( SOURCE_FILES
    src/Application.cpp
    src/CommandQueue.cpp
    src/CommandList.cpp	
    src/Game.cpp
	src/GenerateMipsPSO.cpp
    src/HighResolutionClock.cpp
    src/Window.cpp
	src/UploadBuffer.cpp
	src/DescriptorAllocation.cpp
    src/DescriptorAllocator.cpp
    src/DescriptorAllocatorPage.cpp
    src/DynamicDescriptorHeap.cpp
	src/RenderTarget.cpp
    src/RootSignature.cpp
    src/Resource.cpp
    src/ResourceStateTracker.cpp
    src/Texture.cpp	
	src/URootObject.cpp
)

set( SHADER_FILES
    shaders/GenerateMips_CS.hlsl
)

add_library( Core STATIC
    ${HEADER_FILES}
    ${SOURCE_FILES}
	${SHADER_FILES}
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