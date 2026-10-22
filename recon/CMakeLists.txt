# Vagante reconstruction (rama A).
#   PC:   cmake -S recon -B _local/build-recon-pc -G Ninja -DSFML_DIR=<repo>/_local/deps/sfml-pc/lib/cmake/SFML
# Generated inputs (tools/pdb_headers.py, tools/pdb_stubs.py) must exist first:
#   recon/include/*.h, recon/stubs/*.cpp
cmake_minimum_required(VERSION 3.16)
project(vagante_recon CXX)
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

set(SFML_STATIC_LIBRARIES TRUE)
find_package(SFML 2.5 REQUIRED COMPONENTS graphics window audio network system)
find_package(ZLIB REQUIRED)
target_link_libraries(recon_port PRIVATE sfml-graphics sfml-window sfml-system)
target_link_libraries(vagante PRIVATE recon_port sfml-graphics sfml-window sfml-audio sfml-network sfml-system ZLIB::ZLIB)
target_compile_definitions(vagante PRIVATE SFML_STATIC)
