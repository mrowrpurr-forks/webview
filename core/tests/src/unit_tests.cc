#include "webview/test_driver.hh"
#include "webview/webview.h"

TEST_CASE("Ensure that JSON parsing works") {
  auto J = webview::detail::json_parse;
  // Valid input with expected output
  REQUIRE(J(R"({"foo":"bar"})", "foo", -1) == "bar");
  REQUIRE(J(R"({"foo":""})", "foo", -1).empty());
  REQUIRE(J(R"({"foo":{}")", "foo", -1) == "{}");
  REQUIRE(J(R"({"foo": {"bar": 1}})", "foo", -1) == R"({"bar": 1})");
  REQUIRE(J(R"(["foo", "bar", "baz"])", "", 0) == "foo");
  REQUIRE(J(R"(["foo", "bar", "baz"])", "", 2) == "baz");
  // Valid UTF-8 with expected output
  REQUIRE(J(R"({"フー":"バー"})", "フー", -1) == "バー");
  REQUIRE(J(R"(["フー", "バー", "バズ"])", "", 2) == "バズ");
  // Invalid input with valid output - should probably fail
  REQUIRE(J(R"({"foo":"bar")", "foo", -1) == "bar");
  // Valid input with other invalid parameters - should fail
  REQUIRE(J(R"([])", "", 0).empty());
  REQUIRE(J(R"({})", "foo", -1).empty());
  REQUIRE(J(R"(["foo", "bar", "baz"])", "", -1).empty());
  REQUIRE(J(R"(["foo"])", "", 1234).empty());
  REQUIRE(J(R"(["foo"])", "", -1234).empty());
  // Invalid input - should fail
  REQUIRE(J("", "", 0).empty());
  REQUIRE(J("", "foo", -1).empty());
  REQUIRE(J(R"({"foo":")", "foo", -1).empty());
  REQUIRE(J(R"({"foo":{)", "foo", -1).empty());
  REQUIRE(J(R"({"foo":{")", "foo", -1).empty());
  REQUIRE(J(R"(}")", "foo", -1).empty());
  REQUIRE(J(R"({}}")", "foo", -1).empty());
  REQUIRE(J(R"("foo)", "foo", -1).empty());
  REQUIRE(J(R"(foo)", "foo", -1).empty());
  REQUIRE(J(R"({{[[""foo""]]}})", "", 1234).empty());
  REQUIRE(J("bad", "", 0).empty());
  REQUIRE(J("bad", "foo", -1).empty());
}

TEST_CASE("Ensure that JSON escaping works") {
  using webview::detail::json_escape;

  // Simple case without need for escaping. Quotes added by default.
  REQUIRE(json_escape("hello") == "\"hello\"");
  // Simple case without need for escaping. Quotes explicitly not added.
  REQUIRE(json_escape("hello", false) == "hello");
  // Empty input should return empty output.
  REQUIRE(json_escape("", false).empty());
  // '"' and '\' should be escaped.
  REQUIRE(json_escape("\"", false) == "\\\"");
  REQUIRE(json_escape("\\", false) == "\\\\");
  // Commonly-used characters that should be escaped.
  REQUIRE(json_escape("\b\f\n\r\t", false) == "\\b\\f\\n\\r\\t");
  // ASCII control characters should be escaped.
  REQUIRE(json_escape(std::string{"\0\x1f", 2}, false) == "\\u0000\\u001f");
  // ASCII printable characters (even DEL) shouldn't be escaped.
  REQUIRE(json_escape("\x20\x7e\x7f", false) == "\x20\x7e\x7f");
  // Valid UTF-8.
  REQUIRE(json_escape("\u2328", false) == "\u2328");
  REQUIRE(json_escape("フーバー", false) == "フーバー");
  // Replacement character for invalid characters.
  REQUIRE(json_escape("�", false) == "�");
  // Invalid characters should be replaced with '�' but we just leave them as-is.
  REQUIRE(json_escape("\x80\x9f\xa0\xff", false) == "\x80\x9f\xa0\xff");
  // JS code should not be executed (eval).
  auto expected_gotcha = R"js(alert(\"gotcha\"))js";
  REQUIRE(json_escape(R"(alert("gotcha"))", false) == expected_gotcha);
}

TEST_CASE("optional class") {
  using namespace webview::detail;

  REQUIRE(!optional<int>{}.has_value());
  REQUIRE(optional<int>{1}.has_value());
  REQUIRE(optional<int>{1}.get() == 1);

  REQUIRE_THROW(bad_access, [] { optional<int>{}.get(); });

  REQUIRE(!optional<int>{optional<int>{}}.has_value());
  REQUIRE(optional<int>{optional<int>{1}}.has_value());
  REQUIRE(optional<int>{optional<int>{1}}.get() == 1);
}

TEST_CASE("result class") {
  using namespace webview::detail;
  using namespace webview;

  REQUIRE(result<int>{}.has_value());
  REQUIRE(result<int>{}.value() == 0);
  REQUIRE(result<int>{1}.has_value());
  REQUIRE(result<int>{1}.value() == 1);
  REQUIRE(!result<int>{}.has_error());
  REQUIRE(!result<int>{1}.has_error());
  REQUIRE(result<int>{}.ok());
  REQUIRE(result<int>{1}.ok());
  REQUIRE(!result<int>{error_info{}}.ok());
  REQUIRE(!result<int>{error_info{}}.has_value());
  REQUIRE(result<int>{error_info{}}.has_error());

  auto result_with_error = result<int>{
      error_info{WEBVIEW_ERROR_INVALID_ARGUMENT, "invalid argument"}};
  REQUIRE(result_with_error.error().code() == WEBVIEW_ERROR_INVALID_ARGUMENT);
  REQUIRE(result_with_error.error().message() == "invalid argument");

  REQUIRE_THROW(bad_access, [] { result<int>{}.error(); });
}

TEST_CASE("noresult class") {
  using namespace webview::detail;
  using namespace webview;

  REQUIRE(!noresult{}.has_error());
  REQUIRE(noresult{}.ok());
  REQUIRE(!noresult{error_info{}}.ok());
  REQUIRE(noresult{error_info{}}.has_error());

  auto result_with_error =
      noresult{error_info{WEBVIEW_ERROR_INVALID_ARGUMENT, "invalid argument"}};
  REQUIRE(result_with_error.error().code() == WEBVIEW_ERROR_INVALID_ARGUMENT);
  REQUIRE(result_with_error.error().message() == "invalid argument");

  REQUIRE_THROW(bad_access, [] { noresult{}.error(); });
}

#if _WIN32
TEST_CASE("Ensure that version number parsing works on Windows") {
  using namespace webview::detail;
  auto v = parse_version("");
  REQUIRE(v.size() == 4);
  REQUIRE(v[0] == 0 && v[1] == 0 && v[2] == 0 && v[3] == 0);
  v = parse_version("1");
  REQUIRE(v[0] == 1 && v[1] == 0 && v[2] == 0 && v[3] == 0);
  v = parse_version("0.2");
  REQUIRE(v[0] == 0 && v[1] == 2 && v[2] == 0 && v[3] == 0);
  v = parse_version("0.0.3");
  REQUIRE(v[0] == 0 && v[1] == 0 && v[2] == 3 && v[3] == 0);
  v = parse_version("0.0.0.4");
  REQUIRE(v[0] == 0 && v[1] == 0 && v[2] == 0 && v[3] == 4);
  v = parse_version("1.2.3.4.5");
  REQUIRE(v.size() == 4);
  REQUIRE(v[0] == 1 && v[1] == 2 && v[2] == 3 && v[3] == 4);
  v = parse_version("1.2.3.4.5.6");
  REQUIRE(v[0] == 1 && v[1] == 2 && v[2] == 3 && v[3] == 4);
  v = parse_version("11.22.33.44");
  REQUIRE(v[0] == 11 && v[1] == 22 && v[2] == 33 && v[3] == 44);
  v = parse_version("0.0.0.0");
  REQUIRE(v[0] == 0 && v[1] == 0 && v[2] == 0 && v[3] == 0);
  v = parse_version("-1.-2.-3.-4");
  REQUIRE(v[0] == 0 && v[1] == 0 && v[2] == 0 && v[3] == 0);
  v = parse_version("a.b.c.d");
  REQUIRE(v[0] == 0 && v[1] == 0 && v[2] == 0 && v[3] == 0);
  v = parse_version("...");
  REQUIRE(v[0] == 0 && v[1] == 0 && v[2] == 0 && v[3] == 0);
}

TEST_CASE("Ensure that narrow/wide string conversion works on Windows") {
  using namespace webview::detail;
  REQUIRE(widen_string("").empty());
  REQUIRE(narrow_string(L"").empty());
  REQUIRE(widen_string("foo") == L"foo");
  REQUIRE(narrow_string(L"foo") == "foo");
  REQUIRE(widen_string("フー") == L"フー");
  REQUIRE(narrow_string(L"フー") == "フー");
  REQUIRE(widen_string("æøå") == L"æøå");
  REQUIRE(narrow_string(L"æøå") == "æøå");
  // Unicode number for the smiley face below: U+1F600
  REQUIRE(widen_string("😀") == L"😀");
  REQUIRE(narrow_string(L"😀") == "😀");
  // Ensure that elements of wide string are correct
  {
    auto s = widen_string("😀");
    REQUIRE(s.size() == 2);
    REQUIRE(static_cast<std::uint16_t>(s[0]) == 0xD83D);
    REQUIRE(static_cast<std::uint16_t>(s[1]) == 0xDE00);
  }
  // Ensure that elements of narrow string are correct
  {
    auto s = narrow_string(L"😀");
    REQUIRE(s.size() == 4);
    REQUIRE(static_cast<std::uint8_t>(s[0]) == 0xf0);
    REQUIRE(static_cast<std::uint8_t>(s[1]) == 0x9f);
    REQUIRE(static_cast<std::uint8_t>(s[2]) == 0x98);
    REQUIRE(static_cast<std::uint8_t>(s[3]) == 0x80);
  }
  // Null-characters must also be converted
  REQUIRE(widen_string(std::string(2, '\0')) == std::wstring(2, L'\0'));
  REQUIRE(narrow_string(std::wstring(2, L'\0')) == std::string(2, '\0'));
}
#endif

// The key tables behind the keyboard hook's DOM codes, on every platform.
TEST_CASE("Key codes: Windows scan codes to DOM codes") {
  using webview::detail::key_codes::dom_code_from_windows_scan;
  REQUIRE(std::string(dom_code_from_windows_scan(0x001E)) == "KeyA");
  REQUIRE(std::string(dom_code_from_windows_scan(0x002C)) == "KeyZ");
  REQUIRE(std::string(dom_code_from_windows_scan(0x0002)) == "Digit1");
  REQUIRE(std::string(dom_code_from_windows_scan(0x001C)) == "Enter");
  REQUIRE(std::string(dom_code_from_windows_scan(0xE01C)) == "NumpadEnter");
  REQUIRE(std::string(dom_code_from_windows_scan(0x001D)) == "ControlLeft");
  REQUIRE(std::string(dom_code_from_windows_scan(0xE01D)) == "ControlRight");
  REQUIRE(std::string(dom_code_from_windows_scan(0x0045)) == "Pause");
  REQUIRE(std::string(dom_code_from_windows_scan(0xE045)) == "NumLock");
  REQUIRE(std::string(dom_code_from_windows_scan(0x0053)) == "NumpadDecimal");
  REQUIRE(std::string(dom_code_from_windows_scan(0xE053)) == "Delete");
  REQUIRE(std::string(dom_code_from_windows_scan(0xE04B)) == "ArrowLeft");
  REQUIRE(std::string(dom_code_from_windows_scan(0xE05B)) == "MetaLeft");
  REQUIRE(std::string(dom_code_from_windows_scan(0x0058)) == "F12");
  REQUIRE(std::string(dom_code_from_windows_scan(0x0076)) == "F24");
  REQUIRE(std::string(dom_code_from_windows_scan(0x00FF)) == "Unidentified");
  REQUIRE(std::string(dom_code_from_windows_scan(0xE0FF)) == "Unidentified");
}

TEST_CASE("Key codes: macOS keycodes to DOM codes") {
  using webview::detail::key_codes::dom_code_from_mac_keycode;
  REQUIRE(std::string(dom_code_from_mac_keycode(0x00)) == "KeyA");
  REQUIRE(std::string(dom_code_from_mac_keycode(0x06)) == "KeyZ");
  REQUIRE(std::string(dom_code_from_mac_keycode(0x12)) == "Digit1");
  REQUIRE(std::string(dom_code_from_mac_keycode(0x24)) == "Enter");
  REQUIRE(std::string(dom_code_from_mac_keycode(0x4C)) == "NumpadEnter");
  REQUIRE(std::string(dom_code_from_mac_keycode(0x33)) == "Backspace");
  REQUIRE(std::string(dom_code_from_mac_keycode(0x75)) == "Delete");
  REQUIRE(std::string(dom_code_from_mac_keycode(0x35)) == "Escape");
  REQUIRE(std::string(dom_code_from_mac_keycode(0x37)) == "MetaLeft");
  REQUIRE(std::string(dom_code_from_mac_keycode(0x3A)) == "AltLeft");
  REQUIRE(std::string(dom_code_from_mac_keycode(0x7B)) == "ArrowLeft");
  REQUIRE(std::string(dom_code_from_mac_keycode(0x7A)) == "F1");
  REQUIRE(std::string(dom_code_from_mac_keycode(0x7F)) == "Unidentified");
}

#if defined(_WIN32)
// The scan code the keyboard hook names a key by when injected input carries
// none: derived from the virtual key, with the navigation cluster, Pause and
// PrintScreen given the codes the keyboard itself would send.
TEST_CASE("Keyboard hook: scan code from a bare virtual key on Windows") {
  using webview::detail::keyboard_hook::scan_of;
  auto bare = [](DWORD vk) {
    KBDLLHOOKSTRUCT k{};
    k.vkCode = vk;
    return scan_of(k);
  };
  REQUIRE(bare(VK_DELETE) == 0xE053u);
  REQUIRE(bare(VK_HOME) == 0xE047u);
  REQUIRE(bare(VK_LEFT) == 0xE04Bu);
  REQUIRE(bare(VK_RETURN) == 0x001Cu);
  REQUIRE(bare(VK_PAUSE) == 0x0045u);
  REQUIRE(bare(VK_NUMLOCK) == 0xE045u);
  REQUIRE(bare(VK_SNAPSHOT) == 0xE037u);
  REQUIRE(bare(VK_RCONTROL) == 0xE01Du);
  REQUIRE(bare(VK_LWIN) == 0xE05Bu);
  REQUIRE(bare(VK_DIVIDE) == 0xE035u);
  REQUIRE(bare(VK_PACKET) == 0u);
  REQUIRE(bare('Z') == 0x002Cu);
  // A scan code that is there is used as it is, with the extended flag.
  KBDLLHOOKSTRUCT k{};
  k.vkCode = VK_DELETE;
  k.scanCode = 0x53;
  k.flags = LLKHF_EXTENDED;
  REQUIRE(scan_of(k) == 0xE053u);
  k.flags = 0;
  REQUIRE(scan_of(k) == 0x0053u); // the numpad's decimal under Num Lock off
}
#endif

#if defined(_WIN32) || defined(__APPLE__)
// The hooks' lifecycle: one of each per process, removable, independent of
// each other, and honest about their state. On macOS the install itself needs
// the Accessibility permission, which a test machine may not have granted;
// the tests then check only the refusal.
TEST_CASE("Mouse hook: install once, remove once") {
  auto fn = [](int, double, double, double, unsigned, void *) -> int {
    return 0;
  };
  REQUIRE(webview_mouse_hook_install(nullptr, nullptr) ==
          WEBVIEW_ERROR_INVALID_ARGUMENT);
  REQUIRE(webview_mouse_hook_remove() == WEBVIEW_ERROR_NOT_FOUND);
  auto installed = webview_mouse_hook_install(fn, nullptr);
  if (installed == WEBVIEW_ERROR_MISSING_DEPENDENCY) {
    REQUIRE(webview_mouse_hook_remove() == WEBVIEW_ERROR_NOT_FOUND);
    return;
  }
  REQUIRE(installed == WEBVIEW_ERROR_OK);
  REQUIRE(webview_mouse_hook_install(fn, nullptr) == WEBVIEW_ERROR_DUPLICATE);
  REQUIRE(webview_mouse_hook_remove() == WEBVIEW_ERROR_OK);
  REQUIRE(webview_mouse_hook_remove() == WEBVIEW_ERROR_NOT_FOUND);
  REQUIRE(webview_mouse_hook_install(fn, nullptr) == WEBVIEW_ERROR_OK);
  REQUIRE(webview_mouse_hook_remove() == WEBVIEW_ERROR_OK);
}

TEST_CASE("Keyboard hook: install once, remove once") {
  auto fn = [](int, const char *, unsigned, unsigned, void *) -> int {
    return 0;
  };
  REQUIRE(webview_keyboard_hook_install(nullptr, nullptr) ==
          WEBVIEW_ERROR_INVALID_ARGUMENT);
  REQUIRE(webview_keyboard_hook_remove() == WEBVIEW_ERROR_NOT_FOUND);
  auto installed = webview_keyboard_hook_install(fn, nullptr);
  if (installed == WEBVIEW_ERROR_MISSING_DEPENDENCY) {
    REQUIRE(webview_keyboard_hook_remove() == WEBVIEW_ERROR_NOT_FOUND);
    return;
  }
  REQUIRE(installed == WEBVIEW_ERROR_OK);
  REQUIRE(webview_keyboard_hook_install(fn, nullptr) ==
          WEBVIEW_ERROR_DUPLICATE);
  REQUIRE(webview_keyboard_hook_remove() == WEBVIEW_ERROR_OK);
  REQUIRE(webview_keyboard_hook_remove() == WEBVIEW_ERROR_NOT_FOUND);
  REQUIRE(webview_keyboard_hook_install(fn, nullptr) == WEBVIEW_ERROR_OK);
  REQUIRE(webview_keyboard_hook_remove() == WEBVIEW_ERROR_OK);
}

TEST_CASE("Mouse and keyboard hooks: either order, either removed first") {
  auto mouse = [](int, double, double, double, unsigned, void *) -> int {
    return 0;
  };
  auto keys = [](int, const char *, unsigned, unsigned, void *) -> int {
    return 0;
  };
  auto first = webview_mouse_hook_install(mouse, nullptr);
  if (first == WEBVIEW_ERROR_MISSING_DEPENDENCY) {
    return;
  }
  REQUIRE(first == WEBVIEW_ERROR_OK);
  REQUIRE(webview_keyboard_hook_install(keys, nullptr) == WEBVIEW_ERROR_OK);
  // Removing the mouse hook leaves the keyboard hook installed.
  REQUIRE(webview_mouse_hook_remove() == WEBVIEW_ERROR_OK);
  REQUIRE(webview_keyboard_hook_install(keys, nullptr) ==
          WEBVIEW_ERROR_DUPLICATE);
  // And the other way round, keyboard first.
  REQUIRE(webview_mouse_hook_install(mouse, nullptr) == WEBVIEW_ERROR_OK);
  REQUIRE(webview_keyboard_hook_remove() == WEBVIEW_ERROR_OK);
  REQUIRE(webview_mouse_hook_install(mouse, nullptr) ==
          WEBVIEW_ERROR_DUPLICATE);
  REQUIRE(webview_mouse_hook_remove() == WEBVIEW_ERROR_OK);
  REQUIRE(webview_keyboard_hook_remove() == WEBVIEW_ERROR_NOT_FOUND);
}
#endif
