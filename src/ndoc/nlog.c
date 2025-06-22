/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: nlog.c
 *
 */

/* ndoc logger */

#include "nlog.h"
#include <nlib/nlib.h>
#include <html5/html5.h>
#include <core/arrpt.h>
#include <core/hfile.h>
#include <core/strings.h>
#include <osbs/log.h>
#include <sewer/cassert.h>

/*---------------------------------------------------------------------------*/

void nlog_init(void)
{
    String *file = hfile_appdata("log.txt");
    log_file(tc(file));
    str_destroy(&file);
}

/*---------------------------------------------------------------------------*/

void nlog_ok(String **msg)
{
    cassert_no_null(msg);
    log_printf("%s[Ok]%s %s", kASCII_GREEN, kASCII_RESET, tc(*msg));
    str_destroy(msg);
}

/*---------------------------------------------------------------------------*/

void nlog_warn(String **msg)
{
    cassert_no_null(msg);
    log_printf("%s[Warning]%s %s", kASCII_YELLOW, kASCII_RESET, tc(*msg));
    str_destroy(msg);
}

/*---------------------------------------------------------------------------*/

void nlog_error(String **msg)
{
    cassert_no_null(msg);
    log_printf("%s[Error]%s %s", kASCII_RED, kASCII_RESET, tc(*msg));
    str_destroy(msg);
}

/*---------------------------------------------------------------------------*/

void nlog_html5(Html5Status *status, const char_t *prefix)
{
    const ArrPt(String) *errors = html5_status_errors(status);
    arrpt_foreach_const(error, errors, String)
        String *msg = str_printf("%s %s.", prefix, tc(error));
        nlog_error(&msg);
    arrpt_end()
    html5_status_clean_errors(status);
}
