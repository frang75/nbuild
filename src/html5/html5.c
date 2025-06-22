/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: html5.c
 *
 */

/* Html5 */

#include "html5.h"
#include <draw2d/draw2d.h>
#include <core/arrpt.h>
#include <core/heap.h>
#include <core/strings.h>
#include <sewer/cassert.h>

struct _html5status_t
{
    ArrPt(String) *errors;
};

/*---------------------------------------------------------------------------*/

void html5_start(void)
{
    draw2d_start();
}

/*---------------------------------------------------------------------------*/

void html5_finish(void)
{
    draw2d_finish();
}

/*---------------------------------------------------------------------------*/

Html5Status *html5_status_create(void)
{
    Html5Status *status = heap_new0(Html5Status);
    status->errors = arrpt_create(String);
    return status;
}

/*---------------------------------------------------------------------------*/

void html5_status_destroy(Html5Status **status)
{
    cassert_no_null(status);
    cassert_no_null(*status);
    cassert(arrpt_size((*status)->errors, String) == 0);
    arrpt_destroy(&(*status)->errors, str_destroy, String);
    heap_delete(status, Html5Status);
}

/*---------------------------------------------------------------------------*/

void html5_status_err(Html5Status *status, String **err)
{
    cassert_no_null(status);
    cassert_no_null(err);
    cassert_no_null(*err);
    arrpt_append(status->errors, *err, String);
    *err = NULL;
}

/*---------------------------------------------------------------------------*/

const ArrPt(String) *html5_status_errors(const Html5Status *status)
{
    cassert_no_null(status);
    return status->errors;
}

/*---------------------------------------------------------------------------*/

bool_t html5_status_with_errors(const Html5Status *status)
{
    cassert_no_null(status);
    return (bool_t)(arrpt_size(status->errors, String) > 0);
}

/*---------------------------------------------------------------------------*/

void html5_status_clean_errors(Html5Status *status)
{
    cassert_no_null(status);
    arrpt_clear(status->errors, str_destroy, String);
}
