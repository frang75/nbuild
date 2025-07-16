/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: target.c
 *
 */

/* Preprocess, copy and compress targets sources */

#include "target.h"
#include "report.h"
#include "nbuild.h"
#include <nlib/nlib.h>
#include <nlib/ssh.h>
#include <inet/httpreq.h>
#include <core/arrpt.h>
#include <core/arrst.h>
#include <core/date.h>
#include <core/hfile.h>
#include <core/regex.h>
#include <core/stream.h>
#include <core/strings.h>
#include <osbs/bproc.h>
#include <osbs/log.h>
#include <sewer/cassert.h>

/*---------------------------------------------------------------------------*/

String *target_clang_format_file(const ArrSt(Target) *targets, const char_t *repo_url, const char_t *repo_user, const char_t *repo_pass, const uint32_t repo_vers, const char_t *cwd)
{
    String *file = NULL;
    /* clang-format needs .clang-format file in current working directory */
    arrst_foreach_const(target, targets, Target)
        String *filename = NULL;
        str_split_pathname(tc(target->name), NULL, &filename);
        if (str_equ(filename, ".clang-format") == TRUE)
        {
            String *url = str_printf("%s/%s", repo_url, tc(target->name));
            Stream *stm = ssh_repo_cat(tc(url), repo_vers, repo_user, repo_pass);
            if (stm != NULL)
            {
                const byte_t *data = stm_buffer(stm);
                uint32_t size = stm_buffer_size(stm);
                file = str_cpath("%s/.clang-format", cwd);
                if (hfile_from_data(tc(file), data, size, NULL) == FALSE)
                    str_destroy(&file);
                stm_close(&stm);
            }
            else
            {
                log_printf("%s can't access to '%s'", kASCII_FAIL, tc(url));
            }

            str_destroy(&url);
        }
        str_destroy(&filename);
    arrst_end()
    return file;
}

/*---------------------------------------------------------------------------*/

static bool_t i_is_dir(const char_t *filename)
{
    uint32_t n = str_len_c(filename);
    if (n > 2)
    {
        if (filename[n - 1] == '/')
            return TRUE;
    }

    return FALSE;
}

/*---------------------------------------------------------------------------*/

static bool_t i_local_dir(const char_t *dir)
{
    file_type_t ftype = ENUM_MAX(file_type_t);
    if (hfile_exists(dir, &ftype) == FALSE)
    {
        if (hfile_dir_create(dir, NULL) == TRUE)
        {
            return TRUE;
        }
        else
        {
            log_printf("%s Creating directory '%s'", kASCII_FAIL, dir);
            return FALSE;
        }
    }
    else
    {
        if (ftype == ekDIRECTORY)
        {
            return TRUE;
        }

        log_printf("%s Directory exists as file '%s'", kASCII_FAIL, dir);
        return FALSE;
    }
}

/*---------------------------------------------------------------------------*/

static bool_t i_is_source_file(const char_t *ext)
{
    const char_t *src_files[] = {"h", "hxx", "hpp", "inl", "ixx", "ipp", "c", "cpp", "m", "def"};
    uint32_t i, n = sizeof32(src_files) / sizeof32(const char_t *);

    for (i = 0; i < n; ++i)
    {
        if (str_equ_c(src_files[i], ext) == TRUE)
            return TRUE;
    }

    return FALSE;
}

/*---------------------------------------------------------------------------*/

static bool_t i_ignore_file(const ArrPt(RegEx) *ignore_regex, const char_t *src)
{
    if (ignore_regex != NULL)
    {
        arrpt_foreach_const(regex, ignore_regex, RegEx)
            if (regex_match(regex, src) == TRUE)
                return TRUE;
        arrpt_end()
    }

    return FALSE;
}
/*---------------------------------------------------------------------------*/

