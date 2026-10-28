cmake_minimum_required(VERSION 3.21)
project(MegaHackGD VERSION 1.0.0 LANGUAGES C CXX)

# Versione di Geometry Dash per cui compilare (indirizzi dalle bindings geode-sdk)
#   22091 = GD 2.2091 (offset validati)   2209 = GD 2.209
set(GD_VERSION "22091" CACHE STRING "Versione di Geometry Dash (22091 o 2209)")
set_property(CACHE GD_VERSION PROPERTY STRINGS 22091 2209)

if(NOT CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(FATAL_ERROR "Geometry Dash 2.2 e' a 64 bit: compila per x64")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release)
endif()

include(FetchContent)
# SOURCE_SUBDIR inesistente: scarica solo i sorgenti, i target li definiamo noi
FetchContent_Declare(imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.9b
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR _none)
FetchContent_Declare(minhook
    GIT_REPOSITORY https://github.com/TsudaKageyu/minhook.git
    GIT_TAG c3fcafdc10146beb5919319d0683e44e3c30d537 # v1.3.4
    SOURCE_SUBDIR _none)
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
    SOURCE_SUBDIR _none)
FetchContent_MakeAvailable(imgui minhook nlohmann_json)

# --- MinHook ---
add_library(minhook STATIC
    ${minhook_SOURCE_DIR}/src/buffer.c
    ${minhook_SOURCE_DIR}/src/hook.c
    ${minhook_SOURCE_DIR}/src/trampoline.c
    ${minhook_SOURCE_DIR}/src/hde/hde64.c)
target_include_directories(minhook PUBLIC ${minhook_SOURCE_DIR}/include)

# --- Dear ImGui (Win32 + OpenGL3) ---
add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_win32.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp)
target_include_directories(imgui PUBLIC ${imgui_SOURCE_DIR} ${imgui_SOURCE_DIR}/backends)
target_compile_definitions(imgui PUBLIC IMGUI_DISABLE_OBSOLETE_FUNCTIONS)
target_link_libraries(imgui PUBLIC opengl32 dwmapi)

# --- Lingue incorporate (Mega-Hack-Translate) ---
set(LANG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/resources/langs)
set(LANG_GEN ${CMAKE_CURRENT_BINARY_DIR}/generated/embedded_langs.hpp)
file(GLOB LANG_FILES ${LANG_DIR}/*.json)
add_custom_command(
    OUTPUT ${LANG_GEN}
    COMMAND ${CMAKE_COMMAND} -DLANG_DIR=${LANG_DIR} -DOUT=${LANG_GEN} -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedLangs.cmake
    DEPENDS ${LANG_FILES} ${CMAKE_CURRENT_SOURCE_DIR}/cmake/EmbedLangs.cmake
    COMMENT "Incorporo i file di lingua")

# --- Mod menu ---
file(GLOB_RECURSE MH_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_library(MegaHackGD SHARED ${MH_SOURCES} ${LANG_GEN})
target_include_directories(MegaHackGD PRIVATE src ${CMAKE_CURRENT_BINARY_DIR}/generated ${nlohmann_json_SOURCE_DIR}/single_include)
target_compile_definitions(MegaHackGD PRIVATE
    GD_VERSION=${GD_VERSION}
    MH_VERSION_STR="${PROJECT_VERSION}"
    WIN32_LEAN_AND_MEAN NOMINMAX UNICODE _UNICODE _CRT_SECURE_NO_WARNINGS)
target_link_libraries(MegaHackGD PRIVATE minhook imgui psapi shell32 user32 gdi32 winmm)
if(MSVC)
    target_compile_options(MegaHackGD PRIVATE /W3 /utf-8 /permissive- /bigobj)
else()
    target_compile_options(MegaHackGD PRIVATE -Wall -Wno-unused-function -Wno-missing-field-initializers)
    target_link_options(MegaHackGD PRIVATE -static -static-libgcc -static-libstdc++)
endif()
set_target_properties(MegaHackGD PROPERTIES OUTPUT_NAME "MegaHackGD" PREFIX "")

# --- Loader senza Geode: proxy di XInput1_4.dll ---
add_library(xinput_proxy SHARED loader/xinput_proxy.cpp loader/xinput_proxy.def)
set_target_properties(xinput_proxy PROPERTIES OUTPUT_NAME "XInput1_4" PREFIX "")
target_compile_definitions(xinput_proxy PRIVATE WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
if(NOT MSVC)
    target_link_options(xinput_proxy PRIVATE -static -static-libgcc -static-libstdc++)
endif()

# --- Injector alternativo ---
add_executable(MegaHackInjector loader/injector.cpp)
target_compile_definitions(MegaHackInjector PRIVATE WIN32_LEAN_AND_MEAN UNICODE _UNICODE)
target_link_libraries(MegaHackInjector PRIVATE psapi)
if(NOT MSVC)
    target_link_options(MegaHackInjector PRIVATE -static -municode)
endif()
