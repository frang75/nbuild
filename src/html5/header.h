/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: header.h
 *
 */

/* Html5 Header */

#include "html5.hxx"

__EXTERN_C

Header *header_create(const char_t *banner, const color_t txcolor, const color_t bkcolor, const real32_t fsize);

void header_text(Header *header, const char_t *text);

void header_home(Header *header, const char_t *url);

__END_C
