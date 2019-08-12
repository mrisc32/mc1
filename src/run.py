#!/usr/bin/env python3
from vunit import VUnit

def main():
    # Create VUnit instance by parsing command line arguments
    vu = VUnit.from_argv()
    
    # Create library 'lib'
    lib = vu.add_library("lib")
    
    # Add all files ending in _tb.vhd in current working directory to library
    lib.add_source_files("test/*_tb.vhd")
    
    # Run vunit function
    vu.main()


if __name__ == '__main__':
    main()
