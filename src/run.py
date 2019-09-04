#!/usr/bin/env python3
from vunit import VUnit

def main():
    # Create VUnit instance by parsing command line arguments
    vu = VUnit.from_argv()
    
    # Create library 'lib' containing all the test benches...
    lib = vu.add_library("lib")
    lib.add_source_files("test/*_tb.vhd")

    # ...and all the DUT:s.
    lib.add_source_files("rtl/vid_vcpp.vhd")
    lib.add_source_files("rtl/vid_vcpp_stack.vhd")

    # Run vunit function
    vu.main()


if __name__ == '__main__':
    main()
