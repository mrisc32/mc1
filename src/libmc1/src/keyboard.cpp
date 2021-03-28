// -*- mode: c++; tab-width: 2; indent-tabs-mode: nil; -*-
//--------------------------------------------------------------------------------------------------
// Copyright (c) 2020 Marcus Geelnard
//
// This software is provided 'as-is', without any express or implied warranty. In no event will the
// authors be held liable for any damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose, including commercial
// applications, and to alter it and redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you must not claim that you wrote
//     the original software. If you use this software in a product, an acknowledgment in the
//     product documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and must not be misrepresented as
//     being the original software.
//
//  3. This notice may not be removed or altered from any source distribution.
//--------------------------------------------------------------------------------------------------

#include <mc1/keyboard.h>

#include <mc1/keyboard_layout.h>
#include <mc1/mmio.h>

namespace {

class keyboard_t {
public:
  void init();
  void poll();
  void set_layout(const uint32_t layout_id);

  uint32_t get_next_event();
  uint32_t event_to_char(const uint32_t event);
  bool is_pressed(const uint32_t scancode);

private:
  static const int NUM_KEYS = 512;
  static const unsigned FIFO_CAPACITY = 16;
  static const uint8_t KEY_PRESSED = 1;
  static const uint8_t KEY_RELEASED = 0;

  static uint16_t encode_event(const uint32_t keycode,
                               const bool has_shift,
                               const bool has_alt,
                               const bool has_altgr);

  bool fifo_is_empty() const {
    return m_fifo_size == 0u;
  }

  bool fifo_is_full() const {
    return m_fifo_size == FIFO_CAPACITY;
  }

  uint32_t m_keyptr;
  unsigned m_fifo_read_pos;
  unsigned m_fifo_size;
  uint16_t m_fifo[FIFO_CAPACITY];
  uint8_t m_keys[NUM_KEYS];
  const keyboard_layout_t* m_layout;
};

void keyboard_t::init() {
  m_keyptr = MMIO(KEYPTR);
  m_fifo_read_pos = 0u;
  m_fifo_size = 0u;
  for (unsigned i = 0u; i < FIFO_CAPACITY; ++i) {
    m_fifo[i] = 0u;
  }
  for (int i = 0; i < NUM_KEYS; ++i) {
    m_keys[i] = 0u;
  }

  // Default to EN_us layout.
  m_layout = &g_kb_layout_en_us;
}

void keyboard_t::poll() {
  // Check if we have any new keycode from the keyboard.
  const auto keyptr = MMIO(KEYPTR);
  while (m_keyptr != keyptr) {
    ++m_keyptr;
    if (!fifo_is_full()) {
      // Determine which modifiers are currently active.
      const auto has_shift = is_pressed(KB_LSHIFT) || is_pressed(KB_RSHIFT);
      const auto has_alt = is_pressed(KB_LALT) || is_pressed(KB_RALT);
      const auto has_ctrl = is_pressed(KB_LCTRL) || is_pressed(KB_RCTRL);

      // Encode the keyboard event.
      const auto keycode = KEYBUF(m_keyptr % KEYBUF_SIZE);
      const auto event = encode_event(keycode, has_shift, has_alt, has_ctrl);

      // Insert the new key event into the FIFO.
      const auto write_pos = (m_fifo_read_pos + m_fifo_size) % FIFO_CAPACITY;
      ++m_fifo_size;
      m_fifo[write_pos] = event;

      // Update the key map.
      m_keys[kb_event_scancode(event)] = kb_event_is_press(event) ? KEY_PRESSED : KEY_RELEASED;
    }
  }
}

void keyboard_t::set_layout(const uint32_t layout_id) {
  switch (layout_id) {
    case KB_LAYOUT_EN_US:
      m_layout = &g_kb_layout_en_us;
      break;
    default:
      break;
  }
}

uint32_t keyboard_t::get_next_event() {
  if (fifo_is_empty()) {
    return 0u;
  }
  const uint32_t event = m_fifo[m_fifo_read_pos];
  m_fifo_read_pos = (m_fifo_read_pos + 1u) % FIFO_CAPACITY;
  --m_fifo_size;
  return event;
}

uint32_t keyboard_t::event_to_char(const uint32_t event) {
  const auto scancode = kb_event_scancode(event);
  if (scancode == 0u || scancode > 127u) {
    return 0u;
  }

  // Look up the character from the keyboard mapping.
  const auto& layout_entry = (*m_layout)[scancode];
  return kb_event_has_shift(event) ? layout_entry.shifted : layout_entry.normal;
}

bool keyboard_t::is_pressed(const uint32_t scancode) {
  return (scancode < NUM_KEYS) ? (m_keys[scancode] == KEY_PRESSED) : false;
}

uint16_t keyboard_t::encode_event(const uint32_t keycode,
                                  const bool has_shift,
                                  const bool has_alt,
                                  const bool has_ctrl) {
  // Determine the scan code.
  auto scancode = keycode & 0x1ffu;

  // Special cases: Map some high PS/2 scancodes to a lower 7-bit representation (to enable more
  // compact keyboard layout tables).
  // TODO(m): Should this be handled in hardware instead? Rationale: The MMIO KEYCODE register
  // should not really be tied to the PS/2 protocol, since we expect the encoding to be the same for
  // USB keyboards. Thus we expect the hardware interface to do transcoding to our virtual scancodes
  // anyway.
  if (scancode == 0x14au) {
    scancode = KB_KP_DIV;
  } else if (scancode == 0x15au) {
    scancode = KB_KP_ENTER;
  }

  // Determine event attributes.
  const auto release = (static_cast<int32_t>(keycode) > 0) ? 0x200u : 0u;
  const auto shift_mod = has_shift ? 0x400u : 0u;
  const auto alt_mod = has_alt ? 0x800u : 0u;
  const auto ctrl_mod = has_ctrl ? 0x1000u : 0u;

  // Combine scancode and attributes into a single 16-bit word.
  return static_cast<uint16_t>(scancode | release | shift_mod | alt_mod | ctrl_mod);
}

keyboard_t s_keyboard;

}  // namespace

extern "C" void kb_init() {
  s_keyboard.init();
}

extern "C" void kb_poll() {
  s_keyboard.poll();
}

extern "C" void kb_set_layout(const uint32_t layout_id) {
  s_keyboard.set_layout(layout_id);
}

extern "C" uint32_t kb_get_next_event() {
  return s_keyboard.get_next_event();
}

extern "C" uint32_t kb_event_to_char(const uint32_t event) {
  return s_keyboard.event_to_char(event);
}

extern "C" int kb_is_pressed(const uint32_t scancode) {
  return s_keyboard.is_pressed(scancode) ? 1 : 0;
}
