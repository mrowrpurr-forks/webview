# Background color

```c
webview_error_t webview_set_background_color(webview_t w, int r, int g, int b, int a);
```

Sets the color the browser paints behind the page. With `a` of 0 and a page whose own background is
transparent, the native window shows through, which is what an overlay or a shaped window wants.

| Platform | Behavior                                                                                                                                                                                   |
| -------- | ------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| Windows  | Sets `ICoreWebView2Controller2::DefaultBackgroundColor`. For true transparency the host window also needs DWM composition that honors alpha, for example `DwmEnableBlurBehindWindow` with an empty region |
| macOS    | Turns `drawsBackground` off on the `WKWebView` and makes the window non-opaque with this color as its background. `webview_set_size` leaves an adopted window alone                         |
| Linux    | Accepted; does nothing                                                                                                                                                                      |

Channels are 0 to 255; `a` of 0 is fully transparent.
