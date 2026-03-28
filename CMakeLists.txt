cmake_minimum_required(VERSION 3.21)

project(EasySW LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(EasySW WIN32
    resources/app.rc
    src/main.cpp
    src/App.cpp
    src/App.h
    src/SettingsStore.cpp
    src/SettingsStore.h
    src/StartupManager.cpp
    src/StartupManager.h
    src/TextInteractor.cpp
    src/TextInteractor.h
    src/TextTransform.cpp
    src/TextTransform.h
    src/UiAutomationTextProvider.cpp
    src/UiAutomationTextProvider.h
)

target_compile_definitions(EasySW PRIVATE
    UNICODE
    _UNICODE
    WIN32_LEAN_AND_MEAN
    NOMINMAX
)

target_link_libraries(EasySW PRIVATE
    advapi32
    user32
    shell32
    ole32
    comctl32
    uiautomationcore
)

if(MSVC)
    set_property(TARGET EasySW PROPERTY
        MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_options(EasySW PRIVATE
        /W4
        /permissive-
        /EHsc
        /utf-8
        /Zc:__cplusplus
    )
endif()
