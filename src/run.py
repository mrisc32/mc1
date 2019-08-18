#!/usr/bin/env python3
import os
import struct
import sys
from vunit import VUnit

sys.path.insert(1, os.path.join(sys.path[0], '../tools/vcpas'))
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

    # ...and all the DUT:s.
    lib.add_source_files("rtl/video.vhd")
    lib.add_source_files("rtl/vid_palette.vhd")
    lib.add_source_files("rtl/vid_pixel.vhd")
    lib.add_source_files("rtl/vid_raster.vhd")
    lib.add_source_files("rtl/vid_regs.vhd")
    lib.add_source_files("rtl/vid_types.vhd")
    lib.add_source_files("rtl/vid_vcpp.vhd")
    lib.add_source_files("rtl/vid_vcpp_stack.vhd")

    # Bake the test data.
    bake_video_tb_vram()

    # Run vunit function
    vu.main()


if __name__ == '__main__':
    main()
