/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: rcol.h
 *
 */

/* Html5 Right Column */

#include "html5.hxx"

__EXTERN_C

RCol *rcol_create(const real32_t width, const color_t bkcolor);

void rcol_add_banner(RCol *rcol, const char_t *img, const char_t *text, const char_t *url, Html5Status *status);

void rcol_remove_banners(RCol *rcol);

__END_C
