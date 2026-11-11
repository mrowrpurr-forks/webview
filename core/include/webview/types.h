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

#ifndef WEBVIEW_TYPES_H
#define WEBVIEW_TYPES_H

/// Holds the elements of a MAJOR.MINOR.PATCH version number.
typedef struct {
  /// Major version.
  unsigned int major;
  /// Minor version.
  unsigned int minor;
  /// Patch version.
  unsigned int patch;
} webview_version_t;

/// Holds the library's version information.
typedef struct {
  /// The elements of the version number.
  webview_version_t version;
  /// SemVer 2.0.0 version number in MAJOR.MINOR.PATCH format.
  char version_number[32];
  /// SemVer 2.0.0 pre-release labels prefixed with "-" if specified, otherwise
  /// an empty string.
  char pre_release[48];
  /// SemVer 2.0.0 build metadata prefixed with "+", otherwise an empty string.
  char build_metadata[48];
} webview_version_info_t;

/// Pointer to a webview instance.
typedef void *webview_t;

/// Native handle kind. The actual type depends on the backend.
typedef enum {
  /// Top-level window. @c GtkWindow pointer (GTK), @c NSWindow pointer (Cocoa)
  /// or @c HWND (Win32).
  WEBVIEW_NATIVE_HANDLE_KIND_UI_WINDOW,
  /// Browser widget. @c GtkWidget pointer (GTK), @c NSView pointer (Cocoa) or
  /// @c HWND (Win32).
  WEBVIEW_NATIVE_HANDLE_KIND_UI_WIDGET,
  /// Browser controller. @c WebKitWebView pointer (WebKitGTK), @c WKWebView
  /// pointer (Cocoa/WebKit) or @c ICoreWebView2Controller pointer
  /// (Win32/WebView2).
  WEBVIEW_NATIVE_HANDLE_KIND_BROWSER_CONTROLLER
} webview_native_handle_kind_t;

/// Window size hints
typedef enum {
  /// Width and height are default size.
  WEBVIEW_HINT_NONE,
  /// Width and height are minimum bounds.
  WEBVIEW_HINT_MIN,
  /// Width and height are maximum bounds.
  WEBVIEW_HINT_MAX,
  /// Window size can not be changed by a user.
  WEBVIEW_HINT_FIXED
} webview_hint_t;

/// The modifier keys held when an input hook saw an event, as bits of the
/// @c modifiers argument of @c webview_mouse_hook_fn_t and
/// @c webview_keyboard_hook_fn_t. The same four the DOM's KeyboardEvent
/// reports (shiftKey, ctrlKey, altKey, metaKey); lock keys are not modifiers.
typedef enum {
  WEBVIEW_MODIFIER_SHIFT = 1 << 0,
  WEBVIEW_MODIFIER_CONTROL = 1 << 1,
  /// Alt on Windows and Linux, Option on macOS.
  WEBVIEW_MODIFIER_ALT = 1 << 2,
  /// The Windows key on Windows, Command on macOS.
  WEBVIEW_MODIFIER_META = 1 << 3
} webview_modifier_t;

/// What a mouse hook saw (webview_mouse_hook_install). Any button but the
/// left, right and middle, and a horizontal wheel, never reach the hook.
typedef enum {
  /// The cursor moved, with or without a button held.
  WEBVIEW_MOUSE_MOVE = 0,
  WEBVIEW_MOUSE_LEFT_DOWN = 1,
  WEBVIEW_MOUSE_LEFT_UP = 2,
  WEBVIEW_MOUSE_RIGHT_DOWN = 3,
  WEBVIEW_MOUSE_RIGHT_UP = 4,
  WEBVIEW_MOUSE_MIDDLE_DOWN = 5,
  WEBVIEW_MOUSE_MIDDLE_UP = 6,
  /// The vertical wheel turned; the callback's @c wheel is the amount.
  WEBVIEW_MOUSE_WHEEL = 7
} webview_mouse_event_t;

/**
 * Called by the mouse hook for every mouse event on the system, before any
 * window gets it.
 *
 * @param event A @c webview_mouse_event_t.
 * @param x Cursor position on the screen, in the platform's screen units
 *          (physical pixels on Windows, points on macOS), origin at the
 *          top-left corner of the primary display.
 * @param y See @p x.
 * @param wheel For @c WEBVIEW_MOUSE_WHEEL the amount turned in notches,
 *              positive away from the user (a trackpad gives fractions);
 *              0 otherwise.
 * @param modifiers The modifier keys held when the event was generated, as
 *                  @c webview_modifier_t bits: on Windows the asynchronous key
 *                  state inside the hook, on macOS the event's own flags.
 * @param arg The @c arg given to @c webview_mouse_hook_install.
 * @return Non-zero to swallow the event, so that no window receives it; zero
 *         to let it go on to whatever is under the cursor.
 */
typedef int (*webview_mouse_hook_fn_t)(int event, double x, double y,
                                       double wheel, unsigned modifiers,
                                       void *arg);

/// What a keyboard hook saw (webview_keyboard_hook_install).
typedef enum {
  /// A key went down that was up.
  WEBVIEW_KEY_DOWN = 0,
  /// A key went up.
  WEBVIEW_KEY_UP = 1,
  /// A key that is held sent another down (the OS's auto-repeat).
  WEBVIEW_KEY_REPEAT = 2
} webview_key_event_t;

/**
 * Called by the keyboard hook for every key event on the system, before any
 * window gets it.
 *
 * @param event A @c webview_key_event_t.
 * @param code The physical key as the DOM's KeyboardEvent.code names it
 *             ("KeyA", "Digit1", "Enter", "ArrowLeft", "ShiftLeft",
 *             "Numpad7", "F5"...), so a caller never switches on a platform's
 *             key numbers; "Unidentified" for a key the library has no name
 *             for (@p raw still says which). Valid for the duration of the
 *             call only.
 * @param raw The platform's own code for the key: the virtual-key code on
 *            Windows (VK_*), the virtual keycode on macOS (kVK_*).
 * @param modifiers The modifier keys held after this event, as
 *                  @c webview_modifier_t bits, so a modifier's own down has
 *                  its bit set and its up has it clear (the DOM's convention).
 * @param arg The @c arg given to @c webview_keyboard_hook_install.
 * @return Non-zero to swallow the event, so that no window receives it; zero
 *         to let it go on to the focused window.
 */
typedef int (*webview_keyboard_hook_fn_t)(int event, const char *code,
                                          unsigned raw, unsigned modifiers,
                                          void *arg);

#endif // WEBVIEW_TYPES_H
