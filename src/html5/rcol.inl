/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: rcol.inl
 *
 */

/* Html5 Right Column */

#include "html5.ixx"

__EXTERN_C

void rcol_destroy(RCol **rcol);

void rcol_site(RCol *rcol, WSite *site);

real32_t rcol_width(const RCol *rcol);

void rcol_css(const RCol *rcol, CSS *css);

void rcol_js(const RCol *rcol, const jssec_t sec, Stream *js);

void rcol_html(const RCol *rcol, Stream *html, Listener *listener);

__END_C
