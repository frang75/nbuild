/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: web.h
 *
 */

/* Web generator */

#include "ndoc.hxx"

WSite *web_start(const char_t *docpath, const char_t *webpath, const char_t *tmppath, const Config *config);

void web_css(CSS *css, const Config *config);

void web_part(Stream *html, const char_t *title, const char_t *packname, const Doc *doc, const Doc *packdoc, LNav *lnav);

void web_doc_lnav(Stream *html, const Doc *curdoc, const Doc *doc, LNav *lnav);

void web_nav_buttons(Stream *html, DocParser *parser);

void web_tags(Stream *html, const Block *block, DocParser *parser);

void web_epig(Stream *html, const Block *block, DocParser *parser);

void web_meta_desc(Stream *html, const Block *block, DocParser *parser);

void web_h1(Stream *html, const Block *block, DocParser *parser);

void web_h2_plain(Stream *html, const Block *block, DocParser *parser);

void web_h2(Stream *html, const Block *block, DocParser *parser);

void web_h3_plain(Stream *html, const Block *block, DocParser *parser);

void web_h3(Stream *html, const Block *block, DocParser *parser);

void web_parag(Stream *html, const Block *block, DocParser *parser);

void web_bq(Stream *html, const Block *block, DocParser *parser);

void web_li(Stream *html, const Block *block, DocParser *parser);

void web_lili(Stream *html, const Block *block, DocParser *parser);

void web_img(Stream *html, const Block *block, DocParser *parser);

void web_img2(Stream *html, const Block *block, DocParser *parser);

void web_table(Stream *html, const Block *block, DocParser *parser);

void web_code(Stream *html, const Block *block, const char_t *clang, DocParser *parser);

void web_math(Stream *html, const Block *block, DocParser *parser);

void web_types_header(Stream *html, const ArrSt(Block) *types, DocParser *parser);

void web_types_body(Stream *html, const ArrSt(Block) *types, DocParser *parser);

void web_funcs_header(Stream *html, const ArrSt(Block) *funcs, DocParser *parser);

void web_funcs_body(Stream *html, const ArrSt(Block) *funcs, DocParser *parser);
