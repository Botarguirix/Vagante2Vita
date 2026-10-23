# Vagante reconstruction (rama A).
#   PC:   cmake -S recon -B _local/build-recon-pc -G Ninja -DSFML_DIR=<repo>/_local/deps/sfml-pc/lib/cmake/SFML
# Generated inputs (tools/pdb_headers.py, tools/pdb_stubs.py) must exist first:
#   recon/include/*.h, recon/stubs/*.cpp
cmake_minimum_required(VERSION 3.16)
project(vagante_recon C CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(REPO_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(RAPIDJSON_INCLUDE "${REPO_ROOT}/_local/deps/rapidjson-1.1.0/include" CACHE PATH "rapidjson include dir")

file(GLOB RECON_SRC CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB RECON_STUBS CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/stubs/*.cpp")
if(NOT RECON_STUBS)
  message(FATAL_ERROR "recon/stubs is empty: run tools/pdb_stubs.py first")
endif()

# Port helpers live in their own library: they must not see recon/include, whose generated
# headers can shadow system ones (the game has a type "rpc" -> rpc.h, used by <windows.h>).
add_library(recon_port STATIC port/recon_stub.cpp port/gl_info.cpp)
target_compile_definitions(recon_port PRIVATE SFML_STATIC)

add_executable(vagante ${RECON_SRC} ${RECON_STUBS})
target_include_directories(vagante PRIVATE include port "${RAPIDJSON_INCLUDE}")
target_precompile_headers(vagante PRIVATE include/recon_all.h)
# Reconstructed code keeps MSVC-era idioms; keep warnings quiet until units are cleaned up.
target_compile_options(vagante PRIVATE -w)

option(RECON_SFML_SDL "Use SFML with the SDL2 backend (recon/sfml-sdl), as on the Vita" OFF)
if(RECON_SFML_SDL OR VITA)
  add_subdirectory(sfml-sdl "${CMAKE_BINARY_DIR}/sfml-sdl")
  set(RECON_SFML sfml-sdl-graphics sfml-sdl-window sfml-sdl-audio sfml-sdl-network sfml-sdl-system)
else()
  set(SFML_STATIC_LIBRARIES TRUE)
  find_package(SFML 2.5 REQUIRED COMPONENTS graphics window audio network system)
  set(RECON_SFML sfml-graphics sfml-window sfml-audio sfml-network sfml-system)
endif()
find_package(ZLIB REQUIRED)
target_link_libraries(recon_port PRIVATE ${RECON_SFML})
target_link_libraries(vagante PRIVATE recon_port ${RECON_SFML} ZLIB::ZLIB)
target_compile_definitions(vagante PRIVATE SFML_STATIC)

if(VITA)
  # PS Vita package. Game data is not inside the VPK: data.vra goes to ux0:data/Vagante/.
  include("$ENV{VITASDK}/share/vita.cmake" REQUIRED)
  set(VITA_APP_NAME "Vagante rama A")
  set(VITA_TITLEID "VAGA00001")  # distinct from the rama B demo (VAGV00006)
  set(VITA_VERSION "00.01")
  set(VITA_MKSFOEX_FLAGS "${VITA_MKSFOEX_FLAGS} -d ATTRIBUTE2=12")  # extended memory mode
  target_sources(vagante PRIVATE port/vita_gl_compat.c)
  set_source_files_properties(port/vita_gl_compat.c PROPERTIES SKIP_PRECOMPILE_HEADERS ON)
  # Static archives resolve left to right: system stubs and pthread must come last.
  target_link_libraries(vagante PRIVATE -Wl,--whole-archive pthread -Wl,--no-whole-archive)
  set(CMAKE_CXX_STANDARD_LIBRARIES "${CMAKE_CXX_STANDARD_LIBRARIES} -lSceAudio_stub -lSceAudioIn_stub -lpthread")
  vita_create_self(eboot.bin vagante UNSAFE)
  # The VPK itself is packed by tools/build_recon_vita.sh: vita.cmake passes unquoted paths to
  # vita-pack-vpk, which crashes on paths with spaces ("C:/Users/Andy PC/...").
endif()
