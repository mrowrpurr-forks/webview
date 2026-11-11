# Mouse hook

```c
webview_error_t webview_mouse_hook_install(webview_mouse_hook_fn_t fn, void *arg);
webview_error_t webview_mouse_hook_remove(void);
```

A system-wide mouse hook: `fn` sees every mouse event on the system before any window does, and
decides per event whether it goes on or is swallowed. This is how a click-through overlay gets some of
the buttons over what it draws without ever taking the others: pass every event through except, say,
right and middle presses over a drawn thing.

```c
static int on_mouse(int event, double x, double y, double wheel, unsigned modifiers, void *arg) {
  if (event == WEBVIEW_MOUSE_RIGHT_DOWN && over_something_drawn(x, y))
    return 1; // swallowed: no window gets it
  return 0;   // on to whatever is under the cursor
}

webview_mouse_hook_install(on_mouse, NULL);
webview_run(w);
webview_mouse_hook_remove();
```

## The callback

```c
typedef int (*webview_mouse_hook_fn_t)(int event, double x, double y, double wheel,
                                       unsigned modifiers, void *arg);
```

| Argument    | Meaning                                                                                                                                                                                                            |
| ----------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| `event`     | A `webview_mouse_event_t`: `WEBVIEW_MOUSE_MOVE`, `LEFT_DOWN`, `LEFT_UP`, `RIGHT_DOWN`, `RIGHT_UP`, `MIDDLE_DOWN`, `MIDDLE_UP`, `WHEEL`                                                                              |
| `x`, `y`    | Cursor position in the platform's screen units (physical pixels on Windows, points on macOS), origin at the top-left corner of the primary display                                                                 |
| `wheel`     | For `WEBVIEW_MOUSE_WHEEL`, the amount turned in notches, positive away from the user (a trackpad gives fractions); 0 otherwise                                                                                     |
| `modifiers` | The modifier keys held when the event was generated, as `webview_modifier_t` bits (`SHIFT`, `CONTROL`, `ALT`, `META`; Alt is Option and Meta is Command on macOS, Meta is the Windows key on Windows). On Windows the asynchronous key state inside the hook, on macOS the event's own flags |
| `arg`       | The `arg` given to `webview_mouse_hook_install`                                                                                                                                                                    |
| returns     | Non-zero to swallow the event, so that no window receives it; zero to let it go on                                                                                                                                  |

## Rules

- One hook per process; it belongs to no webview instance.
- Call it on the UI thread before `webview_run`: `fn` runs on that thread from inside the run loop,
  and must return quickly (the OS removes a hook that stalls).
- Independent of the [keyboard hook](keyboard-hook.md): install both in either order, remove either,
  and the other keeps running.

## Per platform

| Platform | Behavior                                                                                                                                                                                                                                                                      |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------ |
| Windows  | A low-level mouse hook (`WH_MOUSE_LL`) on the calling thread, which must pump messages; `webview_run` does                                                                                                                                                                     |
| macOS    | A `CGEventTap` at the session level on the main run loop, shared with the keyboard hook. The process needs **Device Control and Data Access**; see [Permissions](permissions.md). A tap the system disables for being slow is re-enabled by the library                       |
| Linux    | Not supported; fails with `WEBVIEW_ERROR_UNSPECIFIED`                                                                                                                                                                                                                          |

## Errors

| Code                               | When                                                                   |
| ---------------------------------- | ---------------------------------------------------------------------- |
| `WEBVIEW_ERROR_DUPLICATE`          | A mouse hook is already installed                                      |
| `WEBVIEW_ERROR_MISSING_DEPENDENCY` | macOS: the permission is missing (the system was asked, if auto-request is on) |
| `WEBVIEW_ERROR_NOT_FOUND`          | `webview_mouse_hook_remove` with no hook installed                     |
