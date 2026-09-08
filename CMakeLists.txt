cmake_minimum_required(VERSION 3.20)

project(sp400emu LANGUAGES C CXX)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
find_package(Threads REQUIRED)

set(M68_OPTABLE_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
set(M68_OPTABLE "${M68_OPTABLE_DIR}/m68_optab_hc05.h")
set(PAPER_CURSOR_ASSET_HEADER "${M68_OPTABLE_DIR}/paper_cursor_assets.h")
add_custom_command(
  OUTPUT "${M68_OPTABLE}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${M68_OPTABLE_DIR}"
  COMMAND "${CMAKE_COMMAND}"
          "-DPYTHON_EXECUTABLE=${Python3_EXECUTABLE}"
          "-DGENERATOR=${CMAKE_CURRENT_SOURCE_DIR}/m68emu/optable/makeoptab.py"
          "-DOPCODE_CSV=${CMAKE_CURRENT_SOURCE_DIR}/m68emu/optable/opcodes_m68hc05.csv"
          "-DOUTPUT_FILE=${M68_OPTABLE}"
          -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/GenerateM68Optable.cmake"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/m68emu/optable/makeoptab.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/m68emu/optable/opcodes_m68hc05.csv"
  VERBATIM
)

add_custom_command(
  OUTPUT "${PAPER_CURSOR_ASSET_HEADER}"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${M68_OPTABLE_DIR}"
  COMMAND "${Python3_EXECUTABLE}"
          "${CMAKE_CURRENT_SOURCE_DIR}/scripts/embed_cursor_assets.py"
          --open "${CMAKE_CURRENT_SOURCE_DIR}/assets/cursors/open_hand.bmp"
          --closed "${CMAKE_CURRENT_SOURCE_DIR}/assets/cursors/closed_hand.bmp"
          --output "${PAPER_CURSOR_ASSET_HEADER}"
  DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/scripts/embed_cursor_assets.py"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/cursors/open_hand.bmp"
    "${CMAKE_CURRENT_SOURCE_DIR}/assets/cursors/closed_hand.bmp"
  VERBATIM
)

add_executable(sp400
  board.cpp
  file_input.cpp
  m68sys.cpp
  main.cpp
  paper_cursor.cpp
  plotter.cpp
  serial.cpp
  stepper.c
  tcp_input.cpp
  m68emu/m68emu.c
  m68emu/m68_ops.c
  m68emu/m68tmr.c
  "${M68_OPTABLE}"
  "${PAPER_CURSOR_ASSET_HEADER}"
)

target_compile_features(sp400 PRIVATE c_std_11 cxx_std_17)
target_include_directories(sp400 PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}"
  "${M68_OPTABLE_DIR}"
)
target_link_libraries(sp400 PRIVATE Threads::Threads)

if(MSVC)
  target_compile_options(sp400 PRIVATE /W4)
endif()

if(WIN32)
  set(SDL2_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/third_party/SDL2-2.32.2"
      CACHE PATH "Path to the SDL2 2.32.2 Visual C development package")
  set(SDL2_DLL "${SDL2_ROOT}/lib/x64/SDL2.dll")
  set(SP400_ROM "${CMAKE_CURRENT_SOURCE_DIR}/sp400_6805.bin")

  foreach(required_file
      "${SDL2_ROOT}/include/SDL.h"
      "${SDL2_ROOT}/lib/x64/SDL2.lib"
      "${SDL2_DLL}"
      "${SP400_ROM}")
    if(NOT EXISTS "${required_file}")
      message(FATAL_ERROR "Required file not found: ${required_file}")
    endif()
  endforeach()

  add_library(SDL2::SDL2 SHARED IMPORTED GLOBAL)
  set_target_properties(SDL2::SDL2 PROPERTIES
    IMPORTED_IMPLIB "${SDL2_ROOT}/lib/x64/SDL2.lib"
    IMPORTED_LOCATION "${SDL2_DLL}"
    INTERFACE_INCLUDE_DIRECTORIES "${SDL2_ROOT}/include"
  )

  target_compile_definitions(sp400 PRIVATE
    SDL_MAIN_HANDLED
    NOMINMAX
    WIN32_LEAN_AND_MEAN
  )
  target_link_libraries(sp400 PRIVATE SDL2::SDL2 ws2_32)

  add_custom_command(TARGET sp400 POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${SDL2_DLL}" "$<TARGET_FILE_DIR:sp400>/SDL2.dll"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
            "${SP400_ROM}" "$<TARGET_FILE_DIR:sp400>/sp400_6805.bin"
    VERBATIM
  )
else()
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(SDL2 REQUIRED IMPORTED_TARGET sdl2)
  target_link_libraries(sp400 PRIVATE PkgConfig::SDL2)
endif()

include(CTest)
if(BUILD_TESTING)
  add_executable(m68sys_input_test
    tests/m68sys_input_test.cpp
    m68sys.cpp
    stepper.c
    m68emu/m68emu.c
    m68emu/m68_ops.c
    m68emu/m68tmr.c
    "${M68_OPTABLE}"
  )
  target_compile_features(m68sys_input_test PRIVATE c_std_11 cxx_std_17)
  target_include_directories(m68sys_input_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
    "${M68_OPTABLE_DIR}"
  )
  target_compile_definitions(m68sys_input_test PRIVATE
    SP400_ROM_PATH="${CMAKE_CURRENT_SOURCE_DIR}/sp400_6805.bin"
  )
  if(MSVC)
    target_compile_options(m68sys_input_test PRIVATE /W4)
  endif()
  add_test(NAME m68sys_input COMMAND m68sys_input_test)

  add_executable(tcp_input_test
    tests/tcp_input_test.cpp
    tcp_input.cpp
  )
  target_compile_features(tcp_input_test PRIVATE cxx_std_17)
  target_include_directories(tcp_input_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  target_link_libraries(tcp_input_test PRIVATE Threads::Threads)
  if(MSVC)
    target_compile_options(tcp_input_test PRIVATE /W4)
  endif()
  if(WIN32)
    target_compile_definitions(tcp_input_test PRIVATE
      NOMINMAX
      WIN32_LEAN_AND_MEAN
    )
    target_link_libraries(tcp_input_test PRIVATE ws2_32)
  endif()
  add_test(NAME tcp_input COMMAND tcp_input_test)

  add_executable(front_panel_test tests/front_panel_test.cpp)
  target_compile_features(front_panel_test PRIVATE cxx_std_17)
  target_include_directories(front_panel_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  if(MSVC)
    target_compile_options(front_panel_test PRIVATE /W4)
  endif()
  add_test(NAME front_panel COMMAND front_panel_test)

  add_executable(paper_viewport_test tests/paper_viewport_test.cpp)
  target_compile_features(paper_viewport_test PRIVATE cxx_std_17)
  target_include_directories(paper_viewport_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}"
  )
  if(MSVC)
    target_compile_options(paper_viewport_test PRIVATE /W4)
  endif()
  add_test(NAME paper_viewport COMMAND paper_viewport_test)

  add_test(
    NAME cursor_assets
    COMMAND "${Python3_EXECUTABLE}" -B
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/embed_cursor_assets_test.py"
  )
endif()
