/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: nboot.h
 *
 */

/* nbuild booter */

#include "nbuild.hxx"

bool_t nboot_boot(const Host *host, const ArrSt(Host) *hosts, runstate_t *state);

bool_t nboot_shutdown(const Host *host, const ArrSt(Host) *hosts, const runstate_t state);
