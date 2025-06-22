/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: css.h
 *
 */

/* Html5 CSS */

#include "html5.hxx"

__EXTERN_C

void css_comment(CSS *css, const char_t *comment);

void css_sel(CSS *css, const char_t *sel);

void css_sele(CSS *css);

void css_media_max(CSS *css, const real32_t width);

void css_mediae(CSS *css);

void css_px(CSS *css, const char_t *prop, const real32_t value);

void css_px_important(CSS *css, const char_t *prop, const real32_t value);

void css_px2(CSS *css, const char_t *prop, const real32_t value1, const real32_t value2);

void css_px4(CSS *css, const char_t *prop, const real32_t value1, const real32_t value2, const real32_t value3, const real32_t value4);

void css_int(CSS *css, const char_t *prop, const int32_t value);

void css_real(CSS *css, const char_t *prop, const real32_t value);

void css_str(CSS *css, const char_t *prop, const char_t *value);

void css_rgba(CSS *css, const char_t *prop, const color_t color);

void css_rgba_important(CSS *css, const char_t *prop, const color_t color);

void css_direct(CSS *css, const char_t *cssline);

__END_C
