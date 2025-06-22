/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: latex.h
 *
 */

/* LaTeX generator */

#include "ndoc.hxx"

void latex_tokens(const char_t *output_dir, const Config *config, const uint32_t lang_id, const Loader *loader);

bool_t latex_preamble(Stream *latex, const char_t *project_vers, const Config *config, const uint32_t lang_id, const ResPack *pack, const char_t *docpath, const char_t *res_dir);

bool_t latex_cover(Stream *latex, const Config *config, const uint32_t lang_id, const char_t *docpath, const char_t *res_dir);

void latex_part(Stream *latex, const char_t *title);

void latex_h1(Stream *latex, const Block *block, DocParser *parser);

void latex_epig(Stream *latex, const Block *block, DocParser *parser);

void latex_section(Stream *latex, const Block *block, const char_t *secname, const char_t *secref, DocParser *parser);

void latex_parag(Stream *latex, const Block *block, DocParser *parser);

void latex_bq(Stream *latex, const Block *block, DocParser *parser);

void latex_li(Stream *latex, const Block *block, const Config *config, const char_t *docpath, const char_t *prespath, DocParser *parser);

void latex_lili(Stream *latex, const Block *block, const Config *config, const char_t *docpath, const char_t *prespath, DocParser *parser);

void latex_liend(Stream *latex, DocParser *parser);

void latex_img(Stream *latex, const Block *block, const char_t *docpath, const char_t *prespath, DocParser *parser);

void latex_img2(Stream *latex, const Block *block, const char_t *docpath, const char_t *prespath, DocParser *parser);

void latex_table(Stream *latex, const Block *block, const char_t *docpath, const char_t *prespath, DocParser *parser);

void latex_code(Stream *latex, const Block *block, DocParser *parser);

void latex_math(Stream *latex, const Block *block, DocParser *parser);

void latex_type(Stream *latex, const Block *type, DocParser *parser);

void latex_const(Stream *latex, const Block *type, DocParser *parser);

void latex_enum(Stream *latex, const Block *type, DocParser *parser);

void latex_struct(Stream *latex, const Block *sblock, DocParser *parser);

void latex_func(Stream *latex, const Block *func, DocParser *parser);
