#!/usr/bin/make -f
# Makefile for DISTRHO Plugins #
# ---------------------------- #
# Created by falkTX
#

# --------------------------------------------------------------

# Disable stripping by default
export SKIP_STRIPPING ?= true

# --------------------------------------------------------------

include dpf/Makefile.base.mk

PLUGINS := cws80

all: dgl plugins gen

# --------------------------------------------------------------

dgl:
	$(MAKE) -C dpf/dgl

plugins: dgl
	$(foreach p,$(PLUGINS),$(MAKE) all -C plugins/$(p);)

ifneq ($(CROSS_COMPILING),true)
gen: plugins dpf/utils/lv2_ttl_generator
	@$(CURDIR)/dpf/utils/generate-ttl.sh
ifeq ($(MACOS),true)
	@$(CURDIR)/dpf/utils/generate-vst-bundles.sh
endif

dpf/utils/lv2_ttl_generator:
	$(MAKE) -C dpf/utils/lv2-ttl-generator
else
gen:
endif

# --------------------------------------------------------------

RESOURCES :=

define compile_resource
$(1): $(2)
	@mkdir -p $(dir $(1))
	tools/bin2c.ros -o $$@ -i $$<
RESOURCES := $(RESOURCES) $(1)
endef

$(eval $(call compile_resource,resources/sqdata/sq80lorom.dat.h,resources/sq80/sq80rom.low))
$(eval $(call compile_resource,resources/sqdata/sq80hirom.dat.h,resources/sq80/sq80rom.hig))
$(eval $(call compile_resource,resources/sqdata/wave2202.dat.h,resources/sq80/2202.bin))
$(eval $(call compile_resource,resources/sqdata/wave2203.dat.h,resources/sq80/2203.bin))
$(eval $(call compile_resource,resources/sqdata/wave2204.dat.h,resources/sq80/2204.bin))
$(eval $(call compile_resource,resources/sqdata/wave2205.dat.h,resources/sq80/2205.bin))
$(eval $(call compile_resource,resources/sqdata/initprogram.dat.h,resources/init/init-program.dat))
$(eval $(call compile_resource,resources/sqdata/extrabank1.dat.h,resources/banks/esq_1.mdx))
$(eval $(call compile_resource,resources/sqdata/extrabank2.dat.h,resources/banks/esq_2.mdx))
$(eval $(call compile_resource,resources/sqdata/extrabank3.dat.h,resources/banks/esq_3.mdx))
$(eval $(call compile_resource,resources/sqdata/extrabank4.dat.h,resources/banks/esq_4.mdx))
$(eval $(call compile_resource,resources/sqdata/extrabank5.dat.h,resources/banks/esq_5.mdx))
$(eval $(call compile_resource,resources/sqdata/extrabank6.dat.h,resources/banks/hackera.mdx))
$(eval $(call compile_resource,resources/sqdata/extrabank7.dat.h,resources/banks/hackerb.mdx))
$(eval $(call compile_resource,resources/sqdata/extrabank8.dat.h,resources/banks/hackerc.mdx))
$(eval $(call compile_resource,resources/sqdata/extrabank9.dat.h,resources/banks/hackerd.mdx))
$(eval $(call compile_resource,resources/sqdata/extrabank10.dat.h,resources/banks/hackere.mdx))
$(eval $(call compile_resource,resources/sqdata/extrabank11.dat.h,resources/banks/hackerf.mdx))

$(eval $(call compile_resource,resources/ui/data/fnt_proggy_clean.dat.h,resources/fonts/ProggyClean.ttf))
$(eval $(call compile_resource,resources/ui/data/fnt_seg14.dat.h,resources/fonts/LCD14Italic.ttf))
$(eval $(call compile_resource,resources/ui/data/fnt_scp_black_italic.dat.h,resources/fonts/SourceCodePro-BlackIt.ttf))
$(eval $(call compile_resource,resources/ui/data/sk_slider.dat.h,resources/ui/slider-skin.png))
$(eval $(call compile_resource,resources/ui/data/sk_knob.dat.h,resources/ui/knob-skin.png))
$(eval $(call compile_resource,resources/ui/data/sk_button.dat.h,resources/ui/button-skin.png))
$(eval $(call compile_resource,resources/ui/data/sk_tiny_button.dat.h,resources/ui/tiny-button-skin.png))
$(eval $(call compile_resource,resources/ui/data/sk_led_on.dat.h,resources/ui/led-on.png))
$(eval $(call compile_resource,resources/ui/data/sk_led_off.dat.h,resources/ui/led-off.png))
$(eval $(call compile_resource,resources/ui/data/im_logo.dat.h,resources/ui/logo.png))
$(eval $(call compile_resource,resources/ui/data/im_diagram.dat.h,resources/ui/diagram.png))
$(eval $(call compile_resource,resources/ui/data/im_background.dat.h,resources/ui/background.png))
$(eval $(call compile_resource,resources/ui/data/im_background2.dat.h,resources/ui/background2.png))
$(eval $(call compile_resource,resources/ui/data/tk_getString.dat.h,resources/ui/tklib/tk_getString.tcl))

resources: $(RESOURCES)
.PHONY: resources

# --------------------------------------------------------------

clean:
	$(MAKE) clean -C dpf/dgl
	$(MAKE) clean -C dpf/utils/lv2-ttl-generator
	$(foreach p,$(PLUGINS),$(MAKE) clean -C plugins/$(p);)
	rm -rf bin build
	rm -f *.d *.o core/*.d core/*.o

# --------------------------------------------------------------

.PHONY: plugins
