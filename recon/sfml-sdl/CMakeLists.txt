# SFML 2.5.1 with the SDL2 window backend (recon/sfml-sdl/SFML/Window/SDL).
# Used for the PS Vita build (SDL2 + vitaGL) and for testing that backend on PC.
# Needs an SFML 2.5.1 source tree patched by tools/patch_sfml_vita.py (SFML_SOURCE_DIR).
cmake_minimum_required(VERSION 3.16)
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
  project(sfml_sdl C CXX)  # standalone build; otherwise included by recon/CMakeLists.txt
endif()

if(NOT SFML_SOURCE_DIR)
  set(SFML_SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/../../_local/deps/SFML-2.5.1")
endif()
set(S "${SFML_SOURCE_DIR}/src/SFML")

if(VITA)
  # Static link lines from VitaSDK's sdl2.pc / freetype2.pc (sdl2_vitagl package).
  set(VITA_INC "$ENV{VITASDK}/arm-vita-eabi/include")
  set(SDL2_INCLUDE "${VITA_INC}/SDL2")
  set(SDL2_LIBS SDL2)
  set(SFML_GL_LIBS vitaGL vitashark mathneon SceShaccCg_stub SceShaccCgExt taihen_stub SceKernelDmacMgr_stub SceIme_stub
      SceGxm_stub SceDisplay_stub SceCtrl_stub SceAppMgr_stub SceAppUtil_stub SceAudio_stub SceAudioIn_stub
      SceSysmodule_stub SceIofilemgr_stub SceCommonDialog_stub SceTouch_stub SceHid_stub SceMotion_stub
      ScePower_stub SceProcessmgr_stub m)
  set(FREETYPE_TARGET freetype png16 bz2 z)
  set(FREETYPE_INCLUDE "${VITA_INC}/freetype2")
  set(OPENAL_LIBRARY openal)
  set(OPENAL_HEADER_DIR "${VITA_INC}/AL")
  set(VORBISFILE_LIB vorbisfile)
  set(VORBISENC_LIB vorbisenc)
  set(VORBIS_LIB vorbis)
  set(OGG_LIB ogg)
  set(FLAC_LIB FLAC)
  set(PLATFORM_DIR Unix)
elseif(WIN32)
  find_package(SDL2 REQUIRED)
  find_package(OpenGL REQUIRED)
  set(SFML_GL_LIBS OpenGL::GL)
  set(SDL2_LIBS SDL2::SDL2)
  set(PLATFORM_DIR Win32)
else()
  find_package(SDL2 REQUIRED)
  find_package(OpenGL REQUIRED)
  set(SFML_GL_LIBS OpenGL::GL)
  set(SDL2_LIBS SDL2::SDL2)
  set(PLATFORM_DIR Unix)
endif()

if(NOT VITA)
  find_package(Freetype REQUIRED)
  set(FREETYPE_TARGET Freetype::Freetype)
  find_package(OpenAL REQUIRED)
  find_library(VORBISFILE_LIB vorbisfile REQUIRED)
  find_library(VORBISENC_LIB vorbisenc REQUIRED)
  find_library(VORBIS_LIB vorbis REQUIRED)
  find_library(OGG_LIB ogg REQUIRED)
  find_library(FLAC_LIB FLAC REQUIRED)
  find_path(OPENAL_HEADER_DIR al.h PATH_SUFFIXES AL REQUIRED)
endif()

set(SFML_COMMON_INCLUDES "${CMAKE_CURRENT_LIST_DIR}" "${SFML_SOURCE_DIR}/include" "${SFML_SOURCE_DIR}/src")

function(sfml_sdl_module name)
  cmake_parse_arguments(M "" "" "SOURCES;LIBS;INCLUDES" ${ARGN})
  add_library(sfml-sdl-${name} STATIC ${M_SOURCES})
  target_include_directories(sfml-sdl-${name} PUBLIC ${SFML_COMMON_INCLUDES} PRIVATE ${M_INCLUDES})
  target_compile_definitions(sfml-sdl-${name} PUBLIC SFML_STATIC SFML_USE_SDL)
  target_compile_features(sfml-sdl-${name} PUBLIC cxx_std_17)
  target_link_libraries(sfml-sdl-${name} PUBLIC ${M_LIBS})
endfunction()

file(GLOB SYSTEM_SRC "${S}/System/*.cpp" "${S}/System/${PLATFORM_DIR}/*.cpp")
sfml_sdl_module(system SOURCES ${SYSTEM_SRC} LIBS $<$<NOT:$<BOOL:${WIN32}>>:pthread> $<$<BOOL:${WIN32}>:winmm>)

file(GLOB WINDOW_SRC "${S}/Window/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/SFML/Window/SDL/*.cpp")
list(FILTER WINDOW_SRC EXCLUDE REGEX "/(EGLCheck|EglContext)\\.cpp$")
sfml_sdl_module(window SOURCES ${WINDOW_SRC} INCLUDES ${SDL2_INCLUDE} LIBS sfml-sdl-system ${SDL2_LIBS} ${SFML_GL_LIBS})
if(SDL2_INCLUDE)
  target_include_directories(sfml-sdl-window PUBLIC ${SDL2_INCLUDE})
endif()

file(GLOB GRAPHICS_SRC "${S}/Graphics/*.cpp")
sfml_sdl_module(graphics SOURCES ${GRAPHICS_SRC} INCLUDES "${SFML_SOURCE_DIR}/extlibs/headers/stb_image" ${FREETYPE_INCLUDE}
                LIBS sfml-sdl-window ${FREETYPE_TARGET})

file(GLOB AUDIO_SRC "${S}/Audio/*.cpp")
sfml_sdl_module(audio SOURCES ${AUDIO_SRC} INCLUDES ${OPENAL_HEADER_DIR}
                LIBS sfml-sdl-system ${OPENAL_LIBRARY} ${VORBISFILE_LIB} ${VORBISENC_LIB} ${VORBIS_LIB} ${FLAC_LIB} ${OGG_LIB})
target_compile_definitions(sfml-sdl-audio PRIVATE FLAC__NO_DLL)

file(GLOB NETWORK_SRC "${S}/Network/*.cpp" "${S}/Network/${PLATFORM_DIR}/*.cpp")
sfml_sdl_module(network SOURCES ${NETWORK_SRC} LIBS sfml-sdl-system $<$<BOOL:${WIN32}>:ws2_32>)
