# Keyboard hook

```c
webview_error_t webview_keyboard_hook_install(webview_keyboard_hook_fn_t fn, void *arg);
webview_error_t webview_keyboard_hook_remove(void);
```

A system-wide keyboard hook. `fn` sees every key event on the system (down, up, and the OS's
auto-repeat) before the focused window does, and decides per event whether it goes on or is swallowed.

A swallowed key reaches no window at all. So a callback takes only the keys it owns, by its own state
(for example while something of its own is selected), and passes the rest.

```c
static int on_key(int event, const char *code, unsigned raw, unsigned modifiers, void *arg) {
  if (event == WEBVIEW_KEY_DOWN && strcmp(code, "Delete") == 0 && something_selected())
    return 1; // swallowed: not even the focused window gets it
  return 0;
}

webview_keyboard_hook_install(on_key, NULL);
webview_run(w);
webview_keyboard_hook_remove();
```

## The callback

```c
typedef int (*webview_keyboard_hook_fn_t)(int event, const char *code, unsigned raw,
                                          unsigned modifiers, void *arg);
```

| Argument    | Meaning                                                                                                                                                                                                                                                              |
| ----------- | -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `event`     | A `webview_key_event_t`: `WEBVIEW_KEY_DOWN` (a key went down that was up), `WEBVIEW_KEY_UP`, `WEBVIEW_KEY_REPEAT` (a held key sent another down)                                                                                                                      |
| `code`      | The physical key as the DOM's `KeyboardEvent.code` names it (`"KeyA"`, `"Digit1"`, `"Enter"`, `"ArrowLeft"`, `"ShiftLeft"`, `"Numpad7"`, `"F5"`...), so a caller never switches on a platform's key numbers; `"Unidentified"` for a key the library has no name for. Valid for the duration of the call only |
| `raw`       | The platform's own code for the key: the virtual-key code on Windows (`VK_*`), the virtual keycode on macOS (`kVK_*`)                                                                                                                                                 |
| `modifiers` | The modifier keys held after this event, as `webview_modifier_t` bits, so a modifier's own down has its bit set and its up has it clear (the DOM's convention)                                                                                                         |
| `arg`       | The `arg` given to `webview_keyboard_hook_install`                                                                                                                                                                                                                    |
| returns     | Non-zero to swallow the event, so that no window receives it; zero to let it go on to the focused window                                                                                                                                                              |

The tables from Windows scan codes and macOS keycodes to DOM codes are in
`core/include/webview/detail/key_codes.hh`, after Chromium's `dom_code_data.inc`, and are unit-tested.

## Rules

- One hook per process; it belongs to no webview instance.
- Call it on the UI thread before `webview_run`: `fn` runs on that thread from inside the run loop,
  and must return quickly (the OS removes a hook that stalls).
- Independent of the [mouse hook](mouse-hook.md): install both in either order, remove either, and
  the other keeps running. A hook may remove itself from inside its own callback.

## Per platform

| Platform | Behavior                                                                                                                                                                                                                                                                                                                                      |
| -------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Windows  | A low-level keyboard hook (`WH_KEYBOARD_LL`) on the calling thread, which must pump messages; `webview_run` does. Repeat is detected from the hook's own state, since the low-level message carries no repeat count                                                                                                                             |
| macOS    | Key-down, key-up and flags-changed on the `CGEventTap` the mouse hook uses: one tap, one permission (**Device Control and Data Access**, see [Permissions](permissions.md)), one run-loop source. A modifier key's press and release arrive as flags-changed and are reported as its down and up. Caps Lock reports a down when the lock turns on and an up when it turns off |
| Linux    | Not supported; fails with `WEBVIEW_ERROR_UNSPECIFIED`                                                                                                                                                                                                                                                                                          |

## Errors

| Code                               | When                                                                            |
| ---------------------------------- | ------------------------------------------------------------------------------- |
| `WEBVIEW_ERROR_DUPLICATE`          | A keyboard hook is already installed                                            |
| `WEBVIEW_ERROR_MISSING_DEPENDENCY` | macOS: the permission is missing (the system was asked, if auto-request is on)  |
| `WEBVIEW_ERROR_NOT_FOUND`          | `webview_keyboard_hook_remove` with no hook installed                           |
