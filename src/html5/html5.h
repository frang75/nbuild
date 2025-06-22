/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: html5.h
 *
 */

/* html5 */

#include "html5.hxx"

__EXTERN_C

void html5_start(void);

void html5_finish(void);

Html5Status *html5_status_create(void);

void html5_status_destroy(Html5Status **status);

void html5_status_err(Html5Status *status, String **err);

const ArrPt(String) *html5_status_errors(const Html5Status *status);

bool_t html5_status_with_errors(const Html5Status *status);

void html5_status_clean_errors(Html5Status *status);

__END_C
