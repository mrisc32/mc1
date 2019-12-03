----------------------------------------------------------------------------------------------------
-- Copyright (c) 2019 Marcus Geelnard
--
-- This software is provided 'as-is', without any express or implied warranty. In no event will the
-- authors be held liable for any damages arising from the use of this software.
--
-- Permission is granted to anyone to use this software for any purpose, including commercial
-- applications, and to alter it and redistribute it freely, subject to the following restrictions:
--
--  1. The origin of this software must not be misrepresented; you must not claim that you wrote
--     the original software. If you use this software in a product, an acknowledgment in the
--     product documentation would be appreciated but is not required.
--
--  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
--     being the original software.
--
--  3. This notice may not be removed or altered from any source distribution.
----------------------------------------------------------------------------------------------------

----------------------------------------------------------------------------------------------------
-- This file contains types definitions for MMIO registers.
----------------------------------------------------------------------------------------------------

library ieee;
use ieee.std_logic_1164.all;

package mmio_types is
  subtype T_MMIO_REG_WORD is std_logic_vector(31 downto 0);

  --------------------------------------------------------------------------------------------------
  -- Read-only registers.
  --------------------------------------------------------------------------------------------------
  type T_MMIO_REGS_RO is record
    -- MC1 internal registers.
    CLKCNTLO : T_MMIO_REG_WORD;    -- CPU clock cycle count - lo bits (free running counter).
    CLKCNTHI : T_MMIO_REG_WORD;    -- CPU clock cycle count - hi bits (free running counter).
    CPUCLK : T_MMIO_REG_WORD;      -- CPU clock frequency in Hz.
    VRAMSIZE : T_MMIO_REG_WORD;    -- VRAM size in bytes.
    XRAMSIZE : T_MMIO_REG_WORD;    -- Extended RAM size in bytes.
    VIDWIDTH : T_MMIO_REG_WORD;    -- Native video resoltuion, width.
    VIDHEIGHT : T_MMIO_REG_WORD;   -- Native video resoltuion, height.
    VIDFPS : T_MMIO_REG_WORD;      -- Video refresh rate in 1000 * frames per s.
    VIDFRAMENO : T_MMIO_REG_WORD;  -- Video frame number (free running counter).

    -- External registers.
    -- TODO(m): microSD inputs, PS/2 inputs, GPIO inputs.
    SWITCHES : T_MMIO_REG_WORD;    -- Switches (one bit per switch, active high).
    BUTTONS : T_MMIO_REG_WORD;     -- Buttons (one bit per button, active high).
  end record T_MMIO_REGS_RO;

  --------------------------------------------------------------------------------------------------
  -- Write-only registers.
  --------------------------------------------------------------------------------------------------
  type T_MMIO_REGS_WO is record
    -- MC1 internal registers.
    -- TODO(m): Add something here?

    -- External registers.
    -- TODO(m): microSD outputs, GPIO outputs.
    SEGDISP0 : T_MMIO_REG_WORD;    -- Segmented display 0 (one bit per segment, active high).
    SEGDISP1 : T_MMIO_REG_WORD;    -- Segmented display 1 (one bit per segment, active high).
    SEGDISP2 : T_MMIO_REG_WORD;    -- Segmented display 2 (one bit per segment, active high).
    SEGDISP3 : T_MMIO_REG_WORD;    -- Segmented display 3 (one bit per segment, active high).
    SEGDISP4 : T_MMIO_REG_WORD;    -- Segmented display 4 (one bit per segment, active high).
    SEGDISP5 : T_MMIO_REG_WORD;    -- Segmented display 5 (one bit per segment, active high).
    SEGDISP6 : T_MMIO_REG_WORD;    -- Segmented display 6 (one bit per segment, active high).
    SEGDISP7 : T_MMIO_REG_WORD;    -- Segmented display 7 (one bit per segment, active high).
    LEDS : T_MMIO_REG_WORD;        -- LED:s (one bit per LED, active high).
  end record T_MMIO_REGS_WO;
end package;
