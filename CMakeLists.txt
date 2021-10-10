cmake_minimum_required(VERSION 3.7)
project(cws80 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_VISIBILITY_PRESET "hidden")
set(CMAKE_CXX_VISIBILITY_PRESET "hidden")
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# options
set(CWS80_GRAPHICS_DEVICE "cairo" CACHE STRING "Graphics device")
set_property(CACHE CWS80_GRAPHICS_DEVICE PROPERTY STRINGS "cairo" "opengl")

###
add_subdirectory("dpf")
find_package(TCL)
if(NOT TCLTK_FOUND)
  message(FATAL_ERROR "Cannot find Tcl/Tk")
endif()

if(CWS80_GRAPHICS_DEVICE STREQUAL "opengl")
  find_package(GLEW REQUIRED)
endif()

find_package(OpenMP)
if(OPENMP_FOUND)
  add_compile_options(
    $<$<COMPILE_LANGUAGE:C>:${OpenMP_C_FLAGS}>
    $<$<COMPILE_LANGUAGE:CXX>:${OpenMP_CXX_FLAGS}>)
endif()

###
add_library(cws80_core
  STATIC
    "sources/cws/cws80_data_banks.cpp"
    "sources/cws/cws80_data_banks.h"
    "sources/cws/cws80_data.cpp"
    "sources/cws/cws80_data.h"
    "sources/cws/cws80_ins.cpp"
    "sources/cws/cws80_ins.h"
    "sources/cws/cws80_ins_midi.cpp"
    "sources/cws/cws80_ins_util.h"
    "sources/cws/cws80_ins_vcm.cpp"
    "sources/cws/cws80_messages.h"
    "sources/cws/cws80_program.cpp"
    "sources/cws/cws80_program.h"
    "sources/cws/cws80_program_parameters.h"
    "sources/cws/cws80_program_realfmt.cpp"
    "sources/cws/cws80_program_sq8lfmt.cpp"
    "sources/cws/cws80_program_textfmt.cpp"
    "sources/cws/component/dca4.cpp"
    "sources/cws/component/dca4.h"
    "sources/cws/component/dca.cpp"
    "sources/cws/component/dca.h"
    "sources/cws/component/env.cpp"
    "sources/cws/component/env.h"
    "sources/cws/component/lfo.cpp"
    "sources/cws/component/lfo.h"
    "sources/cws/component/osc.cpp"
    "sources/cws/component/osc.h"
    "sources/cws/component/sat.cpp"
    "sources/cws/component/sat.h"
    "sources/cws/component/tables.cpp"
    "sources/cws/component/tables.h"
    "sources/cws/component/vcf.cpp"
    "sources/cws/component/vcf.h"
    "sources/utility/arithmetic.h"
    "sources/utility/attributes.h"
    "sources/utility/container/bounded_vector.h"
    "sources/utility/c++std/optional.h"
    "sources/utility/c++std/string_view.h"
    "sources/utility/debug.h"
    "sources/utility/dynarray.h"
    "sources/utility/filter.h"
    "sources/utility/load_library.h"
    "sources/utility/path.cpp"
    "sources/utility/path.h"
    "sources/utility/pb_alloc.h"
    "sources/utility/scope_guard.h"
    "sources/utility/string.cpp"
    "sources/utility/string.h"
    "sources/utility/types.h")
target_include_directories(cws80_core
  PUBLIC
    "sources"
    "resources"
    "thirdparty/simpleini"
    "thirdparty/fmt/include"
    "thirdparty/gsl-lite/include"
    "thirdparty/optional-lite/include"
    "thirdparty/string-view-lite/include")
target_compile_definitions(cws80_core
  PUBLIC
    "FMT_HEADER_ONLY"
    "_USE_MATH_DEFINES")

