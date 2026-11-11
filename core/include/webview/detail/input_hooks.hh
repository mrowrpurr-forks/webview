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

#ifndef WEBVIEW_DETAIL_INPUT_HOOKS_HH
#define WEBVIEW_DETAIL_INPUT_HOOKS_HH

#if defined(__cplusplus) && !defined(WEBVIEW_HEADER)

// What the mouse hook (mouse_hook.hh) and the keyboard hook
// (keyboard_hook.hh) share: reading the modifier keys into
// webview_modifier_t bits, and on macOS the one CGEventTap both hooks listen
// through, so the process asks for the Accessibility permission once and
// adds one source to the run loop.

#include "../errors.h"
#include "../errors.hh"
#include "../macros.h"
#include "../types.h"
#include "../types.hh"

#if defined(WEBVIEW_PLATFORM_DARWIN)
#include <ApplicationServices/ApplicationServices.h>
#elif defined(WEBVIEW_PLATFORM_WINDOWS)
#include <windows.h>
#endif

namespace webview {
namespace detail {
namespace input_hooks {

#if defined(WEBVIEW_PLATFORM_WINDOWS)

/// Whether a key is physically down right now, by the asynchronous key state:
/// a low-level hook runs before the input reaches any thread's queue, so the
/// synchronous (per-thread) state is stale.
inline bool key_down(int vk) {
  return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

/// The modifier keys held right now, as webview_modifier_t bits.
inline unsigned modifiers_now() {
  unsigned m = 0;
  if (key_down(VK_SHIFT)) {
    m |= WEBVIEW_MODIFIER_SHIFT;
  }
  if (key_down(VK_CONTROL)) {
    m |= WEBVIEW_MODIFIER_CONTROL;
  }
  if (key_down(VK_MENU)) {
    m |= WEBVIEW_MODIFIER_ALT;
  }
  if (key_down(VK_LWIN) || key_down(VK_RWIN)) {
    m |= WEBVIEW_MODIFIER_META;
  }
  return m;
}

/// The modifier keys held after a key event, as webview_modifier_t bits. The
/// asynchronous state does not yet include the event the hook is looking at,
/// so the event's own key is taken from the event: its down sets the bit and
/// its up clears it, unless the other key of the pair is still held.
inline unsigned modifiers_after_key(unsigned vk, bool down) {
  // A generic code (VK_SHIFT) stands for either side; an injected event may
  // carry one.
  auto held = [&](int left, int right, int generic) {
    auto side = [&](int side_vk) {
      if (vk == static_cast<unsigned>(side_vk) ||
          vk == static_cast<unsigned>(generic)) {
        return down;
      }
      return key_down(side_vk);
    };
    return side(left) || side(right);
  };
  unsigned m = 0;
  if (held(VK_LSHIFT, VK_RSHIFT, VK_SHIFT)) {
    m |= WEBVIEW_MODIFIER_SHIFT;
  }
  if (held(VK_LCONTROL, VK_RCONTROL, VK_CONTROL)) {
    m |= WEBVIEW_MODIFIER_CONTROL;
  }
  if (held(VK_LMENU, VK_RMENU, VK_MENU)) {
    m |= WEBVIEW_MODIFIER_ALT;
  }
  if (held(VK_LWIN, VK_RWIN, 0)) {
    m |= WEBVIEW_MODIFIER_META;
  }
  return m;
}

#elif defined(WEBVIEW_PLATFORM_DARWIN)

/// An event's flags as webview_modifier_t bits.
inline unsigned modifiers_from_flags(CGEventFlags flags) {
  unsigned m = 0;
  if (flags & kCGEventFlagMaskShift) {
    m |= WEBVIEW_MODIFIER_SHIFT;
  }
  if (flags & kCGEventFlagMaskControl) {
    m |= WEBVIEW_MODIFIER_CONTROL;
  }
  if (flags & kCGEventFlagMaskAlternate) {
    m |= WEBVIEW_MODIFIER_ALT;
  }
  if (flags & kCGEventFlagMaskCommand) {
    m |= WEBVIEW_MODIFIER_META;
  }
  return m;
}

// The one event tap. Each hook is a listener: the event types it wants and
// a function that gets each matching event and returns it to pass it on or
// nullptr to swallow it. The tap is created when the first listener arrives
// with the union of the listeners' masks, recreated when the set changes (a
// tap's mask is fixed at creation), and destroyed when the last one leaves.
namespace event_tap {

using listener_fn_t = CGEventRef (*)(CGEventType type, CGEventRef event);

enum listener_id { mouse = 0, keyboard = 1, listener_count = 2 };

struct listener_t {
  CGEventMask mask{};
  listener_fn_t fn{};
};

struct state_t {
  listener_t listeners[listener_count]{};
  CFMachPortRef tap{};
  CFRunLoopSourceRef source{};
};

inline state_t &state() {
  static state_t s;
  return s;
}

inline CGEventRef callback(CGEventTapProxy /*proxy*/, CGEventType type,
                           CGEventRef event, void *refcon) {
  auto &s = *static_cast<state_t *>(refcon);
  // The system turns a tap off when its callback was too slow or on user
  // input it judged urgent; turn it back on, or the hooks are silently dead.
  if (type == kCGEventTapDisabledByTimeout ||
      type == kCGEventTapDisabledByUserInput) {
    if (s.tap) {
      CGEventTapEnable(s.tap, true);
    }
    return event;
  }
  for (auto &l : s.listeners) {
    if (l.fn && (l.mask & CGEventMaskBit(type))) {
      return l.fn(type, event);
    }
  }
  return event;
}

inline void destroy() {
  auto &s = state();
  if (!s.tap) {
    return;
  }
  CGEventTapEnable(s.tap, false);
  CFRunLoopRemoveSource(CFRunLoopGetMain(), s.source, kCFRunLoopCommonModes);
  CFMachPortInvalidate(s.tap);
  CFRelease(s.source);
  CFRelease(s.tap);
  s.tap = nullptr;
  s.source = nullptr;
}

/// Creates the tap for the given mask on the main run loop (the one
/// webview_run spins); false when the system refuses, which is the
/// Accessibility permission missing.
inline bool create(CGEventMask mask) {
  auto &s = state();
  // A default (not listen-only) tap can drop events; that takes the
  // Accessibility permission, and without it the tap is not created.
  s.tap = CGEventTapCreate(kCGSessionEventTap, kCGHeadInsertEventTap,
                           kCGEventTapOptionDefault, mask, callback, &s);
  if (!s.tap) {
    return false;
  }
  s.source = CFMachPortCreateRunLoopSource(kCFAllocatorDefault, s.tap, 0);
  CFRunLoopAddSource(CFRunLoopGetMain(), s.source, kCFRunLoopCommonModes);
  CGEventTapEnable(s.tap, true);
  return true;
}

inline CGEventMask combined_mask() {
  CGEventMask mask = 0;
  for (auto &l : state().listeners) {
    if (l.fn) {
      mask |= l.mask;
    }
  }
  return mask;
}

/// Asks the system to show its Accessibility prompt (it shows once per
/// process); the caller retries on a later launch, after the user granted it.
inline void prompt_for_accessibility() {
  const void *keys[] = {kAXTrustedCheckOptionPrompt};
  const void *values[] = {kCFBooleanTrue};
  CFDictionaryRef options = CFDictionaryCreate(
      kCFAllocatorDefault, keys, values, 1, &kCFTypeDictionaryKeyCallBacks,
      &kCFTypeDictionaryValueCallBacks);
  AXIsProcessTrustedWithOptions(options);
  CFRelease(options);
}

/// Adds a listener and rebuilds the tap for the new set. On failure the
/// listener is not added and the tap is put back as it was.
inline noresult add(listener_id id, CGEventMask mask, listener_fn_t fn) {
  auto &s = state();
  CGEventMask before = combined_mask();
  s.listeners[id] = listener_t{mask, fn};
  destroy();
  if (create(combined_mask())) {
    return {};
  }
  s.listeners[id] = listener_t{};
  if (before) {
    create(before); // the other hook keeps running
  }
  prompt_for_accessibility();
  return error_info{WEBVIEW_ERROR_MISSING_DEPENDENCY,
                    "CGEventTapCreate failed: the process needs the "
                    "Accessibility permission (System Settings > Privacy & "
                    "Security > Accessibility)"};
}

/// Removes a listener and rebuilds the tap for what is left, or destroys it
/// when nothing is.
inline void remove(listener_id id) {
  auto &s = state();
  s.listeners[id] = listener_t{};
  destroy();
  CGEventMask mask = combined_mask();
  if (mask) {
    create(mask);
  }
}

} // namespace event_tap

#endif

} // namespace input_hooks
} // namespace detail
} // namespace webview

#endif // defined(__cplusplus) && !defined(WEBVIEW_HEADER)
#endif // WEBVIEW_DETAIL_INPUT_HOOKS_HH
