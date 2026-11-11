# Permissions

```c
unsigned        webview_feature_permissions(int feature);
webview_error_t webview_permission_status(unsigned permission);
webview_error_t webview_permission_request(unsigned permission);
webview_error_t webview_permission_set_auto_request(int on);
```

What a feature needs from the operating system, whether the process has it, and who asks the user.

Only macOS has anything to ask; Windows and Linux need nothing.

The [mouse hook](mouse-hook.md) and the [keyboard hook](keyboard-hook.md) share one `CGEventTap` that
filters events, which takes **Device Control and Data Access** in System Settings > Privacy & Security.

```c
webview_permission_set_auto_request(0); // no system dialog from a failed install

unsigned needed = webview_feature_permissions(WEBVIEW_FEATURE_KEYBOARD_HOOK);
if (needed & WEBVIEW_PERMISSION_DEVICE_CONTROL &&
    webview_permission_status(WEBVIEW_PERMISSION_DEVICE_CONTROL) != WEBVIEW_ERROR_OK) {
  explain_why_the_app_needs_it();
  webview_permission_request(WEBVIEW_PERMISSION_DEVICE_CONTROL); // the system's dialog
}
```

## Types

| Type                   | Values                                                                                                                                                                          |
| ---------------------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `webview_permission_t` | `WEBVIEW_PERMISSION_DEVICE_CONTROL` (macOS "Device Control and Data Access": filter and post input events), `WEBVIEW_PERMISSION_INPUT_MONITORING` (macOS "Input Monitoring": watch without filtering; no feature here needs it, it is modeled so an app can ask about it the same way) |
| `webview_feature_t`    | `WEBVIEW_FEATURE_MOUSE_HOOK`, `WEBVIEW_FEATURE_KEYBOARD_HOOK`                                                                                                                   |

## Calls

| Call                                           | Answer                                                                                                                                                                                                                                                                              |
| ---------------------------------------------- | ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| `webview_feature_permissions(feature)`         | The `webview_permission_t` bits the feature needs on this platform; 0 when none. Ask this first, then `status` for each bit, and the app knows what it needs before it installs                                                                                                  |
| `webview_permission_status(permission)`        | `WEBVIEW_ERROR_OK` granted; `WEBVIEW_ERROR_MISSING_DEPENDENCY` not granted; `WEBVIEW_ERROR_UNSPECIFIED` no such permission on this platform. Nothing is shown to the user                                                                                                          |
| `webview_permission_request(permission)`       | Asks the system. On macOS it adds the app to the row in System Settings and shows the dialog each time this is called while the permission is missing; the user grants it there, and an install after that succeeds. Same answers as `status`; `MISSING_DEPENDENCY` means the dialog was shown |
| `webview_permission_set_auto_request(on)`      | Whether an install that fails for a missing permission asks the system by itself, once per process, as `request` would. On by default, so an app that never asks about permissions still gets the dialog the first time it installs a hook. Off: the install only fails, and the app asks through `request` |

Without the permission, an install fails with `WEBVIEW_ERROR_MISSING_DEPENDENCY` (after asking once,
if auto-request is on). The caller falls back to whatever else it has, for example hit regions.

## How macOS is asked

| Permission                     | Status                           | Request                        |
| ------------------------------ | -------------------------------- | ------------------------------ |
| Device Control and Data Access | `CGPreflightPostEventAccess`     | `CGRequestPostEventAccess`     |
| Input Monitoring               | `CGPreflightListenEventAccess`   | `CGRequestListenEventAccess`   |

- A grant belongs to the app that asks, by code identity. A script run from a terminal gets the
  terminal's grant. An `.app` bundle gets its own, and loses it when its signature changes (an ad-hoc
  signature changes with every rebuild).
- Granting Device Control and Data Access makes macOS report Input Monitoring as granted too: post
  access includes listen access.
