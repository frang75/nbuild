/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: head.c
 *
 */

/* Html5 Head */

#include "head.h"
#include "head.inl"
#include "wsite.inl"
#include <core/event.h>
#include <core/heap.h>
#include <core/stream.h>
#include <core/strings.h>
#include <sewer/cassert.h>

struct _head_t
{
    WSite *site;
    String *favicon;
    String *analytics;
};

/*---------------------------------------------------------------------------*/

Head *head_create(const char_t *favicon, const char_t *analytics)
{
    Head *head = heap_new0(Head);
    head->favicon = str_c(favicon);
    head->analytics = str_c(analytics);
    return head;
}

/*---------------------------------------------------------------------------*/

void head_destroy(Head **head)
{
    cassert_no_null(head);
    cassert_no_null(*head);
    str_destroy(&(*head)->analytics);
    str_destroy(&(*head)->favicon);
    heap_delete(head, Head);
}

/*---------------------------------------------------------------------------*/

void head_site(Head *head, WSite *site, Html5Status *status)
{
    cassert_no_null(head);
    head->site = site;
    wsite_add_res(site, tc(head->favicon), status);
}

/*---------------------------------------------------------------------------*/

void head_html(const Head *head, Stream *html, Listener *listener)
{
    cassert_no_null(head);
    stm_writef(html, "<head>\n");
    stm_writef(html, "<meta charset='utf-8'>\n");
    stm_writef(html, "<meta name='viewport' content='width=device-width, initial-scale=1.0'>\n");
    stm_writef(html, "<meta name='robots' content='NOODP'>\n");
    listener_event(listener, ekPAGE_META_DESC, head, html, NULL, Head, Stream, void);
    stm_writef(html, "<link rel='stylesheet' href='");
    wsite_stm_res(head->site, html);
    stm_printf(html, "style_%s.css'>\n", wsite_lang(head->site));
    stm_writef(html, "<link rel='icon' href='");
    wsite_stm_res(head->site, html);
    stm_writef(html, tc(head->favicon));
    stm_writef(html, "'>\n");

    stm_writef(html, "<title>");
    listener_event(listener, ekPAGE_TITLE, head, html, NULL, Head, Stream, void);
    stm_writef(html, " - ");
    stm_writef(html, wsite_prjname(head->site));
    stm_writef(html, "</title>\n");

    if (str_empty(head->analytics) == FALSE)
    {
        stm_writef(html, "\n");
        stm_writef(html, "<!-- Google tag (gtag.js) -->\n");
        stm_printf(html, "<script async src=\"https://www.googletagmanager.com/gtag/js?id=%s\"></script>\n", tc(head->analytics));
        stm_writef(html, "<script>\n");
        stm_writef(html, "window.dataLayer = window.dataLayer || [];\n");
        stm_writef(html, "function gtag(){dataLayer.push(arguments);}\n");
        stm_writef(html, "gtag('js', new Date());\n");
        stm_printf(html, "gtag('config', '%s');\n", tc(head->analytics));
        stm_printf(html, "</script>\n");
        stm_writef(html, "\n");
    }

    stm_writef(html, "</head>\n");
}
