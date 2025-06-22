/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: network.h
 *
 */

/* nbuild network */

#include "nbuild.hxx"

void network_dbind(void);

void network_localhost(Login *login, const ArrSt(uint32_t) *ips);

ArrSt(uint32_t) *network_local_ips(void);
