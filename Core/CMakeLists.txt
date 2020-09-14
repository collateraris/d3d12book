cmake_minimum_required( VERSION 3.18.2 ) # Latest version of CMake when this file was created.

set( HEADER_FILES 
    inc/DX12LibPCH.h
    inc/Application.h
    inc/CommandQueue.h
    inc/Game.h
	inc/Events.h
    inc/Helpers.h
    inc/HighResolutionClock.h
    inc/Window.h
    inc/d3dx12.h
)


set( SOURCE_FILES
    src/Application.cpp
    src/CommandQueue.cpp
    src/Game.cpp
    src/HighResolutionClock.cpp
    src/Window.cpp
)

add_library( Core STATIC
    ${HEADER_FILES}
    ${SOURCE_FILES}
)

target_include_directories( Core
    PUBLIC inc
)

target_link_libraries( Core 
    PUBLIC d3d12.lib
    PUBLIC dxgi.lib
    PUBLIC dxguid.lib
)