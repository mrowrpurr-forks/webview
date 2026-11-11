# sweetview-webview

A fork of [webview](https://github.com/webview/webview) for the sweetview projects. What the fork adds
is documented here, one page per addition. Upstream's files are as upstream has them; the only edit
outside this folder is the pointer at the top of the root README.

| Addition                                                                            | C API                                                                                                                                                           |
| ----------------------------------------------------------------------------------- | --------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| [Background color](background-color.md): a color, or transparency, behind the page | `webview_set_background_color`                                                                                                                                  |
| [Assets](assets.md): serve bytes at the webview's own origin                        | `webview_set_asset`                                                                                                                                             |
| [Mouse hook](mouse-hook.md): every mouse event on the system, swallow any           | `webview_mouse_hook_install`, `webview_mouse_hook_remove`                                                                                                       |
| [Keyboard hook](keyboard-hook.md): every key event on the system, swallow any       | `webview_keyboard_hook_install`, `webview_keyboard_hook_remove`                                                                                                 |
| [Permissions](permissions.md): what the hooks need from the OS, and who asks        | `webview_feature_permissions`, `webview_permission_status`, `webview_permission_request`, `webview_permission_set_auto_request`                                 |

The declarations and their full documentation are in `core/include/webview/api.h` and
`core/include/webview/types.h`. The implementation lives next to upstream's in
`core/include/webview/detail/` (`input_hooks.hh`, `mouse_hook.hh`, `keyboard_hook.hh`,
`key_codes.hh`, and the backend files). Unit tests are in `core/tests/src/unit_tests.cc`.

Building is upstream's CMake build, unchanged. The Bun bindings,
[sweetview-webview-bun](https://github.com/n0t-invented-here/sweetview-webview-bun), build and ship
this library and wrap the hooks and the permission model in TypeScript.
