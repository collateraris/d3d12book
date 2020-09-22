cmake_minimum_required( VERSION 3.18.2 ) # Latest version of CMake when this file was created.

# Use solution folders to organize projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Make sure DLL and EXE targets go to the same directory.
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib) # Output directory for static lib (.LIB)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Output directory for shared lib (.DLL)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin) # Output directory for executables (.EXE)

project( LearningDirectX12 LANGUAGES CXX )

# Enable multithreaded builds
if( MSVC )
    add_compile_options(/MP)
endif()

add_subdirectory( DirectXTex )
add_subdirectory( Core )
add_subdirectory( Lesson1_cube3D )

# Set the startup project.
set_directory_properties( PROPERTIES 
    VS_STARTUP_PROJECT Lesson1_cube3D
)
