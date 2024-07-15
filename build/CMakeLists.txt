cmake_policy(SET CMP0091 NEW)
cmake_minimum_required (VERSION 3.21)
project (xt-audio)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Assumes MSVC.
if (WIN32)
  message (STATUS "Building for Windows.")
# Assumes CLang.
elseif (APPLE)
  message (STATUS "Building for MacOS.")
elseif (UNIX AND NOT APPLE)
# Assumes GCC.
# Really Linux, because ALSA.
  message (STATUS "Building for Linux.")
else ()
  message (FATAL_ERROR "Unsupported platform.")
endif ()

# Make sure we don't export unwanted stuff.
if (APPLE)
  add_compile_options(-fvisibility=hidden -fvisibility-inlines-hidden)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -fvisibility-inlines-hidden")
elseif (UNIX AND NOT APPLE)
  add_compile_options(-Wl,--no-undefined -fvisibility=hidden -fvisibility-inlines-hidden)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wl,--no-undefined -fvisibility=hidden -fvisibility-inlines-hidden")
endif()

# Static link runtime libs.
if (WIN32)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif (WIN32)

# Core library (C API).
set (CORE_DIR "../src/core/xt")
file (GLOB_RECURSE CORE_SRC "${CORE_DIR}/*.*")
add_library (xt-audio SHARED ${CORE_SRC})
target_compile_options (xt-audio PRIVATE -DXT_EXPORT=1)
target_include_directories (xt-audio PRIVATE ${CORE_DIR})
if (WIN32)
  source_group(TREE "../../${CORE_DIR}" FILES ${CORE_SRC})
  set_target_properties(xt-audio PROPERTIES RUNTIME_OUTPUT_DIRECTORY "../../../dist/core/xt")
endif ()

# TODO make sure this goes for clang too, not just gcc.
if (UNIX)
  set_target_properties(xt-audio PROPERTIES LIBRARY_OUTPUT_DIRECTORY "../../../../dist/core/xt/${CMAKE_BUILD_TYPE}")
endif ()

# C++ library (C++ API).
set (CPP_DIR "../src/cpp/xt")
file (GLOB_RECURSE CPP_SRC "${CPP_DIR}/*.*")
add_library(xt-cpp INTERFACE)
target_sources(xt-cpp INTERFACE ${CPP_SRC})
target_include_directories (xt-cpp INTERFACE ${CORE_DIR})
add_library(xt-cpp_ ${CPP_SRC})
target_link_libraries (xt-cpp_ xt-audio)
target_include_directories (xt-cpp_ PRIVATE ${CPP_DIR})
target_include_directories (xt-cpp_ PRIVATE ${CORE_DIR})
set_target_properties(xt-cpp_ PROPERTIES LINKER_LANGUAGE CXX)
if (WIN32)
  source_group(TREE "../../${CPP_DIR}" FILES ${CPP_SRC})
endif ()

# C++ sample program.
set (SAMPLE_DIR "../src/cpp/sample")
file (GLOB SAMPLE_SRC "${SAMPLE_DIR}/*.*")
add_executable (xt-sample ${SAMPLE_SRC})
target_link_libraries (xt-sample xt-audio)
target_include_directories (xt-sample PRIVATE ${CPP_DIR})
target_include_directories (xt-sample PRIVATE ${CORE_DIR})
if (WIN32)
  source_group(TREE "../../${SAMPLE_DIR}" FILES ${SAMPLE_SRC})
  set_target_properties(xt-sample PROPERTIES RUNTIME_OUTPUT_DIRECTORY "../../../dist/cpp/sample")
endif ()

# TODO make sure this goes for clang too, not just gcc.
if (UNIX)
  set_target_properties(xt-sample PROPERTIES RUNTIME_OUTPUT_DIRECTORY "../../../../dist/cpp/sample/${CMAKE_BUILD_TYPE}")
endif ()

# Runtime dependencies.
if (XT_ENABLE_JACK)
  target_link_libraries (xt-audio jack)
endif ()
if (XT_ENABLE_ALSA)
  target_link_libraries (xt-audio asound)
endif ()
if (XT_ENABLE_WASAPI)
  target_link_libraries (xt-audio avrt)
endif ()
if (XT_ENABLE_PULSE)
  target_link_libraries (xt-audio pulse-simple pulse)
endif ()
if (XT_ENABLE_DSOUND)
  target_link_libraries (xt-audio dsound dxguid winmm)
endif ()
if (XT_ENABLE_CORE_AUDIO)
#TODO should we link in any system libs here?
endif ()

# Source code dependencies.
if (XT_ENABLE_ASIO)
  file (GLOB_RECURSE ASMJIT_SRC "${XT_ASMJIT_DIR}/*.*")
  add_library (asmjit STATIC ${ASMJIT_SRC})
  target_compile_options (asmjit PUBLIC -DASMJIT_STATIC=1)
  target_include_directories (xt-audio PRIVATE ${XT_ASMJIT_DIR})
  target_link_libraries (xt-audio asmjit)
endif ()

# Source code dependencies.
if (XT_ENABLE_ASIO)
  file (GLOB ASIOSDK_SRC "${XT_ASIOSDK_DIR}/host/asiodrivers.cpp"
                         "${XT_ASIOSDK_DIR}/host/pc/asiolist.cpp")
  add_library (asiosdk STATIC ${ASIOSDK_SRC})
  target_include_directories (asiosdk PRIVATE ${XT_ASIOSDK_DIR}/common)
  target_include_directories (asiosdk PRIVATE ${XT_ASIOSDK_DIR}/host/pc)
  target_include_directories (xt-audio PRIVATE ${XT_ASIOSDK_DIR})
  target_link_libraries (xt-audio asiosdk)
endif ()

# Backend selection.
target_compile_options (xt-audio PRIVATE -DXT_ENABLE_ASIO=${XT_ENABLE_ASIO})
target_compile_options (xt-audio PRIVATE -DXT_ENABLE_ALSA=${XT_ENABLE_ALSA})
target_compile_options (xt-audio PRIVATE -DXT_ENABLE_JACK=${XT_ENABLE_JACK})
target_compile_options (xt-audio PRIVATE -DXT_ENABLE_PULSE=${XT_ENABLE_PULSE})
target_compile_options (xt-audio PRIVATE -DXT_ENABLE_WASAPI=${XT_ENABLE_WASAPI})
target_compile_options (xt-audio PRIVATE -DXT_ENABLE_DSOUND=${XT_ENABLE_DSOUND})
target_compile_options (xt-audio PRIVATE -DXT_ENABLE_CORE_AUDIO=${XT_ENABLE_CORE_AUDIO})