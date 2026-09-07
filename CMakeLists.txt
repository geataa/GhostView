cmake_minimum_required(VERSION 3.20)
project(GhostView LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(src third_party/stb)

if (WIN32)
    if (MSVC)
        add_compile_options(/utf-8)
    endif()

    add_executable(GhostView WIN32
        src/main.cpp
        src/ViewerApp.h
        src/ViewerApp.cpp
        src/ImageLoader.h
        src/ImageLoader.cpp
        src/FolderNavigator.h
        src/FolderNavigator.cpp
        src/HudRenderer.h
        src/HudRenderer.cpp
        src/ThumbnailBar.h
        src/ThumbnailBar.cpp
        src/Localization.h
        src/Localization.cpp
        src/CropToolbar.h
        src/CropToolbar.cpp
        GhostView.rc
    )

    target_link_libraries(GhostView PRIVATE
        d3d11
        dxgi
        d2d1
        dwrite
        dcomp
        windowscodecs
        propsys
        shlwapi
        comdlg32
        shell32
        user32
        gdi32
        ole32
        advapi32
    )
else()
    add_executable(GhostView
        src/main.cpp
        src/ViewerApp.h
        src/ViewerApp.cpp
        src/ImageLoader.h
        src/ImageLoader.cpp
        src/FolderNavigator.h
        src/FolderNavigator.cpp
        src/HudRenderer.h
        src/HudRenderer.cpp
        src/ThumbnailBar.h
        src/ThumbnailBar.cpp
        src/Localization.h
        src/Localization.cpp
        src/CropToolbar.h
        src/CropToolbar.cpp
        src/platform/D2DCompat.h
        src/platform/D2DCompat.cpp
        src/platform/PlatformLinux.h
        src/platform/PlatformLinux.cpp
    )

    find_package(Threads REQUIRED)
    target_link_libraries(GhostView PRIVATE
        X11
        GL
        Threads::Threads
    )
endif()