static bool_t i_copy_repo_file(const Global *global, const ArrPt(RegEx) *ignore_regex, const char_t *repo_url, const char_t *src, const char_t *dest, const char_t *file_doc_url, const uint32_t repo_vers, const char_t *repo_user, const char_t *repo_pass, const bool_t with_legal, const char_t *clang_format, bool_t *formatted, bool_t *legalized, String **error_msg)
{
    if (i_ignore_file(ignore_regex, src) == FALSE)
    {
        bool_t ok = TRUE;
        String *fileurl = NULL;
        Stream *filestm = NULL;
        String *path = NULL, *filename = NULL, *ext = NULL;

        cassert_no_null(global);
        cassert_no_null(error_msg);
        cassert(*error_msg == NULL);
        cassert_no_null(formatted);
        cassert_no_null(legalized);

        /* Download file from repo */
        fileurl = str_printf("%s/%s", repo_url, src);
        filestm = ssh_repo_cat(tc(fileurl), repo_vers, repo_user, repo_pass);
        if (filestm == NULL)
        {
            ok = FALSE;
            *error_msg = str_printf("Error download '%s'", src);
        }

        /* Create the destiny directory */
        if (ok == TRUE)
        {
            const char_t *e = NULL;
            str_split_pathname(dest, &path, &filename);
            e = str_filext(tc(filename));
            ext = str_c(e != NULL ? e : "");
            ok = i_local_dir(tc(path));
            if (ok == FALSE)
                *error_msg = str_printf("Error creating '%s'", tc(path));
        }

        /* Add legal header */
        if (ok == TRUE && with_legal == TRUE && i_is_source_file(tc(ext)) == TRUE)
        {
            const byte_t *fdata = stm_buffer(filestm);
            uint32_t fsize = stm_buffer_size(filestm);
            Stream *stm = stm_memory(fsize + 512);
            uint16_t year = (uint16_t)date_year();
            stm_writef(stm, "/*\n");
            stm_printf(stm, " * %s %s\n", tc(global->project), tc(global->description));

            if (year == global->start_year)
                stm_printf(stm, " * %d %s\n", global->start_year, tc(global->author));
            else
                stm_printf(stm, " * %d-%d %s\n", global->start_year, year, tc(global->author));

            if (arrpt_size(global->license, String) > 0)
            {
                arrpt_foreach(line, global->license, String)
                    stm_writef(stm, " * ");
                    str_writef(stm, line);
                    stm_writef(stm, "\n");
                arrpt_end()
            }

            stm_writef(stm, " *\n");
            stm_printf(stm, " * File: %s\n", tc(filename));

            if (str_empty(global->doc_url) == FALSE && str_empty_c(file_doc_url) == FALSE)
            {
                if (str_equ(ext, "h") == TRUE || str_equ(ext, "hxx") == TRUE || str_equ(ext, "hpp") == TRUE)
                {
                    String *file = NULL;
                    String *url = NULL;
                    str_split_pathext(src, NULL, &file, NULL);
                    url = str_printf("%s/%s/%s.html", tc(global->doc_url), file_doc_url, tc(file));
                    if (http_exists(tc(url)) == TRUE)
                        stm_printf(stm, " * %s\n", tc(url));
                    str_destroy(&file);
                    str_destroy(&url);
                }
            }

            stm_writef(stm, " *\n");
            stm_writef(stm, " */\n\n");
            stm_write(stm, fdata, fsize);

            /* Change the stream */
            stm_close(&filestm);
            filestm = stm;
            *legalized = TRUE;
        }

        /* Clang-format the file */
        if (ok == TRUE && str_empty_c(clang_format) == FALSE && i_is_source_file(tc(ext)) == TRUE)
        {
            String *cmd = str_printf("clang-format -style=file -assume-filename='%s'", tc(filename));
            const byte_t *fdata = stm_buffer(filestm);
            uint32_t fsize = stm_buffer_size(filestm);
            Proc *proc = bproc_exec(tc(cmd), NULL);
            Stream *stm = NULL;

            if (proc != NULL)
            {
                /* Write the file in clang-format stdin */
                bool_t okc = bproc_write(proc, fdata, fsize, NULL, NULL);
                if (okc == TRUE)
                    okc = bproc_write_close(proc);

                if (okc == TRUE)
                {
                    /* Read from the clang-format stdout */
                    byte_t buffer[512];
                    uint32_t rsize;
                    stm = stm_memory(2048);
                    bproc_write_close(proc);
                    while (bproc_read(proc, buffer, 512, &rsize, NULL) == TRUE)
                        stm_write(stm, buffer, rsize);
                }

                bproc_eread_close(proc);
                bproc_wait(proc);
                bproc_close(&proc);
            }

            /* Change the file to newly formatted file */
            if (stm != NULL)
            {
                stm_close(&filestm);
                filestm = stm;
            }

            str_destroy(&cmd);
            *formatted = TRUE;
        }

        if (ok == TRUE)
        {
            const byte_t *data = stm_buffer(filestm);
            uint32_t size = stm_buffer_size(filestm);
            ok = hfile_from_data(dest, data, size, NULL);
            if (ok == FALSE)
                *error_msg = str_printf("Error copying '%s'", dest);
        }

        str_destopt(&fileurl);
        str_destopt(&path);
        str_destopt(&filename);
        str_destopt(&ext);

        if (filestm != NULL)
            stm_close(&filestm);

        return ok;
    }
    /* The file matchs an ignore pattern */
    else
    {
        return TRUE;
    }
}

