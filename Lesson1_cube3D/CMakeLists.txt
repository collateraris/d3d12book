cmake_minimum_required( VERSION 3.18.3 ) 

set( HEADER_FILES 
    inc/Lesson1_cube3d.h
)

set( SRC_FILES
	main.cpp
	src/Lesson1_cube3d.cpp
)

set( SHADER_FILES
    shaders/VertexShader.hlsl
    shaders/PixelShader.hlsl
	shaders/ScreenQuadVS.hlsl
	shaders/ScreenQuadPS.hlsl
	shaders/SkyboxVS.hlsl
	shaders/SkyboxPS.hlsl
)

set_source_files_properties( ${SHADER_FILES} 
    PROPERTIES
        VS_SHADER_OBJECT_FILE_NAME "${CMAKE_BINARY_DIR}/bin/%(Filename).cso"
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

add_executable( Lesson1_cube3D WIN32
	${HEADER_FILES}
    ${SRC_FILES}
	${SHADER_FILES}
)

target_include_directories( Lesson1_cube3D
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Enable C++20 compiler features.
target_compile_features( Lesson1_cube3D 
	PUBLIC cxx_std_20
)

target_link_libraries( Lesson1_cube3D
    Core
    D3DCompiler.lib
    Shlwapi.lib
	DirectXTex
)
