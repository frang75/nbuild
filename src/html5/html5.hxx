/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: html5.hxx
 *
 */

/* html5 */

#ifndef __HTML5_HXX__
#define __HTML5_HXX__

#include <draw2d/draw2d.hxx>

typedef struct _css_t CSS;
typedef struct _head_t Head;
typedef struct _header_t Header;
typedef struct _nav_t Nav;
typedef struct _lnav_t LNav;
typedef struct _post_t Post;
typedef struct _rcol_t RCol;
typedef struct _scode_t SCode;
typedef struct _wsite_t WSite;
typedef struct _identifier_t Identifier;
typedef struct _html5status_t Html5Status;

typedef enum _hcont_t
{
    ekPAGE_TITLE = 1,
    ekPAGE_TRANSLATE,
    ekPAGE_META_DESC,
    ekPAGE_CONTENT,
    ekLNAV_CONTENT,
    ekCSS_CONTENT,
    ekJS_CONTENT,
    ekCODE_IDENTIFIER,
    ekCODE_LINK,
    ekCODE_TITLE
} hcont_t;

typedef enum _itype_t
{
    ekID_KEYWORD = 1,
    ekID_TYPE,
    ekID_CONSTANT,
    ekID_FUNCTION,
    ekID_OTHER
} itype_t;

typedef enum _caption_t
{
    ekCAPTION_NO,
    ekCAPTION_FIXED,
    ekCAPTION_OPEN,
    ekCAPTION_CLOSED
} caption_t;

struct _identifier_t
{
    itype_t type;
    bool_t has_link;
    bool_t has_title;
};

#endif
