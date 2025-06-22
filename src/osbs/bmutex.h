/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: bmutex.h
 *
 */

/* Basic synchronization services */

#include "osbs.hxx"

__EXTERN_C

_osbs_api Mutex *bmutex_create(void);

_osbs_api void bmutex_close(Mutex **mutex);

_osbs_api void bmutex_lock(Mutex *mutex);

_osbs_api void bmutex_unlock(Mutex *mutex);

__END_C
