/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: head.inl
 *
 */

/* Html5 Head */

#include "html5.ixx"

__EXTERN_C

void head_destroy(Head **head);

void head_site(Head *head, WSite *site, Html5Status *status);

void head_html(const Head *head, Stream *html, Listener *listener);

__END_C