###
dpf_add_plugin(cws80
  TARGETS lv2 vst2 jack
  UI_TYPE "${CWS80_GRAPHICS_DEVICE}"
  FILES_DSP
    "plugins/cws80/plugin.cpp"
    "plugins/cws80/plugin.h"
  FILES_UI
    "plugins/cws80/ui.cpp"
    "plugins/cws80/ui.h"
    "sources/library/compile_nuklear.cpp"
    "sources/library/compile_stb_image.cpp"
    "sources/ui/cws80_ui_controller.h"
    "sources/ui/cws80_ui.cpp"
    "sources/ui/cws80_ui_data.h"
    "sources/ui/cws80_ui.h"
    "sources/ui/cws80_ui_nk.cpp"
    "sources/ui/cws80_ui_nk.h"
    "sources/ui/cws80_ui_view.cpp"
    "sources/ui/cws80_ui_view.h"
    "sources/ui/detail/nk_draw.cpp"
    "sources/ui/detail/nk_draw.h"
    "sources/ui/detail/nk_essential.h"
    "sources/ui/detail/nkg_point.h"
    "sources/ui/detail/nkg_rect.h"
    "sources/ui/detail/nkg_util.h"
    "sources/ui/detail/nki_image.cpp"
    "sources/ui/detail/nki_image.h"
    "sources/ui/detail/nk_layout.cpp"
    "sources/ui/detail/nk_layout.h"
    "sources/ui/detail/nkw_button.cpp"
    "sources/ui/detail/nkw_button.h"
    "sources/ui/detail/nkw_common.cpp"
    "sources/ui/detail/nkw_common.h"
    "sources/ui/detail/nkw_led.cpp"
    "sources/ui/detail/nkw_led.h"
    "sources/ui/detail/nkw_piano.cpp"
    "sources/ui/detail/nkw_piano.h"
    "sources/ui/detail/nkw_placeholder.cpp"
    "sources/ui/detail/nkw_placeholder.h"
    "sources/ui/detail/nkw_select.cpp"
    "sources/ui/detail/nkw_select.h"
    "sources/ui/detail/nkw_slider.cpp"
    "sources/ui/detail/nkw_slider.h"
    "sources/ui/detail/ui_helpers_native.cpp"
    "sources/ui/detail/ui_helpers_native.h"
    "sources/ui/detail/ui_helpers_tk.cpp"
    "sources/ui/detail/ui_helpers_tk.h"
    "sources/ui/detail/device/dev_graphics.cpp"
    "sources/ui/detail/device/dev_graphics.h"
    "sources/ui/detail/device/dev_input_common.h"
    "sources/ui/detail/device/dev_input.cpp"
    "sources/ui/detail/device/dev_input_dpf.cpp"
    "sources/ui/detail/device/dev_input_dpf.h"
    "sources/ui/detail/device/dev_input.h"
    "sources/ui/detail/dynamic/tcltk.cpp"
    "sources/ui/detail/dynamic/tcltk.h"
  FILES_COMMON
    "thirdparty/ring_buffer/ring_buffer.cpp"
    "thirdparty/ring_buffer/ring_buffer.h"
    "thirdparty/ring_buffer/ring_buffer.tcc")
target_include_directories(cws80
  PUBLIC
    "plugins/cws80"
    "thirdparty/stb"
    "thirdparty/nuklear"
    "thirdparty/ring_buffer"
    "${TK_INCLUDE_PATH}"
    "${TCL_INCLUDE_PATH}")
target_compile_definitions(cws80
  PUBLIC
    "NK_INCLUDE_FIXED_TYPES"
    "NK_INCLUDE_STANDARD_IO"
    "NK_INCLUDE_DEFAULT_ALLOCATOR"
    "NK_INCLUDE_VERTEX_BUFFER_OUTPUT"
    "NK_INCLUDE_FONT_BAKING"
    "NK_INCLUDE_DEFAULT_FONT"
    "NK_ZERO_COMMAND_MEMORY"
    "STBI_MALLOC=malloc"
    "STBI_REALLOC=realloc"
    "STBI_FREE=free")
target_link_libraries(cws80
  PUBLIC
    cws80_core
    "${TK_LIBRARY}"
    "${TCL_LIBRARY}")

if(CWS80_GRAPHICS_DEVICE STREQUAL "cairo")
  target_sources(cws80-ui
    PRIVATE
      "sources/ui/detail/device/dev_graphics_cairo.cpp"
      "sources/ui/detail/device/dev_graphics_cairo.h")
  target_compile_definitions(cws80-ui
    PUBLIC
      "CWS80_UI_CAIRO")
elseif(CWS80_GRAPHICS_DEVICE STREQUAL "opengl")
  target_sources(cws80-ui
    PRIVATE
      "sources/ui/detail/device/dev_graphics_gl.cpp"
      "sources/ui/detail/device/dev_graphics_gl.h")
  target_compile_definitions(cws80-ui
    PUBLIC
      "CWS80_UI_OPENGL")
  target_link_libraries(cws80-ui
    PUBLIC
      GLEW::GLEW)
endif()

###
add_executable(cws80_tool
  "tools/sources/cws80_tool.cpp"
  "tools/sources/cws80_data_plot.cpp"
  "tools/sources/cws80_data_plot.h")
target_link_libraries(cws80_tool
  PUBLIC
    cws80_core)
