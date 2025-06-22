/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: target.h
 *
 */

/* Preprocess, copy and compress targets sources */

#include "nbuild.hxx"

String *target_clang_format_file(const ArrSt(Target) *targets, const char_t *repo_url, const char_t *repo_user, const char_t *repo_pass, const uint32_t repo_vers, const char_t *cwd);

bool_t target_target(const Target *target, const Global *global, const ArrPt(RegEx) *ignore_regex, const uint32_t repo_vers, const char_t *format_file, const char_t *dest_path, const char_t *groupid, REvent *event, Report *report, bool_t *formatted, bool_t *legalized);

bool_t target_build_file(const char_t *build, const uint32_t repo_vers, const WorkPaths *wpaths, Report *report);

bool_t target_tar(const Login *drive, const WorkPaths *wpaths, const char_t *srcdir, const char_t *tarname, REvent *event, Report *report);
