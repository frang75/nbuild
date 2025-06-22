/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: header.inl
 *
 */

/* Html5 Header */

#include "html5.ixx"

__EXTERN_C

void header_destroy(Header **header);

void header_site(Header *header, WSite *site, Html5Status *status);

real32_t header_height(const Header *header);

void header_css(const Header *header, CSS *css, Html5Status *status);

void header_js(const Header *header, const jssec_t sec, Stream *js);

void header_html(const Header *header, const ArrPt(String) *langs, Stream *html, Listener *listener);

__END_C
