/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: post.inl
 *
 */

/* Html5 Post */

#include "html5.ixx"

__EXTERN_C

void post_destroy(Post **post);

void post_site(Post *post, WSite *site);

real32_t post_width(const Post *post);

void post_css(const Post *post, CSS *css);

void post_js(const Post *post, const jssec_t sec, Stream *js);

void post_html(const Post *post, Stream *html, Listener *listener);

__END_C
