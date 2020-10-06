cmake_minimum_required( VERSION 3.18.3 ) 

set( HEADER_FILES 
    inc/Terrain_Project.h
	inc/CameraEuler.h
)

set( SRC_FILES
	main.cpp
	src/Terrain_Project.cpp
	src/CameraEuler.cpp
)

set( SHADER_FILES
    shaders/VertexShader.hlsl
    shaders/PixelShader.hlsl
	shaders/ScreenQuadVS.hlsl
	shaders/ScreenQuadPS.hlsl
	shaders/SkyboxVS.hlsl
	shaders/SkyboxPS.hlsl
	shaders/ForwardVS.hlsl
	shaders/ForwardPS.hlsl
)

set_source_files_properties( ${SHADER_FILES} 
    PROPERTIES
        VS_SHADER_OBJECT_FILE_NAME "${CMAKE_BINARY_DIR}/bin/$(Configuration)/%(Filename).cso"
        VS_SHADER_MODEL 5.1
        VS_SHADER_DISABLE_OPTIMIZATIONS $<$<CONFIG:Debug>:ON>
        VS_SHADER_ENABLE_DEBUG $<$<OR:$<CONFIG:Debug>,$<CONFIG:RelWithDebInfo>>:ON>
)

set_source_files_properties( shaders/VertexShader.hlsl
    PROPERTIES 
        VS_SHADER_TYPE Vertex
		VS_SHADER_VARIABLE_NAME VertexShader
)

set_source_files_properties( shaders/PixelShader.hlsl
    PROPERTIES 
        VS_SHADER_TYPE Pixel
		VS_SHADER_VARIABLE_NAME PixelShader
)

set_source_files_properties( shaders/ScreenQuadVS.hlsl
    PROPERTIES 
        VS_SHADER_TYPE Vertex
		VS_SHADER_VARIABLE_NAME ScreenQuadVS
)

set_source_files_properties( shaders/ScreenQuadPS.hlsl
    PROPERTIES 
        VS_SHADER_TYPE Pixel
		VS_SHADER_VARIABLE_NAME ScreenQuadPS
)

set_source_files_properties( shaders/SkyboxVS.hlsl
    PROPERTIES 
        VS_SHADER_TYPE Vertex
		VS_SHADER_VARIABLE_NAME SkyboxVS
)

set_source_files_properties( shaders/SkyboxPS.hlsl
    PROPERTIES 
        VS_SHADER_TYPE Pixel
		VS_SHADER_VARIABLE_NAME SkyboxPS
)

set_source_files_properties( shaders/ForwardVS.hlsl
    PROPERTIES 
        VS_SHADER_TYPE Vertex
		VS_SHADER_VARIABLE_NAME ForwardVS
)

set_source_files_properties( shaders/ForwardPS.hlsl
    PROPERTIES 
        VS_SHADER_TYPE Pixel
		VS_SHADER_VARIABLE_NAME ForwardPS
)

add_executable( Terrain_Project WIN32
	${HEADER_FILES}
    ${SRC_FILES}
	${SHADER_FILES}
)

target_include_directories( Terrain_Project
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Enable C++20 compiler features.
target_compile_features( Terrain_Project 
	PUBLIC cxx_std_20
)

target_link_libraries( Terrain_Project
    Core
    D3DCompiler.lib
    Shlwapi.lib
	DirectXTex
)

# Set debug database to enable the "Edit and Continue" feature.
target_compile_options( Terrain_Project
    PRIVATE $<$<CONFIG:Debug>:/ZI>
)

