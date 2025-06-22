/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: nlib.hxx
 *
 */

/* Commons for NAppGUI Utilities */

#ifndef __NLIB_HXX__
#define __NLIB_HXX__

#include <core/core.hxx>

typedef struct _login_t Login;
typedef struct _vers_t Vers;

struct _login_t
{
    String *ip;
    String *user;
    String *pass;
    uint8_t platform;
    bool_t localhost;
    bool_t use_sshpass;
};

struct _vers_t
{
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
};

#endif
