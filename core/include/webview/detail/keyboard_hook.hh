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
// which needs the Accessibility permission. Linux: not supported.

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
  /// not yet up: a second down of a held key is the OS's auto-repeat on
  /// Windows, and on macOS tells a modifier's flags-changed down from its up.
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

/// The flag a modifier key's flags-changed event toggles, by keycode; 0 for
/// a key that is not a modifier.
inline CGEventFlags flag_for_keycode(unsigned keycode) {
  switch (keycode) {
  case 0x38: // left shift
  case 0x3C: // right shift
    return kCGEventFlagMaskShift;
  case 0x3B: // left control
  case 0x3E: // right control
    return kCGEventFlagMaskControl;
  case 0x3A: // left option
  case 0x3D: // right option
    return kCGEventFlagMaskAlternate;
  case 0x37: // left command
  case 0x36: // right command
    return kCGEventFlagMaskCommand;
  case 0x39: // caps lock
    return kCGEventFlagMaskAlphaShift;
  case 0x3F: // fn
    return kCGEventFlagMaskSecondaryFn;
  default:
    return 0;
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
    // flag it toggles says up when clear; when set, the key is going down
    // unless the hook saw it go down already, in which case this is its up
    // while the key's other side still holds the flag. Caps Lock's flag is
    // the lock state, so its down turns the lock on and its up turns it off.
    CGEventFlags flag = flag_for_keycode(key);
    bool set = flag && (CGEventGetFlags(event) & flag);
    if (set && !s.down.test(key)) {
      kind = WEBVIEW_KEY_DOWN;
      s.down.set(key);
    } else {
      kind = WEBVIEW_KEY_UP;
      s.down.reset(key);
    }
    break;
  }
  default:
    return event;
  }
  unsigned modifiers =
      input_hooks::modifiers_from_flags(CGEventGetFlags(event));
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
/// SendInput with only a virtual key); the virtual key then names it.
inline unsigned scan_of(const KBDLLHOOKSTRUCT &k) {
  unsigned scan = k.scanCode & 0xFF;
  bool extended = (k.flags & LLKHF_EXTENDED) != 0;
  if (scan == 0) {
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
    case VK_SNAPSHOT:
      extended = true;
      break;
    default:
      break;
    }
  }
  return scan | (extended ? 0xE000u : 0u);
}

inline LRESULT CALLBACK hook_proc(int code, WPARAM wparam, LPARAM lparam) {
  auto &s = state();
  if (code >= 0 && lparam && s.fn) {
    const auto &k = *reinterpret_cast<const KBDLLHOOKSTRUCT *>(lparam);
    auto vk = k.vkCode & 0xFF;
    bool is_down = wparam == WM_KEYDOWN || wparam == WM_SYSKEYDOWN;
    int kind;
    if (is_down) {
      // The low-level message has no repeat count; a down for a key the
      // hook saw go down and not yet up is the OS's auto-repeat.
      kind = s.down.test(vk) ? WEBVIEW_KEY_REPEAT : WEBVIEW_KEY_DOWN;
      s.down.set(vk);
    } else {
      kind = WEBVIEW_KEY_UP;
      s.down.reset(vk);
    }
    const char *dom_code = key_codes::dom_code_from_windows_scan(scan_of(k));
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
