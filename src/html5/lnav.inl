/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: lnav.inl
 *
 */

/* Html5 Laterial Nav */

#include "html5.ixx"

__EXTERN_C

void lnav_destroy(LNav **lnav);

void lnav_site(LNav *lnav, WSite *site);

real32_t lnav_width(const LNav *lnav);

void lnav_css(const LNav *lnav, CSS *css);

void lnav_js(const LNav *lnav, const jssec_t sec, Stream *js);

void lnav_html(const LNav *lnav, Stream *html, Listener *listener);

__END_C
