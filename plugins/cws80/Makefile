#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------
# Project name, used for binaries

NAME = cws80
UI_TYPE = cairo

# --------------------------------------------------------------
# Files to build

FILES_DSP = \
	plugin.cpp \
	sources/cws/cws80_data.cpp \
	sources/cws/cws80_data_banks.cpp \
	sources/cws/cws80_ins.cpp \
	sources/cws/cws80_ins_vcm.cpp \
	sources/cws/cws80_ins_midi.cpp \
	sources/cws/cws80_program.cpp \
	sources/cws/cws80_program_realfmt.cpp \
	sources/cws/cws80_program_textfmt.cpp \
	sources/cws/component/dca4.cpp \
	sources/cws/component/dca.cpp \
	sources/cws/component/env.cpp \
	sources/cws/component/lfo.cpp \
	sources/cws/component/osc.cpp \
	sources/cws/component/sat.cpp \
	sources/cws/component/tables.cpp \
	sources/cws/component/vcf.cpp \
	sources/utility/path.cpp \
	sources/utility/string.cpp \
	thirdparty/ring_buffer/ring_buffer.cpp

FILES_UI = \
	ui.cpp \
	sources/ui/cws80_ui.cpp \
	sources/ui/cws80_ui_nk.cpp \
	sources/ui/cws80_ui_view.cpp \
	sources/ui/detail/nk_draw.cpp \
	sources/ui/detail/nk_layout.cpp \
	sources/ui/detail/nki_image.cpp \
	sources/ui/detail/nkw_button.cpp \
	sources/ui/detail/nkw_common.cpp \
	sources/ui/detail/nkw_led.cpp \
	sources/ui/detail/nkw_piano.cpp \
	sources/ui/detail/nkw_placeholder.cpp \
	sources/ui/detail/nkw_select.cpp \
	sources/ui/detail/nkw_slider.cpp \
	sources/ui/detail/ui_helpers_native.cpp \
	sources/ui/detail/ui_helpers_tk.cpp \
	sources/ui/detail/device/dev_input.cpp \
	sources/ui/detail/device/dev_input_dpf.cpp \
	sources/ui/detail/device/dev_graphics.cpp \
	sources/ui/detail/dynamic/tcltk.cpp \
	sources/cws/cws80_data.cpp \
	sources/cws/cws80_data_banks.cpp \
	sources/cws/cws80_program.cpp \
	sources/cws/cws80_program_realfmt.cpp \
	sources/cws/cws80_program_textfmt.cpp \
	sources/cws/component/dca4.cpp \
	sources/cws/component/dca.cpp \
	sources/cws/component/env.cpp \
	sources/cws/component/lfo.cpp \
	sources/cws/component/osc.cpp \
	sources/cws/component/sat.cpp \
	sources/cws/component/tables.cpp \
	sources/cws/component/vcf.cpp \
	sources/utility/path.cpp \
	sources/utility/string.cpp \
	sources/library/compile_nuklear.cpp \
	sources/library/compile_stb_image.cpp \
	thirdparty/ring_buffer/ring_buffer.cpp

ifeq ($(UI_TYPE),opengl)
FILES_UI += \
	sources/ui/detail/device/dev_graphics_gl.cpp
endif

ifeq ($(UI_TYPE),cairo)
FILES_UI += \
	sources/ui/detail/device/dev_graphics_cairo.cpp
endif

# --------------------------------------------------------------
# Do some magic

include ../../dpf/Makefile.plugins.mk

# --------------------------------------------------------------
#

BUILD_CXX_FLAGS += -std=c++14
BUILD_CXX_FLAGS += -fopenmp
BUILD_CXX_FLAGS += -Isources
BUILD_CXX_FLAGS += -Iresources
BUILD_CXX_FLAGS += -Ithirdparty/stb
BUILD_CXX_FLAGS += -Ithirdparty/nuklear
BUILD_CXX_FLAGS += -Ithirdparty/ring_buffer
BUILD_CXX_FLAGS += -Ithirdparty/fmt/include
BUILD_CXX_FLAGS += -Ithirdparty/gsl-lite/include

BUILD_CXX_FLAGS += -DNK_INCLUDE_FIXED_TYPES
BUILD_CXX_FLAGS += -DNK_INCLUDE_STANDARD_IO
BUILD_CXX_FLAGS += -DNK_INCLUDE_DEFAULT_ALLOCATOR
BUILD_CXX_FLAGS += -DNK_INCLUDE_VERTEX_BUFFER_OUTPUT
BUILD_CXX_FLAGS += -DNK_INCLUDE_FONT_BAKING
BUILD_CXX_FLAGS += -DNK_INCLUDE_DEFAULT_FONT
BUILD_CXX_FLAGS += -DNK_ZERO_COMMAND_MEMORY
BUILD_CXX_FLAGS += -DSTBI_MALLOC=malloc
BUILD_CXX_FLAGS += -DSTBI_REALLOC=realloc
BUILD_CXX_FLAGS += -DSTBI_FREE=free
BUILD_CXX_FLAGS += -DFMT_HEADER_ONLY

# release+debug flags
#BUILD_CXX_FLAGS += -g -UNDEBUG

ifeq ($(UI_TYPE),opengl)
$(if $(shell pkg-config --exists glew && echo 1),,$(error cannot find GLEW (libglew-dev)))
BUILD_CXX_FLAGS += $(shell pkg-config --cflags glew)
LINK_FLAGS += $(shell pkg-config --libs glew)
endif

# find Tcl/Tk
TCLTK_PATHS := \
	/usr/$(shell $(CC) -dumpmachine)/lib \
	/usr/lib \
	/usr/local/lib \
	/opt/local/lib
TCL_CONFIG := $(firstword $(foreach x,$(TCLTK_PATHS),$(if $(shell test -f $(x)/tclConfig.sh && echo 1),$(x)/tclConfig.sh)))
TK_CONFIG := $(firstword $(foreach x,$(TCLTK_PATHS),$(if $(shell test -f $(x)/tkConfig.sh && echo 1),$(x)/tkConfig.sh)))
$(if $(TCL_CONFIG),,$(error cannot find tclConfig.sh (tcl-dev)))
$(if $(TK_CONFIG),,$(error cannot find tkConfig.sh (tk-dev)))

BUILD_CXX_FLAGS += -DUSE_TCL_STUBS -DUSE_TK_STUBS
BUILD_CXX_FLAGS += $(shell . $(TCL_CONFIG) && echo $$TCL_INCLUDE_SPEC)
BUILD_CXX_FLAGS += $(shell . $(TK_CONFIG) && echo $$TK_INCLUDE_SPEC)
LINK_FLAGS += $(shell . $(TCL_CONFIG) && echo $$TCL_STUB_LIB_SPEC)
LINK_FLAGS += $(shell . $(TK_CONFIG) && echo $$TK_STUB_LIB_SPEC)

ifeq ($(LINUX),true)
LINK_FLAGS += -ldl
endif

# --------------------------------------------------------------
# Enable all possible plugin types

ifeq ($(HAVE_JACK),true)
TARGETS += jack
endif

# change to lv2_sep when instance-access is removed
TARGETS += lv2

TARGETS += vst

all: $(TARGETS)

# --------------------------------------------------------------