/*---------------------------------------------------------------------------*/

static bool_t i_copy_repo_dir(const Global *global, const ArrPt(RegEx) *ignore_regex, const char_t *repo_url, const char_t *src, const char_t *dest, const char_t *file_doc_url, const uint32_t repo_vers, const char_t *repo_user, const char_t *repo_pass, const bool_t with_legal, const char_t *clang_format, bool_t *formatted, bool_t *legalized, String **error_msg)
{
    bool_t ok = TRUE;
    String *repo_dir = NULL;
    Stream *stm = NULL;
    cassert_no_null(error_msg);
    cassert(*error_msg == NULL);

    /* Get repository list */
    repo_dir = str_printf("%s/%s", repo_url, src);
    stm = ssh_repo_list(tc(repo_dir), repo_vers, repo_user, repo_pass);
    if (stm == NULL)
    {
        ok = FALSE;
        *error_msg = str_printf("Error repo list '%s'", tc(repo_dir));
    }

    /* Recursive copy of files and subdirs */
    if (ok == TRUE)
    {
        stm_lines(filename, stm)
            String *dir_src = str_printf("%s/%s", src, filename);
            String *dir_dest = str_cpath("%s/%s", dest, filename);

            if (i_is_dir(filename) == TRUE)
            {
                String *nsrc = str_cn(tc(dir_src), str_len(dir_src) - 1);
                String *ndest = str_cn(tc(dir_dest), str_len(dir_dest) - 1);
                ok = i_copy_repo_dir(global, ignore_regex, repo_url, tc(nsrc), tc(ndest), file_doc_url, repo_vers, repo_user, repo_pass, with_legal, clang_format, formatted, legalized, error_msg);
                str_destroy(&nsrc);
                str_destroy(&ndest);
            }
            else
            {
                ok = i_copy_repo_file(global, ignore_regex, repo_url, tc(dir_src), tc(dir_dest), file_doc_url, repo_vers, repo_user, repo_pass, with_legal, clang_format, formatted, legalized, error_msg);
            }

            str_destroy(&dir_src);
            str_destroy(&dir_dest);

            if (ok == FALSE)
                break;

        stm_next(filename, stm)
    }

    str_destroy(&repo_dir);
    if (stm != NULL)
        stm_close(&stm);

    return ok;
}

/*---------------------------------------------------------------------------*/

