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

#ifndef WEBVIEW_DETAIL_MOUSE_HOOK_HH
#define WEBVIEW_DETAIL_MOUSE_HOOK_HH

#if defined(__cplusplus) && !defined(WEBVIEW_HEADER)

// The system-wide mouse hook behind webview_mouse_hook_install: one per
// process, on the UI thread's run loop, handing every mouse event with the
// modifier keys held to the caller's function and dropping the ones it says
// to swallow. Windows: a WH_MOUSE_LL hook. macOS: a listener on the
// CGEventTap in input_hooks.hh, which needs the permission the model there
// names (webview_feature_permissions). Linux: not supported.

#include "../errors.h"
#include "../errors.hh"
#include "../macros.h"
#include "../types.h"
#include "../types.hh"
#include "input_hooks.hh"

#if defined(WEBVIEW_PLATFORM_DARWIN)
#include <ApplicationServices/ApplicationServices.h>
#elif defined(WEBVIEW_PLATFORM_WINDOWS)
#include <windows.h>
#endif

namespace webview {
namespace detail {
namespace mouse_hook {

struct state_t {
  webview_mouse_hook_fn_t fn{};
  void *arg{};
#if defined(WEBVIEW_PLATFORM_WINDOWS)
  HHOOK hook{};
#endif
};

inline state_t &state() {
  static state_t s;
  return s;
}

#if defined(WEBVIEW_PLATFORM_DARWIN)

inline CGEventRef on_event(CGEventType type, CGEventRef event) {
  auto &s = state();
  if (!s.fn) {
    return event;
  }
  int kind = -1;
  double wheel = 0;
  switch (type) {
  case kCGEventMouseMoved:
  case kCGEventLeftMouseDragged:
  case kCGEventRightMouseDragged:
  case kCGEventOtherMouseDragged:
    kind = WEBVIEW_MOUSE_MOVE;
    break;
  case kCGEventLeftMouseDown:
    kind = WEBVIEW_MOUSE_LEFT_DOWN;
    break;
  case kCGEventLeftMouseUp:
    kind = WEBVIEW_MOUSE_LEFT_UP;
    break;
  case kCGEventRightMouseDown:
    kind = WEBVIEW_MOUSE_RIGHT_DOWN;
    break;
  case kCGEventRightMouseUp:
    kind = WEBVIEW_MOUSE_RIGHT_UP;
    break;
  case kCGEventOtherMouseDown:
  case kCGEventOtherMouseUp:
    // Button 2 is the middle one; the others are not the hook's business.
    if (CGEventGetIntegerValueField(event, kCGMouseEventButtonNumber) != 2) {
      return event;
    }
    kind = type == kCGEventOtherMouseDown ? WEBVIEW_MOUSE_MIDDLE_DOWN
                                          : WEBVIEW_MOUSE_MIDDLE_UP;
    break;
  case kCGEventScrollWheel:
    kind = WEBVIEW_MOUSE_WHEEL;
    // Axis 1 is vertical; the fixed-point delta is in lines (notches), with
    // fractions from a trackpad, positive when the content would scroll up,
    // which is the wheel turning away from the user.
    wheel = CGEventGetDoubleValueField(event,
                                       kCGScrollWheelEventFixedPtDeltaAxis1);
    break;
  default:
    return event;
  }
  // Global display coordinates: points, origin at the top-left of the main
  // display, which is the convention the library's callers use.
  CGPoint at = CGEventGetLocation(event);
  // The event's own flags: the modifiers as they were for this exact event.
  unsigned modifiers =
      input_hooks::modifiers_from_flags(CGEventGetFlags(event));
  return s.fn(kind, at.x, at.y, wheel, modifiers, s.arg) ? nullptr : event;
}

inline noresult install(webview_mouse_hook_fn_t fn, void *arg) {
  auto &s = state();
  if (s.fn) {
    return error_info{WEBVIEW_ERROR_DUPLICATE,
                      "A mouse hook is already installed"};
  }
  CGEventMask mask =
      CGEventMaskBit(kCGEventMouseMoved) |
      CGEventMaskBit(kCGEventLeftMouseDragged) |
      CGEventMaskBit(kCGEventRightMouseDragged) |
      CGEventMaskBit(kCGEventOtherMouseDragged) |
      CGEventMaskBit(kCGEventLeftMouseDown) |
      CGEventMaskBit(kCGEventLeftMouseUp) |
      CGEventMaskBit(kCGEventRightMouseDown) |
      CGEventMaskBit(kCGEventRightMouseUp) |
      CGEventMaskBit(kCGEventOtherMouseDown) |
      CGEventMaskBit(kCGEventOtherMouseUp) |
      CGEventMaskBit(kCGEventScrollWheel);
  s.fn = fn;
  s.arg = arg;
  auto added = input_hooks::event_tap::add(input_hooks::event_tap::mouse,
                                           mask, on_event);
  if (!added.ok()) {
    s = state_t{};
  }
  return added;
}

inline noresult remove() {
  auto &s = state();
  if (!s.fn) {
    return error_info{WEBVIEW_ERROR_NOT_FOUND, "No mouse hook is installed"};
  }
  input_hooks::event_tap::remove(input_hooks::event_tap::mouse);
  s = state_t{};
  return {};
}

#elif defined(WEBVIEW_PLATFORM_WINDOWS)

inline LRESULT CALLBACK hook_proc(int code, WPARAM wparam, LPARAM lparam) {
  auto &s = state();
  if (code >= 0 && lparam && s.fn) {
    const auto *m = reinterpret_cast<const MSLLHOOKSTRUCT *>(lparam);
    int kind = -1;
    double wheel = 0;
    switch (wparam) {
    case WM_MOUSEMOVE:
      kind = WEBVIEW_MOUSE_MOVE;
      break;
    case WM_LBUTTONDOWN:
      kind = WEBVIEW_MOUSE_LEFT_DOWN;
      break;
    case WM_LBUTTONUP:
      kind = WEBVIEW_MOUSE_LEFT_UP;
      break;
    case WM_RBUTTONDOWN:
      kind = WEBVIEW_MOUSE_RIGHT_DOWN;
      break;
    case WM_RBUTTONUP:
      kind = WEBVIEW_MOUSE_RIGHT_UP;
      break;
    case WM_MBUTTONDOWN:
      kind = WEBVIEW_MOUSE_MIDDLE_DOWN;
      break;
    case WM_MBUTTONUP:
      kind = WEBVIEW_MOUSE_MIDDLE_UP;
      break;
    case WM_MOUSEWHEEL:
      kind = WEBVIEW_MOUSE_WHEEL;
      // The high word of mouseData is the signed delta, WHEEL_DELTA per
      // notch, positive away from the user.
      wheel = static_cast<double>(static_cast<short>(HIWORD(m->mouseData))) /
              WHEEL_DELTA;
      break;
    default:
      break; // X buttons, the horizontal wheel: not the hook's business
    }
    // The low-level message has no modifier field; the asynchronous key
    // state inside the hook is the keyboard as it was for this event.
    if (kind >= 0 &&
        s.fn(kind, static_cast<double>(m->pt.x), static_cast<double>(m->pt.y),
             wheel, input_hooks::modifiers_now(), s.arg)) {
      return 1;
    }
  }
  return CallNextHookEx(nullptr, code, wparam, lparam);
}

inline noresult install(webview_mouse_hook_fn_t fn, void *arg) {
  auto &s = state();
  if (s.fn) {
    return error_info{WEBVIEW_ERROR_DUPLICATE,
                      "A mouse hook is already installed"};
  }
  s.fn = fn;
  s.arg = arg;
  s.hook = SetWindowsHookExW(WH_MOUSE_LL, hook_proc, GetModuleHandleW(nullptr),
                             0);
  if (!s.hook) {
    s = state_t{};
    return error_info{WEBVIEW_ERROR_UNSPECIFIED,
                      "SetWindowsHookEx(WH_MOUSE_LL) failed"};
  }
  return {};
}

inline noresult remove() {
  auto &s = state();
  if (!s.fn) {
    return error_info{WEBVIEW_ERROR_NOT_FOUND, "No mouse hook is installed"};
  }
  UnhookWindowsHookEx(s.hook);
  s = state_t{};
  return {};
}

#else

inline noresult install(webview_mouse_hook_fn_t /*fn*/, void * /*arg*/) {
  return error_info{WEBVIEW_ERROR_UNSPECIFIED,
                    "Mouse hooks are not supported on this platform"};
}

inline noresult remove() {
  return error_info{WEBVIEW_ERROR_NOT_FOUND, "No mouse hook is installed"};
}

#endif

} // namespace mouse_hook
} // namespace detail
} // namespace webview

#endif // defined(__cplusplus) && !defined(WEBVIEW_HEADER)
#endif // WEBVIEW_DETAIL_MOUSE_HOOK_HH
