# MC1 boot sequence

The MC1 first boots from ROM, which does minimal system initialization. This includes:

* Set up a small (512 bytes) stack in RAM.
* Initialize the boot medium device.

After the initialization, the ROM boot process will try to load the boot block from a boot medium (e.g. an SD card). The block is loaded into the upper part of VRAM. Since the size of VRAM may vary, the position of the loaded boot block is not fixed. Hence, *the boot code must be position independent* (all addressing must be PC-relative).

After successfully loading the boot code, the ROM code calls the boot code (the first word of the boot code is the call target).

When the boot code is called, it is passed the following information in registers:

| Register | Contents |
|---|---|
| S1 | ROM base (see "ROM function table" below) |

Once the boot code has been called, it is not expected that it will return. All system control is handed over to the loaded boot code.

## Boot block layout

The size of the boot block is 512 bytes, and it is stored in block 0 of the boot medium.

All integers are stored as 32-bit little endian words.

| Offset | Type | Name | Description |
|---|---|---|---|
| 0 | word | type | ID: 0x4231434d ("MC1B") |
| 4 | word | checksum | crc32c checksum of the code (bytes 8..511) |
| 8 | word[126] | code | 126 instruction/data words |

## ROM function table

The ROM function table is an array of callable routines that can be useful for the boot code, as follows:

| Table offset | Name | Signature |
|---|---|---|
| 0 | doh | noreturn void doh(const char* message) |
| 4 | blk_read | int blk_read(void* ptr, int device, size_t first_block, size_t num_block) |
| 8 | crc32c | unsigned crc32c(void* ptr, size_t num_bytes) |
| 12 | LZG_Decode | unsigned LZG_Decode(const void* in, unsigned insize, void* out, unsigned outsize) |

To call a function, jump and link to the address rom_base+offset, e.g:

```
  mov s26, s1     ; s26 = rom_base
  ...
  jl  s26, #4     ; Call blk_read()
```

## Typical boot code

It is expected that the boot code will continue to load further code and data from the boot medium and pass over control to the newly loaded program. For instance, the loaded program may use its own memory allocator and implement full file system support (whereas the ROM only provides primitive block reading functionality).

Here is a simple example that loads a larger piece of code into memory and executes it:

```
  ; ROM routine table offsets.
  DOH = 0
  BLK_READ = 4
  CRC32C = 8
  LZG_DECODE = 12

  ; Program address and location on the boot medium.
  PRG_ADDRESS = 0x40000200
  START_BLOCK = 1
  NUM_BLOCKS = 78

  .section .text.start, "ax"
  .globl  _boot
  .p2align 2

_boot:
  ; Store the ROM jump table address in s26.
  mov     s26, s1

  ; Read the program blocks into VRAM.
  ldi     s16, #PRG_ADDRESS
  mov     s1, s16
  ldi     s2, #0
  ldi     s3, #START_BLOCK
  ldi     s4, #NUM_BLOCKS
  jl      s26, #BLK_READ
  bz      s1, fail

  ; Jump to the loaded code (start the program).
  j       s16, #0

fail:
  ldi     s1, #msg@pc
  j       s26, #DOH        ; doh (msg) !

msg:
  .asciz  "Failed to load MyProgram"
```
