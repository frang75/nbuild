/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: css.inl
 *
 */

/* Html5 CSS */

#include "html5.ixx"

__EXTERN_C

CSS *css_create(const char_t *pathname, ferror_t *error);

void css_destroy(CSS **css);

__END_C
