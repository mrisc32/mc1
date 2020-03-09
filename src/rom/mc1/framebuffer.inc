; -*- mode: mr32asm; tab-width: 4; indent-tabs-mode: nil; -*-
;---------------------------------------------------------------------------------------------------
; Copyright (c) 2020 Marcus Geelnard
;
; This software is provided 'as-is', without any express or implied warranty. In no event will the
; authors be held liable for any damages arising from the use of this software.
;
; Permission is granted to anyone to use this software for any purpose, including commercial
; applications, and to alter it and redistribute it freely, subject to the following restrictions:
;
;  1. The origin of this software must not be misrepresented; you must not claim that you wrote
;     the original software. If you use this software in a product, an acknowledgment in the
;     product documentation would be appreciated but is not required.
;
;  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
;     being the original software.
;
;  3. This notice may not be removed or altered from any source distribution.
;---------------------------------------------------------------------------------------------------

; color_mode_t
MODE_RGBA8888 = 0
MODE_RGBA5551 = 1
MODE_PAL8 = 2
MODE_PAL4 = 3
MODE_PAL2 = 4
MODE_PAL1 = 5

; fb_t fields.
FB_PIXELS  = 0      ; void*
FB_VCP     = 4      ; void*
FB_PALETTE = 8      ; uint32_t*
FB_STRIDE  = 12     ; size_t
FB_WIDTH   = 16     ; int
FB_HEIGHT  = 20     ; int
FB_MODE    = 24     ; color_mode_t

