cmake_minimum_required(VERSION 3.12)

project(FloatingSandboxWeb C CXX)

if(NOT EMSCRIPTEN)
    message(FATAL_ERROR "This project must be configured with the Emscripten toolchain (use emcmake)")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FS_WEB_DEPS_PREFIX "" CACHE PATH "Install prefix containing wasm builds of zlib and libpng")
set(FS_PICOJSON_DIR "" CACHE PATH "Directory containing picojson.h")

if(NOT FS_WEB_DEPS_PREFIX OR NOT FS_PICOJSON_DIR)
    message(FATAL_ERROR "FS_WEB_DEPS_PREFIX and FS_PICOJSON_DIR must be set")
endif()

find_library(FS_WEB_PNG_LIBRARY NAMES png16 png PATHS ${FS_WEB_DEPS_PREFIX}/lib NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)
find_library(FS_WEB_ZLIB_LIBRARY NAMES z PATHS ${FS_WEB_DEPS_PREFIX}/lib NO_DEFAULT_PATH NO_CMAKE_FIND_ROOT_PATH REQUIRED)

set(FS_SOURCES_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/../Sources)

add_compile_definitions(PICOJSON_USE_INT64 LIBPNG_MODE_LIB)

function(fs_web_module name)
    file(GLOB_RECURSE module_sources CONFIGURE_DEPENDS ${FS_SOURCES_ROOT}/${name}/*.cpp)
    list(FILTER module_sources EXCLUDE REGEX "/_Dead/")
    add_library(${name} STATIC ${module_sources})
    target_include_directories(${name} PUBLIC
        ${FS_SOURCES_ROOT}
        ${FS_PICOJSON_DIR}
        ${FS_WEB_DEPS_PREFIX}/include)
    target_include_directories(${name} PRIVATE ${FS_SOURCES_ROOT}/${name})
endfunction()

fs_web_module(Core)
fs_web_module(Simulation)
fs_web_module(Game)
fs_web_module(OpenGLCore)
fs_web_module(Render)
fs_web_module(SoundCore)

add_custom_target(FloatingSandboxWebAll DEPENDS Core Simulation Game OpenGLCore Render SoundCore)

add_library(FloatingSandboxWeb INTERFACE)
target_link_libraries(FloatingSandboxWeb INTERFACE
    Game
    Render
    OpenGLCore
    Simulation
    SoundCore
    Core
    ${FS_WEB_PNG_LIBRARY}
    ${FS_WEB_ZLIB_LIBRARY})
