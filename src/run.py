#!/usr/bin/env python3
import os
import struct
import sys
from vunit import VUnit

sys.path.insert(1, os.path.join(sys.path[0], 'mc1-sdk/tools'))
import vcpas


_VIDEO_TB_VCP_SOURCE = "test/test-image-640x360-pal8.vcp"
_VIDEO_TB_VRAM_FILE = "vunit_out/video_tb_ram.bin"


def bake_video_tb_vram():
    # Assemble the VCP.
    vcpas.assemble(_VIDEO_TB_VCP_SOURCE, _VIDEO_TB_VRAM_FILE, "bin")


def main():
    # Create VUnit instance by parsing command line arguments
    vu = VUnit.from_argv()
    
    # Create library 'lib' containing all the test benches...
    lib = vu.add_library("lib")
    lib.add_source_files("test/*_tb.vhd")

    # Add simulation models.
    lib.add_source_files("test/sdram_model.vhd")

    # Add the MC1 design.
    lib.add_source_files("rtl/bit_synchronizer.vhd")
    lib.add_source_files("rtl/dither.vhd")
    lib.add_source_files("rtl/mc1.vhd")
    lib.add_source_files("rtl/mmio_types.vhd")
    lib.add_source_files("rtl/mmio.vhd")
    lib.add_source_files("rtl/prng.vhd")
    lib.add_source_files("rtl/ps2_keyboard.vhd")
    lib.add_source_files("rtl/ps2_receiver.vhd")
    lib.add_source_files("rtl/ram_true_dual_port.vhd")
    lib.add_source_files("rtl/reset_conditioner.vhd")
    lib.add_source_files("rtl/reset_stabilizer.vhd")
    lib.add_source_files("rtl/sdram.vhd")
    lib.add_source_files("rtl/sdram_controller.vhd")
    lib.add_source_files("rtl/synchronizer.vhd")
    lib.add_source_files("rtl/vid_blend.vhd")
    lib.add_source_files("rtl/video_layer.vhd")
    lib.add_source_files("rtl/video.vhd")
    lib.add_source_files("rtl/vid_palette.vhd")
    lib.add_source_files("rtl/vid_pixel.vhd")
    lib.add_source_files("rtl/vid_pix_prefetch.vhd")
    lib.add_source_files("rtl/vid_raster.vhd")
    lib.add_source_files("rtl/vid_regs.vhd")
    lib.add_source_files("rtl/vid_types.vhd")
    lib.add_source_files("rtl/vid_vcpp_stack.vhd")
    lib.add_source_files("rtl/vid_vcpp.vhd")
    lib.add_source_files("rtl/vram.vhd")
    lib.add_source_files("rtl/wb_crossbar_2x4.vhd")
    lib.add_source_files("rtl/xram_sdram.vhd")

    # Add the MC1 boot ROM (must be generated with "make").
    lib.add_source_files("rom/out/rom.vhd")

    # Add the MRISC32-A1 implementation.
    mrisc32 = vu.add_library("mrisc32")
    mrisc32.add_source_files("mrisc32-a1/rtl/agu/*.vhd")
    mrisc32.add_source_files("mrisc32-a1/rtl/alu/*.vhd")
    mrisc32.add_source_files("mrisc32-a1/rtl/common/*.vhd")
    mrisc32.add_source_files("mrisc32-a1/rtl/core/*.vhd")
    mrisc32.add_source_files("mrisc32-a1/rtl/fpu/*.vhd")
    mrisc32.add_source_files("mrisc32-a1/rtl/muldiv/*.vhd")
    mrisc32.add_source_files("mrisc32-a1/rtl/pipeline/*.vhd")
    mrisc32.add_source_files("mrisc32-a1/rtl/sau/*.vhd")

    # Bake the video_tb test data.
    bake_video_tb_vram()

    # Run vunit function
    vu.main()


if __name__ == '__main__':
    main()
