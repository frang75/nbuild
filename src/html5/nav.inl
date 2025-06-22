/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: nav.inl
 *
 */

/* Html5 Nav */

#include "html5.ixx"

__EXTERN_C

void nav_destroy(Nav **nav);

void nav_site(Nav *nav, WSite *site, Html5Status *status);

real32_t nav_height(const Nav *nav);

void nav_css(const Nav *nav, CSS *css, Html5Status *status);

void nav_js(const Nav *nav, const jssec_t sec, Stream *js);

void nav_html(const Nav *nav, const uint32_t menu_id, Stream *html);

__END_C