bool_t target_target(const Target *target, const Global *global, const ArrPt(RegEx) *ignore_regex, const uint32_t repo_vers, const char_t *format_file, const char_t *dest_path, const char_t *groupid, REvent *event, Report *report, bool_t *formatted, bool_t *legalized)
{
    bool_t ok = TRUE;
    RState state;
    cassert_no_null(target);

    report_event_state(report, event, &state);
    cassert(state.done == FALSE);

    {
        String *msg = str_printf("%s '%s%s%s'", groupid, kASCII_TARGET, tc(target->name), kASCII_RESET);
        String *repo_base = str_printf("%s/%s", tc(global->repo_url), tc(global->repo_branch));
        String *src = str_printf("%s/%s", tc(repo_base), tc(target->name));
        String *dest = NULL;
        String *error_msg = NULL;
        const char_t *format = NULL;

        if (target->format == TRUE && str_empty_c(format_file) == FALSE)
            format = format_file;

        if (str_empty(target->dest) == TRUE)
            dest = str_cpath("%s/%s", dest_path, tc(target->name));
        else
            dest = str_cpath("%s/%s", dest_path, tc(target->dest));

        log_printf("%s %s. Starting copy", kASCII_OK, tc(msg));
        report_event_init(report, event);

        if (ssh_repo_is_dir(tc(src), repo_vers, tc(global->repo_user), tc(global->repo_pass)) == TRUE)
            ok = i_copy_repo_dir(global, ignore_regex, tc(repo_base), tc(target->name), tc(dest), tc(target->url), repo_vers, tc(global->repo_user), tc(global->repo_pass), target->legal, format, formatted, legalized, &error_msg);
        else
            ok = i_copy_repo_file(global, ignore_regex, tc(repo_base), tc(target->name), tc(dest), tc(target->url), repo_vers, tc(global->repo_user), tc(global->repo_pass), target->legal, format, formatted, legalized, &error_msg);

        report_event_end(report, event, ok, &error_msg);
        report_event_state(report, event, &state);
        report_state_log(&state, tc(msg));
        str_destroy(&src);
        str_destroy(&dest);
        str_destroy(&repo_base);
        str_destroy(&msg);
    }

    return ok;
}

/*---------------------------------------------------------------------------*/

bool_t target_build_file(const char_t *build, const uint32_t repo_vers, const WorkPaths *wpaths, Report *report)
{
    bool_t ok = TRUE;
    cassert_no_null(wpaths);
    if (str_empty_c(build) == FALSE)
    {
        RState state;
        report_build_file_state(report, &state);

        if (state.done == FALSE)
        {
            String *msg = str_printf("'%s%s%s'", kASCII_TARGET, build, kASCII_RESET);
            String *pathname = str_cpath("%s/%s", tc(wpaths->tmp_src), build);
            String *path = NULL;
            String *error_msg = NULL;

            log_printf("%s %s. Starting copy", kASCII_OK, tc(msg));
            report_build_file_init(report);
            str_split_pathname(build, &path, NULL);
            ok = i_local_dir(tc(path));
            if (ok == TRUE)
            {
                String *version = str_printf("%d\n", repo_vers);
                ok = hfile_from_string(tc(pathname), version, NULL);
                str_destroy(&version);
            }

            if (ok == FALSE)
            {
                error_msg = str_printf("Creating '%s'", tc(pathname));
                log_printf("%s %s", kASCII_FAIL, tc(error_msg));
            }

            report_build_file_end(report, ok, &error_msg);
            report_build_file_state(report, &state);
            report_state_log(&state, tc(msg));
            str_destroy(&path);
            str_destroy(&pathname);
            str_destroy(&msg);
        }
    }

    return ok;
}

/*---------------------------------------------------------------------------*/

bool_t target_tar(const Login *drive, const WorkPaths *wpaths, const char_t *srcdir, const char_t *tarname, REvent *event, Report *report)
{
    bool_t ok = TRUE;
    RState state;
    report_event_state(report, event, &state);

    if (state.done == FALSE)
    {
        String *msg = str_printf("'%s%s%s'", kASCII_TARGET, tarname, kASCII_RESET);
        String *tarpath = NULL;
        String *error_msg = NULL;
        tarpath = str_printf("%s/%s", tc(wpaths->tmp_path), tarname);
        log_printf("%s %s. Starting compressing.", kASCII_OK, tc(msg));
        report_event_init(report, event);
        ok = ssh_cmake_tar(NULL, srcdir, tc(tarpath));

        /* Once compressed, we move the .tar to drive node */
        if (ok == TRUE)
        {
            ok = ssh_copy(NULL, tc(wpaths->tmp_path), tarname, drive, tc(wpaths->drive_path), tarname);
            if (ok == FALSE)
                error_msg = str_printf("Error moving '%s' to drive", tc(tarpath));
        }
        else
        {
            error_msg = str_printf("Error generating '%s'", tc(tarpath));
        }

        report_event_end(report, event, ok, &error_msg);
        report_event_state(report, event, &state);
        report_state_log(&state, tc(msg));
        str_destroy(&tarpath);
        str_destroy(&msg);
    }

    return ok;
}
