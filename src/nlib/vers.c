/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: vers.c
 *
 */

/* Version */

#include "vers.h"
#include <core/stream.h>
#include <core/strings.h>
#include <sewer/cassert.h>

typedef enum _state_t
{
    ekSTATE_NO_VERS = 1,
    ekSTATE_REAL1,
    ekSTATE_OK
} state_t;

/*---------------------------------------------------------------------------*/

Vers vers_from_stm(Stream *stm)
{
    Vers vers = {0, 0, 0};
    state_t state = ekSTATE_NO_VERS;
    ltoken_t token = stm_read_token(stm);
    const char_t *lexeme = stm_token_lexeme(stm, NULL);
    while (token != ekTEOF)
    {
        if (token == ekTREAL)
        {
            String *left = NULL, *right = NULL;
            if (str_split(lexeme, ".", &left, &right) == TRUE)
            {
                if (state == ekSTATE_NO_VERS)
                {
                    bool_t err = FALSE;
                    vers.major = str_to_u16(tc(left), 10, &err);
                    if (err == FALSE)
                        vers.minor = str_to_u16(tc(right), 10, &err);
                    if (err == FALSE)
                        state = ekSTATE_REAL1;
                }
                else if (state == ekSTATE_REAL1)
                {
                    if (str_empty(left) == TRUE)
                    {
                        bool_t err = FALSE;
                        vers.patch = str_to_u16(tc(right), 10, &err);
                        if (err == FALSE)
                            state = ekSTATE_OK;
                    }

                    if (state != ekSTATE_OK)
                        state = ekSTATE_NO_VERS;
                }
            }
            else
            {
                state = ekSTATE_NO_VERS;
            }

            str_destroy(&left);
            str_destroy(&right);
        }

        token = stm_read_token(stm);
        lexeme = stm_token_lexeme(stm, NULL);
    }

    if (state != ekSTATE_OK)
    {
        vers.major = 0;
        vers.minor = 0;
        vers.patch = 0;
    }

    return vers;
}

/*---------------------------------------------------------------------------*/

int vers_cmp(const Vers *v1, const Vers *v2)
{
    cassert_no_null(v1);
    cassert_no_null(v2);
    if (v1->major < v2->major)
        return -1;
    if (v1->major > v2->major)
        return 1;
    if (v1->minor < v2->minor)
        return -1;
    if (v1->minor > v2->minor)
        return 1;
    if (v1->patch < v2->patch)
        return -1;
    if (v1->patch > v2->patch)
        return 1;
    return 0;
}

/*---------------------------------------------------------------------------*/

bool_t vers_gte(const Vers *vers, const uint16_t major, const uint16_t minor, const uint16_t patch)
{
    Vers v;
    v.major = major;
    v.minor = minor;
    v.patch = patch;
    return (bool_t)(vers_cmp(vers, &v) >= 0);
}

/*---------------------------------------------------------------------------*/

bool_t vers_lt(const Vers *vers, const uint16_t major, const uint16_t minor, const uint16_t patch)
{
    Vers v;
    v.major = major;
    v.minor = minor;
    v.patch = patch;
    return (bool_t)(vers_cmp(vers, &v) < 0);
}
