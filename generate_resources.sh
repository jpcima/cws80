#!/bin/bash
set -e

compile_resource() {
    if test "$#" -ne 2; then
        echo "Invalid number of arguments"
        false
        return
    fi
    local out="$1"
    local in="$2"
    tools/bin2c.py -o "$out" -i "$in"
}

compile_resource resources/sqdata/sq80lorom.dat.h resources/sq80/sq80rom.low
compile_resource resources/sqdata/sq80hirom.dat.h resources/sq80/sq80rom.hig
compile_resource resources/sqdata/wave2202.dat.h resources/sq80/2202.bin
compile_resource resources/sqdata/wave2203.dat.h resources/sq80/2203.bin
compile_resource resources/sqdata/wave2204.dat.h resources/sq80/2204.bin
compile_resource resources/sqdata/wave2205.dat.h resources/sq80/2205.bin
compile_resource resources/sqdata/initprogram.dat.h resources/init/init-program.dat
compile_resource resources/sqdata/extrabank1.dat.h resources/banks/esq_1.mdx
compile_resource resources/sqdata/extrabank2.dat.h resources/banks/esq_2.mdx
compile_resource resources/sqdata/extrabank3.dat.h resources/banks/esq_3.mdx
compile_resource resources/sqdata/extrabank4.dat.h resources/banks/esq_4.mdx
compile_resource resources/sqdata/extrabank5.dat.h resources/banks/esq_5.mdx
compile_resource resources/sqdata/extrabank6.dat.h resources/banks/hackera.mdx
compile_resource resources/sqdata/extrabank7.dat.h resources/banks/hackerb.mdx
compile_resource resources/sqdata/extrabank8.dat.h resources/banks/hackerc.mdx
compile_resource resources/sqdata/extrabank9.dat.h resources/banks/hackerd.mdx
compile_resource resources/sqdata/extrabank10.dat.h resources/banks/hackere.mdx
compile_resource resources/sqdata/extrabank11.dat.h resources/banks/hackerf.mdx

compile_resource resources/ui/data/fnt_proggy_clean.dat.h resources/fonts/ProggyClean.ttf
compile_resource resources/ui/data/fnt_seg14.dat.h resources/fonts/LCD14Italic.ttf
compile_resource resources/ui/data/fnt_scp_black_italic.dat.h resources/fonts/SourceCodePro-BlackIt.ttf
compile_resource resources/ui/data/sk_slider.dat.h resources/ui/slider-skin.png
compile_resource resources/ui/data/sk_knob.dat.h resources/ui/knob-skin.png
compile_resource resources/ui/data/sk_button.dat.h resources/ui/button-skin.png
compile_resource resources/ui/data/sk_tiny_button.dat.h resources/ui/tiny-button-skin.png
compile_resource resources/ui/data/sk_led_on.dat.h resources/ui/led-on.png
compile_resource resources/ui/data/sk_led_off.dat.h resources/ui/led-off.png
compile_resource resources/ui/data/im_logo.dat.h resources/ui/logo.png
compile_resource resources/ui/data/im_diagram.dat.h resources/ui/diagram.png
compile_resource resources/ui/data/im_background.dat.h resources/ui/background.png
compile_resource resources/ui/data/im_background2.dat.h resources/ui/background2.png
compile_resource resources/ui/data/tk_getString.dat.h resources/ui/tklib/tk_getString.tcl
