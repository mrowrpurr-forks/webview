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

#ifndef WEBVIEW_DETAIL_KEY_CODES_HH
#define WEBVIEW_DETAIL_KEY_CODES_HH

#if defined(__cplusplus) && !defined(WEBVIEW_HEADER)

// The platform's physical key numbers to the DOM's KeyboardEvent.code names
// (https://www.w3.org/TR/uievents-code/), which is what the keyboard hook
// hands its caller. Compiled on every platform so the tables are unit-tested
// where the hook itself is not available. The values follow Chromium's
// ui/events/keycodes/dom/dom_code_data.inc, the de facto reference.

namespace webview {
namespace detail {
namespace key_codes {

/// The name the DOM gives a key it has no name for.
constexpr const char *unidentified = "Unidentified";

/// Windows: a keyboard scan code (set 1 make code, the low byte of
/// KBDLLHOOKSTRUCT::scanCode) with 0xE000 added when the key is extended
/// (LLKHF_EXTENDED), to its DOM code. Unknown codes give @c unidentified.
inline const char *dom_code_from_windows_scan(unsigned scan) {
  switch (scan) {
  case 0x0001:
    return "Escape";
  case 0x0002:
    return "Digit1";
  case 0x0003:
    return "Digit2";
  case 0x0004:
    return "Digit3";
  case 0x0005:
    return "Digit4";
  case 0x0006:
    return "Digit5";
  case 0x0007:
    return "Digit6";
  case 0x0008:
    return "Digit7";
  case 0x0009:
    return "Digit8";
  case 0x000A:
    return "Digit9";
  case 0x000B:
    return "Digit0";
  case 0x000C:
    return "Minus";
  case 0x000D:
    return "Equal";
  case 0x000E:
    return "Backspace";
  case 0x000F:
    return "Tab";
  case 0x0010:
    return "KeyQ";
  case 0x0011:
    return "KeyW";
  case 0x0012:
    return "KeyE";
  case 0x0013:
    return "KeyR";
  case 0x0014:
    return "KeyT";
  case 0x0015:
    return "KeyY";
  case 0x0016:
    return "KeyU";
  case 0x0017:
    return "KeyI";
  case 0x0018:
    return "KeyO";
  case 0x0019:
    return "KeyP";
  case 0x001A:
    return "BracketLeft";
  case 0x001B:
    return "BracketRight";
  case 0x001C:
    return "Enter";
  case 0x001D:
    return "ControlLeft";
  case 0x001E:
    return "KeyA";
  case 0x001F:
    return "KeyS";
  case 0x0020:
    return "KeyD";
  case 0x0021:
    return "KeyF";
  case 0x0022:
    return "KeyG";
  case 0x0023:
    return "KeyH";
  case 0x0024:
    return "KeyJ";
  case 0x0025:
    return "KeyK";
  case 0x0026:
    return "KeyL";
  case 0x0027:
    return "Semicolon";
  case 0x0028:
    return "Quote";
  case 0x0029:
    return "Backquote";
  case 0x002A:
    return "ShiftLeft";
  case 0x002B:
    return "Backslash";
  case 0x002C:
    return "KeyZ";
  case 0x002D:
    return "KeyX";
  case 0x002E:
    return "KeyC";
  case 0x002F:
    return "KeyV";
  case 0x0030:
    return "KeyB";
  case 0x0031:
    return "KeyN";
  case 0x0032:
    return "KeyM";
  case 0x0033:
    return "Comma";
  case 0x0034:
    return "Period";
  case 0x0035:
    return "Slash";
  case 0x0036:
    return "ShiftRight";
  case 0x0037:
    return "NumpadMultiply";
  case 0x0038:
    return "AltLeft";
  case 0x0039:
    return "Space";
  case 0x003A:
    return "CapsLock";
  case 0x003B:
    return "F1";
  case 0x003C:
    return "F2";
  case 0x003D:
    return "F3";
  case 0x003E:
    return "F4";
  case 0x003F:
    return "F5";
  case 0x0040:
    return "F6";
  case 0x0041:
    return "F7";
  case 0x0042:
    return "F8";
  case 0x0043:
    return "F9";
  case 0x0044:
    return "F10";
  case 0x0045:
    return "Pause"; // NumLock is the same code with the extended flag
  case 0x0046:
    return "ScrollLock";
  case 0x0047:
    return "Numpad7";
  case 0x0048:
    return "Numpad8";
  case 0x0049:
    return "Numpad9";
  case 0x004A:
    return "NumpadSubtract";
  case 0x004B:
    return "Numpad4";
  case 0x004C:
    return "Numpad5";
  case 0x004D:
    return "Numpad6";
  case 0x004E:
    return "NumpadAdd";
  case 0x004F:
    return "Numpad1";
  case 0x0050:
    return "Numpad2";
  case 0x0051:
    return "Numpad3";
  case 0x0052:
    return "Numpad0";
  case 0x0053:
    return "NumpadDecimal";
  case 0x0054:
    return "PrintScreen"; // Alt+PrintScreen (SysRq)
  case 0x0056:
    return "IntlBackslash";
  case 0x0057:
    return "F11";
  case 0x0058:
    return "F12";
  case 0x0059:
    return "NumpadEqual";
  case 0x0064:
    return "F13";
  case 0x0065:
    return "F14";
  case 0x0066:
    return "F15";
  case 0x0067:
    return "F16";
  case 0x0068:
    return "F17";
  case 0x0069:
    return "F18";
  case 0x006A:
    return "F19";
  case 0x006B:
    return "F20";
  case 0x006C:
    return "F21";
  case 0x006D:
    return "F22";
  case 0x006E:
    return "F23";
  case 0x0070:
    return "KanaMode";
  case 0x0071:
    return "Lang2";
  case 0x0072:
    return "Lang1";
  case 0x0073:
    return "IntlRo";
  case 0x0076:
    return "F24";
  case 0x0079:
    return "Convert";
  case 0x007B:
    return "NonConvert";
  case 0x007D:
    return "IntlYen";
  case 0x007E:
    return "NumpadComma";
  case 0xE010:
    return "MediaTrackPrevious";
  case 0xE019:
    return "MediaTrackNext";
  case 0xE01C:
    return "NumpadEnter";
  case 0xE01D:
    return "ControlRight";
  case 0xE020:
    return "AudioVolumeMute";
  case 0xE021:
    return "LaunchApp2";
  case 0xE022:
    return "MediaPlayPause";
  case 0xE024:
    return "MediaStop";
  case 0xE02E:
    return "AudioVolumeDown";
  case 0xE030:
    return "AudioVolumeUp";
  case 0xE032:
    return "BrowserHome";
  case 0xE035:
    return "NumpadDivide";
  case 0xE037:
    return "PrintScreen";
  case 0xE038:
    return "AltRight";
  case 0xE045:
    return "NumLock";
  case 0xE046:
    return "Pause"; // Ctrl+Pause (Break)
  case 0xE047:
    return "Home";
  case 0xE048:
    return "ArrowUp";
  case 0xE049:
    return "PageUp";
  case 0xE04B:
    return "ArrowLeft";
  case 0xE04D:
    return "ArrowRight";
  case 0xE04F:
    return "End";
  case 0xE050:
    return "ArrowDown";
  case 0xE051:
    return "PageDown";
  case 0xE052:
    return "Insert";
  case 0xE053:
    return "Delete";
  case 0xE05B:
    return "MetaLeft";
  case 0xE05C:
    return "MetaRight";
  case 0xE05D:
    return "ContextMenu";
  case 0xE05E:
    return "Power";
  case 0xE05F:
    return "Sleep";
  case 0xE063:
    return "WakeUp";
  case 0xE065:
    return "BrowserSearch";
  case 0xE066:
    return "BrowserFavorites";
  case 0xE067:
    return "BrowserRefresh";
  case 0xE068:
    return "BrowserStop";
  case 0xE069:
    return "BrowserForward";
  case 0xE06A:
    return "BrowserBack";
  case 0xE06B:
    return "LaunchApp1";
  case 0xE06C:
    return "LaunchMail";
  case 0xE06D:
    return "MediaSelect";
  default:
    return unidentified;
  }
}

/// macOS: a virtual keycode (kVK_*, CGKeyCode) to its DOM code. Unknown codes
/// give @c unidentified.
inline const char *dom_code_from_mac_keycode(unsigned keycode) {
  switch (keycode) {
  case 0x00:
    return "KeyA";
  case 0x01:
    return "KeyS";
  case 0x02:
    return "KeyD";
  case 0x03:
    return "KeyF";
  case 0x04:
    return "KeyH";
  case 0x05:
    return "KeyG";
  case 0x06:
    return "KeyZ";
  case 0x07:
    return "KeyX";
  case 0x08:
    return "KeyC";
  case 0x09:
    return "KeyV";
  case 0x0A:
    return "IntlBackslash";
  case 0x0B:
    return "KeyB";
  case 0x0C:
    return "KeyQ";
  case 0x0D:
    return "KeyW";
  case 0x0E:
    return "KeyE";
  case 0x0F:
    return "KeyR";
  case 0x10:
    return "KeyY";
  case 0x11:
    return "KeyT";
  case 0x12:
    return "Digit1";
  case 0x13:
    return "Digit2";
  case 0x14:
    return "Digit3";
  case 0x15:
    return "Digit4";
  case 0x16:
    return "Digit6";
  case 0x17:
    return "Digit5";
  case 0x18:
    return "Equal";
  case 0x19:
    return "Digit9";
  case 0x1A:
    return "Digit7";
  case 0x1B:
    return "Minus";
  case 0x1C:
    return "Digit8";
  case 0x1D:
    return "Digit0";
  case 0x1E:
    return "BracketRight";
  case 0x1F:
    return "KeyO";
  case 0x20:
    return "KeyU";
  case 0x21:
    return "BracketLeft";
  case 0x22:
    return "KeyI";
  case 0x23:
    return "KeyP";
  case 0x24:
    return "Enter";
  case 0x25:
    return "KeyL";
  case 0x26:
    return "KeyJ";
  case 0x27:
    return "Quote";
  case 0x28:
    return "KeyK";
  case 0x29:
    return "Semicolon";
  case 0x2A:
    return "Backslash";
  case 0x2B:
    return "Comma";
  case 0x2C:
    return "Slash";
  case 0x2D:
    return "KeyN";
  case 0x2E:
    return "KeyM";
  case 0x2F:
    return "Period";
  case 0x30:
    return "Tab";
  case 0x31:
    return "Space";
  case 0x32:
    return "Backquote";
  case 0x33:
    return "Backspace";
  case 0x34:
    return "NumpadEnter"; // the PowerBook keypad Enter
  case 0x35:
    return "Escape";
  case 0x36:
    return "MetaRight";
  case 0x37:
    return "MetaLeft";
  case 0x38:
    return "ShiftLeft";
  case 0x39:
    return "CapsLock";
  case 0x3A:
    return "AltLeft";
  case 0x3B:
    return "ControlLeft";
  case 0x3C:
    return "ShiftRight";
  case 0x3D:
    return "AltRight";
  case 0x3E:
    return "ControlRight";
  case 0x3F:
    return "Fn";
  case 0x40:
    return "F17";
  case 0x41:
    return "NumpadDecimal";
  case 0x43:
    return "NumpadMultiply";
  case 0x45:
    return "NumpadAdd";
  case 0x47:
    return "NumLock"; // the keypad Clear key
  case 0x48:
    return "AudioVolumeUp";
  case 0x49:
    return "AudioVolumeDown";
  case 0x4A:
    return "AudioVolumeMute";
  case 0x4B:
    return "NumpadDivide";
  case 0x4C:
    return "NumpadEnter";
  case 0x4E:
    return "NumpadSubtract";
  case 0x4F:
    return "F18";
  case 0x50:
    return "F19";
  case 0x51:
    return "NumpadEqual";
  case 0x52:
    return "Numpad0";
  case 0x53:
    return "Numpad1";
  case 0x54:
    return "Numpad2";
  case 0x55:
    return "Numpad3";
  case 0x56:
    return "Numpad4";
  case 0x57:
    return "Numpad5";
  case 0x58:
    return "Numpad6";
  case 0x59:
    return "Numpad7";
  case 0x5A:
    return "F20";
  case 0x5B:
    return "Numpad8";
  case 0x5C:
    return "Numpad9";
  case 0x5D:
    return "IntlYen";
  case 0x5E:
    return "IntlRo";
  case 0x5F:
    return "NumpadComma";
  case 0x60:
    return "F5";
  case 0x61:
    return "F6";
  case 0x62:
    return "F7";
  case 0x63:
    return "F3";
  case 0x64:
    return "F8";
  case 0x65:
    return "F9";
  case 0x66:
    return "Lang2"; // Eisu
  case 0x67:
    return "F11";
  case 0x68:
    return "Lang1"; // Kana
  case 0x69:
    return "F13";
  case 0x6A:
    return "F16";
  case 0x6B:
    return "F14";
  case 0x6D:
    return "F10";
  case 0x6E:
    return "ContextMenu";
  case 0x6F:
    return "F12";
  case 0x71:
    return "F15";
  case 0x72:
    return "Insert"; // Help on older Apple keyboards
  case 0x73:
    return "Home";
  case 0x74:
    return "PageUp";
  case 0x75:
    return "Delete";
  case 0x76:
    return "F4";
  case 0x77:
    return "End";
  case 0x78:
    return "F2";
  case 0x79:
    return "PageDown";
  case 0x7A:
    return "F1";
  case 0x7B:
    return "ArrowLeft";
  case 0x7C:
    return "ArrowRight";
  case 0x7D:
    return "ArrowDown";
  case 0x7E:
    return "ArrowUp";
  default:
    return unidentified;
  }
}

} // namespace key_codes
} // namespace detail
} // namespace webview

#endif // defined(__cplusplus) && !defined(WEBVIEW_HEADER)
#endif // WEBVIEW_DETAIL_KEY_CODES_HH
