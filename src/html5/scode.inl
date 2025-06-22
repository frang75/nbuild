/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: scode.inl
 *
 */

/* Html5 Code Syntax Highlighting */

#include "html5.ixx"

__EXTERN_C

void scode_destroy(SCode **code);

void scode_site(SCode *code, WSite *site);

void scode_css(const SCode *code, CSS *css);

void scode_js(const SCode *code, const jssec_t sec, Stream *js);

__END_C
