# Assets

```c
webview_error_t webview_set_asset(webview_t w, const char *path, const char *mime,
                                  const void *data, size_t len);
```

Serves bytes at a path of the webview's own origin, so the page is loaded with `webview_navigate`
instead of `webview_set_html` (which Windows caps at 2 MB), and can fetch scripts, styles, images and
models by relative URL. Nothing reaches the network. The bytes are copied; calling again with the same
path replaces them.

```c
webview_set_asset(w, "index.html", "text/html; charset=utf-8", html, strlen(html));
webview_set_asset(w, "main.js", "text/javascript", js, js_len);
webview_navigate(w, "http://webview.localhost/index.html");
```

The origin is fixed per platform:

| Platform | Origin                     | How                                                                                                              |
| -------- | -------------------------- | ---------------------------------------------------------------------------------------------------------------- |
| Windows  | `http://webview.localhost` | WebView2's `WebResourceRequested` event; requests to any other origin are untouched                              |
| macOS    | `webview://localhost`      | A `WKURLSchemeHandler` for the scheme `webview`, registered when the `WKWebView` is created. Answers with an `NSHTTPURLResponse`, so WebKit parses a `Content-Type` with a charset and loads module scripts |
| Linux    |                            | Accepted and stored; not served                                                                                  |

`path` is the URL path without the leading slash and must not need percent-encoding; a request's query
string and fragment are ignored when matching. `mime` is the value of the `Content-Type` header.
