/*
 * MIT License
 *
 * Copyright (c) 2017 Serge Zaitsev
 * Copyright (c) 2022 Steffen André Langnes
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef WEBVIEW_DETAIL_KEYBOARD_HOOK_HH
#define WEBVIEW_DETAIL_KEYBOARD_HOOK_HH

#if defined(__cplusplus) && !defined(WEBVIEW_HEADER)

// The system-wide keyboard hook behind webview_keyboard_hook_install: one
// per process, on the UI thread's run loop, handing every key event (down,
// up, auto-repeat) with its DOM code and the modifier keys held to the
// caller's function and dropping the ones it says to swallow. Windows: a
// WH_KEYBOARD_LL hook. macOS: a listener on the CGEventTap in input_hooks.hh,
// which needs the permission the model there names
// (webview_feature_permissions). Linux: not supported.

#include "../errors.h"
#include "../errors.hh"
#include "../macros.h"
#include "../types.h"
#include "../types.hh"
#include "input_hooks.hh"
#include "key_codes.hh"

#include <bitset>

#if defined(WEBVIEW_PLATFORM_DARWIN)
#include <ApplicationServices/ApplicationServices.h>
#elif defined(WEBVIEW_PLATFORM_WINDOWS)
#include <windows.h>
#endif

namespace webview {
namespace detail {
namespace keyboard_hook {

struct state_t {
  webview_keyboard_hook_fn_t fn{};
  void *arg{};
  /// Which keys (by the platform's raw code) the hook has seen go down and
  /// not yet up: on Windows a second down of a held key is the OS's
  /// auto-repeat; on macOS it tells a synthesized modifier's flags-changed
  /// down from its up.
  std::bitset<256> down{};
#if defined(WEBVIEW_PLATFORM_WINDOWS)
  HHOOK hook{};
#endif
};

inline state_t &state() {
  static state_t s;
  return s;
}

#if defined(WEBVIEW_PLATFORM_DARWIN)

/// A modifier key's flags, by keycode: the flag it toggles (what callers
/// read), and the per-side bit the hardware sets in the same flags word
/// (IOLLEvent.h's NX_DEVICE*KEYMASK), 0 for Caps Lock and Fn, which have no
/// sides.
struct modifier_flags_t {
  CGEventFlags flag;
  CGEventFlags side;
  CGEventFlags both_sides;
};

inline modifier_flags_t modifier_flags_for_keycode(unsigned keycode) {
  const CGEventFlags lshift = 0x02, rshift = 0x04, lctrl = 0x01,
                     rctrl = 0x2000, lalt = 0x20, ralt = 0x40, lcmd = 0x08,
                     rcmd = 0x10;
  switch (keycode) {
  case 0x38:
    return {kCGEventFlagMaskShift, lshift, lshift | rshift};
  case 0x3C:
    return {kCGEventFlagMaskShift, rshift, lshift | rshift};
  case 0x3B:
    return {kCGEventFlagMaskControl, lctrl, lctrl | rctrl};
  case 0x3E:
    return {kCGEventFlagMaskControl, rctrl, lctrl | rctrl};
  case 0x3A:
    return {kCGEventFlagMaskAlternate, lalt, lalt | ralt};
  case 0x3D:
    return {kCGEventFlagMaskAlternate, ralt, lalt | ralt};
  case 0x37:
    return {kCGEventFlagMaskCommand, lcmd, lcmd | rcmd};
  case 0x36:
    return {kCGEventFlagMaskCommand, rcmd, lcmd | rcmd};
  case 0x39:
    return {kCGEventFlagMaskAlphaShift, 0, 0};
  case 0x3F:
    return {kCGEventFlagMaskSecondaryFn, 0, 0};
  default:
    return {0, 0, 0};
  }
}

inline CGEventRef on_event(CGEventType type, CGEventRef event) {
  auto &s = state();
  if (!s.fn) {
    return event;
  }
  auto keycode = static_cast<unsigned>(
      CGEventGetIntegerValueField(event, kCGKeyboardEventKeycode));
  auto key = keycode & 0xFF;
  CGEventFlags flags = CGEventGetFlags(event);
  int kind = -1;
  switch (type) {
  case kCGEventKeyDown:
    kind = CGEventGetIntegerValueField(event, kCGKeyboardEventAutorepeat)
               ? WEBVIEW_KEY_REPEAT
               : WEBVIEW_KEY_DOWN;
    s.down.set(key);
    break;
  case kCGEventKeyUp:
    kind = WEBVIEW_KEY_UP;
    s.down.reset(key);
    break;
  case kCGEventFlagsChanged: {
    // A modifier key's press and release both arrive as flags-changed. The
    // hardware sets a bit per side, which says whether this side is down
    // whatever the other side does. An event without side bits is
    // synthesized (CGEventPost with flags set by hand): its flag clear is
    // the up, and its flag set is the down unless the hook saw this key go
    // down already. Caps Lock's flag is the lock state, so its down turns
    // the lock on and its up turns it off. A flags-changed for a key that
    // is no modifier (a remapper's, with keycode 0) is nothing to report.
    modifier_flags_t m = modifier_flags_for_keycode(key);
    if (!m.flag) {
      return event;
    }
    bool is_down;
    if (m.both_sides && (flags & m.both_sides)) {
      is_down = (flags & m.side) != 0;
    } else if (!(flags & m.flag)) {
      is_down = false;
    } else {
      is_down = !s.down.test(key);
    }
    kind = is_down ? WEBVIEW_KEY_DOWN : WEBVIEW_KEY_UP;
    s.down.set(key, is_down);
    break;
  }
  default:
    return event;
  }
  unsigned modifiers = input_hooks::modifiers_from_flags(flags);
  return s.fn(kind, key_codes::dom_code_from_mac_keycode(key), keycode,
              modifiers, s.arg)
             ? nullptr
             : event;
}

inline noresult install(webview_keyboard_hook_fn_t fn, void *arg) {
  auto &s = state();
  if (s.fn) {
    return error_info{WEBVIEW_ERROR_DUPLICATE,
                      "A keyboard hook is already installed"};
  }
  CGEventMask mask = CGEventMaskBit(kCGEventKeyDown) |
                     CGEventMaskBit(kCGEventKeyUp) |
                     CGEventMaskBit(kCGEventFlagsChanged);
  s.fn = fn;
  s.arg = arg;
  s.down.reset();
  auto added = input_hooks::event_tap::add(
      input_hooks::event_tap::keyboard, mask, on_event);
  if (!added.ok()) {
    s = state_t{};
  }
  return added;
}

inline noresult remove() {
  auto &s = state();
  if (!s.fn) {
    return error_info{WEBVIEW_ERROR_NOT_FOUND,
                      "No keyboard hook is installed"};
  }
  input_hooks::event_tap::remove(input_hooks::event_tap::keyboard);
  s = state_t{};
  return {};
}

#elif defined(WEBVIEW_PLATFORM_WINDOWS)

/// The key's scan code with 0xE000 added for an extended key, which is how
/// key_codes names it. Injected input may carry no scan code at all (a
/// SendInput with only a virtual key); the virtual key then names it. A
/// Unicode character injected as VK_PACKET has no physical key: 0, which
/// key_codes names "Unidentified".
inline unsigned scan_of(const KBDLLHOOKSTRUCT &k) {
  if (k.vkCode == VK_PACKET) {
    return 0;
  }
  unsigned scan = k.scanCode & 0xFF;
  bool extended = (k.flags & LLKHF_EXTENDED) != 0;
  if (scan == 0) {
    switch (k.vkCode) {
    case VK_PAUSE:
      return 0x0045; // MapVirtualKey says E1 1D, the key's prefix, not E0 45
    case VK_SNAPSHOT:
      return 0xE037; // MapVirtualKey says 54, the Alt+PrintScreen code
    default:
      break;
    }
    unsigned mapped = MapVirtualKeyW(k.vkCode, MAPVK_VK_TO_VSC_EX);
    scan = mapped & 0xFF;
    extended = (mapped & 0xFF00) == 0xE000;
    // MapVirtualKey's answer for the navigation cluster is the numpad's
    // code without the extended prefix; the virtual key says which it is.
    switch (k.vkCode) {
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_LEFT:
    case VK_UP:
    case VK_RIGHT:
    case VK_DOWN:
    case VK_NUMLOCK:
    case VK_RCONTROL:
    case VK_RMENU:
    case VK_LWIN:
    case VK_RWIN:
    case VK_APPS:
    case VK_DIVIDE:
      extended = true;
      break;
    default:
      break;
    }
  }
  return scan | (extended ? 0xE000u : 0u);
}

/// The Shift events the keyboard makes up around a numpad key pressed with
/// Shift under Num Lock (scan 2A or 36 with the extended prefix): no key
/// went down, so the hook does not report them.
inline bool is_fake_shift(unsigned scan) {
  return scan == 0xE02A || scan == 0xE036;
}

inline LRESULT CALLBACK hook_proc(int code, WPARAM wparam, LPARAM lparam) {
  auto &s = state();
  if (code >= 0 && lparam && s.fn) {
    const auto &k = *reinterpret_cast<const KBDLLHOOKSTRUCT *>(lparam);
    unsigned scan = scan_of(k);
    if (is_fake_shift(scan)) {
      return CallNextHookEx(nullptr, code, wparam, lparam);
    }
    auto vk = k.vkCode & 0xFF;
    bool is_down = wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN;
    int kind;
    if (is_down) {
      // The low-level message has no repeat count; a down for a key the
      // hook saw go down and not yet up is the OS's auto-repeat, provided
      // the key is still held: an up the hook never saw (the secure
      // desktop, another hook) must not turn the next press into a repeat.
      kind = s.down.test(vk) && input_hooks::key_down(static_cast<int>(vk))
                 ? WEBVIEW_KEY_REPEAT
                 : WEBVIEW_KEY_DOWN;
      s.down.set(vk);
    } else {
      kind = WEBVIEW_KEY_UP;
      s.down.reset(vk);
    }
    const char *dom_code = key_codes::dom_code_from_windows_scan(scan);
    unsigned modifiers = input_hooks::modifiers_after_key(k.vkCode, is_down);
    if (s.fn(kind, dom_code, k.vkCode, modifiers, s.arg)) {
      return 1;
    }
  }
  return CallNextHookEx(nullptr, code, wparam, lparam);
}

inline noresult install(webview_keyboard_hook_fn_t fn, void *arg) {
  auto &s = state();
  if (s.fn) {
    return error_info{WEBVIEW_ERROR_DUPLICATE,
                      "A keyboard hook is already installed"};
  }
  s.fn = fn;
  s.arg = arg;
  s.down.reset();
  s.hook = SetWindowsHookExW(WH_KEYBOARD_LL, hook_proc,
                             GetModuleHandleW(nullptr), 0);
  if (!s.hook) {
    s = state_t{};
    return error_info{WEBVIEW_ERROR_UNSPECIFIED,
                      "SetWindowsHookEx(WH_KEYBOARD_LL) failed"};
  }
  return {};
}

inline noresult remove() {
  auto &s = state();
  if (!s.fn) {
    return error_info{WEBVIEW_ERROR_NOT_FOUND,
                      "No keyboard hook is installed"};
  }
  UnhookWindowsHookEx(s.hook);
  s = state_t{};
  return {};
}

#else

inline noresult install(webview_keyboard_hook_fn_t /*fn*/, void * /*arg*/) {
  return error_info{WEBVIEW_ERROR_UNSPECIFIED,
                    "Keyboard hooks are not supported on this platform"};
}

inline noresult remove() {
  return error_info{WEBVIEW_ERROR_NOT_FOUND, "No keyboard hook is installed"};
}

#endif

} // namespace keyboard_hook
} // namespace detail
} // namespace webview

#endif // defined(__cplusplus) && !defined(WEBVIEW_HEADER)
#endif // WEBVIEW_DETAIL_KEYBOARD_HOOK_HH
