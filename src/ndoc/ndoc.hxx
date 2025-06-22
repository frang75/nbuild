/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: ndoc.hxx
 *
 */

/* NAppGUI Doc */

#ifndef __NDOC_HXX__
#define __NDOC_HXX__

#include <nlib/nlib.hxx>
#include <html5/html5.hxx>

typedef struct _lang_t Lang;
typedef struct _ebpart_t EBPart;
typedef struct _websec_t WebSec;
typedef struct _config_t Config;
typedef struct _loader_t Loader;
typedef struct _doc_t Doc;
typedef struct _dindex_t DIndex;
typedef struct _docparser_t DocParser;
typedef struct _tag_t Tag;
typedef struct _block_t Block;

typedef enum _btype_t
{
    ekBDRAFT = 1,
    ekBLANGREV,
    ekBCOMPLETE,
    ekEBOOK_ONLY,
    ekWEB_ONLY,
    ekNOTOC,
    ekNOSECNUM,
    ekEPIG,
    ekDESC,
    ekH1,
    ekH2,
    ekH3,
    ekPARAG,
    ekIMG,
    ekIMG2,
    ekTABLE,
    ekROW,
    ekLI,
    ekLILI,
    ekBQ,
    ekCODE,
    ekCODEFILE,
    ekMATH,
    ekTYPE,
    ekCONST,
    ekENUM,
    ekENUMV,
    ekSTRUCT,
    ekSMEMBER,
    ekFUNC,
    ekFPAR,
    ekFRET,
    ekFCODE,
    ekFNOTE,
    ekWARN,
    ekNOTAG
} btype_t;

typedef enum _ttype_t
{
    ekLESS,
    ekGREATER,
    ekBOLD_OPEN,
    ekBOLD_CLOSE,
    ekITALIC_OPEN,
    ekITALIC_CLOSE,
    ekCODE_OPEN,
    ekCODE_CLOSE,
    ekSUB_OPEN,
    ekSUB_CLOSE,
    ekSUP_OPEN,
    ekSUP_CLOSE,
    ekUNDER_OPEN,
    ekUNDER_CLOSE,
    ekSTRIKE_OPEN,
    ekSTRIKE_CLOSE,
    ekREF_OPEN,
    ekREF_CLOSE,
    ekMATH_OPEN,
    ekMATH_CLOSE,
    ekLINK_OPEN,
    ekLINK_CLOSE,
    ekLFUNC_OPEN,
    ekLFUNC_CLOSE,
    ekLTYPE_OPEN,
    ekLTYPE_CLOSE,
    ekLPAGE_OPEN,
    ekLPAGE_CLOSE,
    ekLHEAD_OPEN,
    ekLHEAD_CLOSE,
    ekPLAINTEXT
} ttype_t;

typedef enum _ptype_t
{
    ekFLOAT,
    ekDOUBLE,
    ekREAL,
    ekOTHER
} ptype_t;

struct _lang_t
{
    String *lang;
    bool_t build;
};

struct _ebpart_t
{
    bool_t build;
    ArrPt(String) *title;
    ArrPt(String) *packs;
};

struct _websec_t
{
    bool_t build;
    ArrPt(String) *packs;
    ArrPt(String) *menu;
    ArrPt(String) *hover;
    ArrPt(String) *url;
};

struct _config_t
{
    ArrSt(Lang) *langs;
    String *project_name;
    String *project_author;
    uint32_t project_start_year;
    String *project_url;
    String *project_email;
    ArrPt(String) *project_brief;
    ArrPt(String) *project_license;
    ArrPt(String) *project_license_url;
    ArrPt(String) *project_legal_url;
    ArrPt(String) *project_license_ebook;
    ArrPt(String) *support_email;
    String *remote_repo;

    bool_t ebook;
    uint8_t ebook_funcs_mode;
    String *ebook_section_color;
    String *ebook_bqback_color;
    String *ebook_bqline_color;
    String *ebook_funcs_color;
    String *ebook_types_color;
    String *ebook_constants_color;
    ArrPt(String) *ebook_title;
    ArrPt(String) *ebook_subtitle;
    ArrPt(String) *ebook_langrev;
    ArrPt(String) *ebook_chapters;
    ArrSt(EBPart) *ebook_parts;

    bool_t web;
    String *web_site_font;
    String *web_head_font;
    String *web_mono_font;
    real32_t web_lnav_width;
    real32_t web_post_width;
    real32_t web_post_def_imgwidth;
    real32_t web_rcol_width;
    String *web_back_color;
    String *web_text_color;
    String *web_title_color;
    String *web_coltext_color;
    String *web_colback_color;
    String *web_sec_color;
    String *web_secback_color;
    String *web_ter_color;
    String *web_over_color;
    String *web_navback_color;
    String *web_navtext_color;
    String *web_navhover_color;
    String *web_funcs_color;
    String *web_types_color;
    String *web_constants_color;
    String *web_analytics;
    ArrPt(String) *web_langrev;
    ArrSt(WebSec) *web_sections;
};

struct _dindex_t
{
    String *name;
    Doc *doc;
    btype_t type;
    uint32_t block_id;
    uint32_t child_id;
};

struct _docparser_t
{
    const Loader *loader;
    const Config *config;
    const WebSec *section;
    const ResPack *respack;
    const Doc *doc;
    uint32_t lang_id;
    const char_t *docpath;
    uint32_t line;
    uint32_t item_level;
    const Post *post;
    const SCode *scode;
    const Doc *scode_doc;
    const Block *scode_block;
    Listener *listener;
    Html5Status *status;
};

struct _tag_t
{
    ttype_t type;
    String *text;
};

struct _block_t
{
    btype_t type;
    uint32_t line;
    ArrPt(String) *params;
    ArrSt(Tag) *tags;
    ArrSt(Tag) *alt1;
    ArrSt(Tag) *alt2;
    ArrSt(Block) *children;
    String *ref;
    Stream *code;
    bool_t is_const;
    bool_t is_ptr;
    bool_t is_dptr;
    String *ptype;
};

DeclSt(Lang);
DeclSt(EBPart);
DeclSt(WebSec);
DeclSt(DIndex);
DeclSt(DocParser);
DeclSt(Tag);
DeclSt(Block);
DeclPt(Doc);
DeclPt(Loader);

#endif
