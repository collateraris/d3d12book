cmake_minimum_required( VERSION 3.18.3 ) 

file(COPY assets DESTINATION ${CMAKE_BINARY_DIR}/bin/Debug/)
file(COPY assets DESTINATION ${CMAKE_BINARY_DIR}/bin/Release/)

set( HEADER_FILES 
    inc/VoxelGIDemo.h
	inc/StringConstant.h
	inc/VoxelGIDemoUtils.h
)

set( SRC_FILES
	main.cpp
	src/VoxelGIDemo.cpp
)

set( SHADER_FILES
	shaders/ScreenQuadVS.hlsl
	shaders/ScreenQuadPS.hlsl
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

add_executable( VoxelGIDemo WIN32
	${HEADER_FILES}
    ${SRC_FILES}
	${SHADER_FILES}
)

target_include_directories( VoxelGIDemo
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/inc
)

# Enable C++20 compiler features.
target_compile_features( VoxelGIDemo 
	PUBLIC cxx_std_20
)

target_link_libraries( VoxelGIDemo
    Core
    D3DCompiler.lib
    Shlwapi.lib
	DirectXTex
)

# Set debug database to enable the "Edit and Continue" feature.
target_compile_options( VoxelGIDemo
    PRIVATE $<$<CONFIG:Debug>:/ZI>
)


