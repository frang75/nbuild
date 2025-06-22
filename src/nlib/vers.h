/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: vers.h
 *
 */

/* Version */

#include "nlib.hxx"

Vers vers_from_stm(Stream *stm);

int vers_cmp(const Vers *v1, const Vers *v2);

bool_t vers_gte(const Vers *vers, const uint16_t major, const uint16_t minor, const uint16_t patch);

bool_t vers_lt(const Vers *vers, const uint16_t major, const uint16_t minor, const uint16_t patch);
