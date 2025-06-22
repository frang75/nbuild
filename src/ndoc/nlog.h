/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: nlog.h
 *
 */

/* ndoc logger */

#include "ndoc.hxx"

void nlog_init(void);

void nlog_ok(String **msg);

void nlog_warn(String **msg);

void nlog_error(String **msg);

void nlog_html5(Html5Status *status, const char_t *prefix);
