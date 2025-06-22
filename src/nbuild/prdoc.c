/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: prdoc.c
 *
 */

/* Process project documentation */

#include "prdoc.h"
#include "report.h"
#include "nbuild.h"
#include <nlib/nlib.h>
#include <nlib/ssh.h>
#include <encode/base64.h>
#include <encode/json.h>
#include <core/arrst.h>
#include <core/arrpt.h>
#include <core/dbind.h>
#include <core/hfile.h>
#include <core/strings.h>
#include <core/stream.h>
#include <osbs/bfile.h>
#include <osbs/log.h>
#include <sewer/cassert.h>

typedef struct _confjson_t ConfJson;
typedef struct _webjson_t WebJson;

struct _webjson_t
{
    ArrPt(String) *title;
    ArrPt(String) *docs;
};

struct _confjson_t
{
    ArrSt(WebJson) *web;
};

DeclSt(WebJson);

/*---------------------------------------------------------------------------*/

void prdoc_dbind(void)
{
    dbind(WebJson, ArrPt(String) *, title);
    dbind(WebJson, ArrPt(String) *, docs);
    dbind(ConfJson, ArrSt(WebJson) *, web);
}

/*---------------------------------------------------------------------------*/

static bool_t i_stm_with_data(const Stream *stm)
{
    if (stm == NULL)
        return FALSE;

    if (stm_buffer_size(stm) > 1)
        return TRUE;
    else
        return FALSE;
}

/*---------------------------------------------------------------------------*/

static String *i_ndoc_cmd(
    const char_t *project_vers,
    const char_t *doc_repo_url,
    const char_t *doc_repo_user,
    const char_t *doc_repo_pass,
    const uint32_t doc_repo_vers,
    const char_t *repo_url,
    const char_t *repo_user,
    const char_t *repo_pass,
    const uint32_t repo_vers,
    const char_t *ndoc_outpath)
{
    char_t exec_path[256];
    String *path = NULL, *ndoc = NULL, *cmd = NULL;
    Stream *stm = stm_memory(512);
    /*
    "args": [ "-v", "version",
              "-r", "svn://192.168.1.2/svn/NAPPGUI/trunk/doc", "franRED", "vOBa3Wiw5HcUN6FSAzec", "0",
              "-s", "svn://192.168.1.2/svn/NAPPGUI/trunk", "franRED", "vOBa3Wiw5HcUN6FSAzec", "0",
              "-o", "/home/fran/nbuild_temp/ndoc"],
    */
    bfile_dir_exec(exec_path, sizeof(exec_path));
    str_split_pathname(exec_path, &path, NULL);
    ndoc = str_cpath("%s/%s", tc(path), NDOC_APP);
    stm_writef(stm, tc(ndoc));
    stm_writef(stm, " ");
    stm_printf(stm, "-v %s.%d", project_vers, doc_repo_vers);
    stm_writef(stm, " ");
    stm_printf(stm, "-r %s %s %s %d", doc_repo_url, doc_repo_user, doc_repo_pass, doc_repo_vers);
    stm_writef(stm, " ");
    stm_printf(stm, "-s %s %s %s %d", repo_url, repo_user, repo_pass, repo_vers);
    stm_writef(stm, " ");
    stm_printf(stm, "-o %s", ndoc_outpath);
    cmd = stm_str(stm);
    stm_close(&stm);
    str_destopt(&path);
    str_destopt(&ndoc);
    return cmd;
}

/*---------------------------------------------------------------------------*/

static bool_t i_ebook_event(const char_t *lang, const char_t *project_name, const uint32_t doc_repo_vers, const char_t *tmp_ndoc, Report *report)
{
    String *tex_path = str_cpath("%s/tex", tmp_ndoc);
    String *tex_file = str_cpath("%s/%s/ndoc_%s.tex", tc(tex_path), lang, lang);
    String *msg = str_printf("%s ebook '%s%s%s' lang", project_name, kASCII_VERSION, lang, kASCII_RESET);
    bool_t ok = TRUE;
    String *error_msg = NULL;
    RState state;

    report_doc_ebook_init(report, doc_repo_vers, lang);
    if (hfile_exists(tc(tex_file), NULL) == TRUE)
    {
        char_t name_lower[128];
        String *cmdtex = str_cpath("cd %s/%s && xelatex -synctex=1 -interaction=nonstopmode ndoc_%s.tex", tc(tex_path), lang, lang);
        String *cmdidx = str_cpath("cd %s/%s && makeindex ndoc_%s.idx", tc(tex_path), lang, lang);
        uint32_t ret = UINT32_MAX;

        log_printf("%s %s. Beginning", kASCII_OK, tc(msg));
        str_lower_c(name_lower, sizeof(name_lower), project_name);

        ret = ssh_command(tc(cmdtex), NULL, NULL);
        if (ret != 0)
        {
            ok = FALSE;
            error_msg = str_copy(cmdtex);
        }

        if (ok == TRUE)
        {
            ret = ssh_command(tc(cmdidx), NULL, NULL);
            if (ret != 0)
            {
                ok = FALSE;
                error_msg = str_copy(cmdidx);
            }
        }

        if (ok == TRUE)
        {
            ret = ssh_command(tc(cmdtex), NULL, NULL);
            if (ret != 0)
            {
                ok = FALSE;
                error_msg = str_copy(cmdtex);
            }
        }

        if (ok == TRUE)
        {
            ret = ssh_command(tc(cmdidx), NULL, NULL);
            if (ret != 0)
            {
                ok = FALSE;
                error_msg = str_copy(cmdidx);
            }
        }

        if (ok == TRUE)
        {
            ret = ssh_command(tc(cmdtex), NULL, NULL);
            if (ret != 0)
            {
                ok = FALSE;
                error_msg = str_copy(cmdtex);
            }
        }

        if (ok == TRUE)
        {
            String *from = str_cpath("%s/%s/ndoc_%s.pdf", tc(tex_path), lang, lang);
            String *to = str_cpath("%s/web/res/%s_%s.pdf", tmp_ndoc, name_lower, lang);
            ok = hfile_copy(tc(from), tc(to), NULL);
            if (ok == FALSE)
                error_msg = str_printf("Copying '%s'", tc(to));
            str_destroy(&from);
            str_destroy(&to);
        }

        str_destroy(&cmdtex);
        str_destroy(&cmdidx);
    }

    report_doc_ebook_end(report, doc_repo_vers, lang, ok, &error_msg);
    report_doc_ebook_state(report, doc_repo_vers, lang, &state);
    report_state_log(&state, tc(msg));
    str_destroy(&msg);
    str_destroy(&tex_path);
    str_destroy(&tex_file);
    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_upload_website(const Global *global, const char_t *local_path, const char_t *hosting_path, String **error_msg)
{
    bool_t ok = TRUE;
    Login login;
    cassert_no_null(global);
    cassert(str_empty(global->hosting_url) == FALSE);
    cassert_no_null(error_msg);

    log_printf("%s Begin upload '%s%s%s' to '%s%s:%s%s'", kASCII_OK, kASCII_PATH, local_path, kASCII_RESET, kASCII_TARGET, tc(global->hosting_url), hosting_path, kASCII_RESET);

    /* Connexion with hosting server */
    login.ip = global->hosting_url;
    login.user = global->hosting_user;
    login.pass = global->hosting_pass;
    login.localhost = FALSE;
    login.platform = ekLINUX;
    login.use_sshpass = !global->hosting_cert;

    /* Set website permissions */
    if (ok == TRUE)
    {
        String *cmd = str_printf("chmod -R 777 %s", local_path);
        uint32_t ret = ssh_command(tc(cmd), NULL, NULL);
        if (ret != 0)
        {
            *error_msg = str_printf("Error chmod in '%s'", local_path);
            ok = FALSE;
        }

        str_destroy(&cmd);
    }

    /* Create the hosting destiny path */
    if (ok == TRUE)
    {
        ok = ssh_create_dir(&login, hosting_path);
        if (ok == FALSE)
            *error_msg = str_printf("Creating hosting path '%s'", hosting_path);
    }

    /* Upload the website */
    if (ok == TRUE)
    {
        String *from_path = str_cpath("%s/*", local_path);
        ok = ssh_upload(tc(from_path), &login, hosting_path, TRUE);
        if (ok == FALSE)
            *error_msg = str_printf("Uploading website in hosting path '%s'", hosting_path);
        str_destroy(&from_path);
    }

    if (ok == TRUE)
        log_printf("%s Uploaded '%s%s%s' to '%s%s:%s%s'", kASCII_OK, kASCII_PATH, local_path, kASCII_RESET, kASCII_TARGET, tc(global->hosting_url), tc(global->hosting_buildpath), kASCII_RESET);
    else
        log_printf("%s %s", kASCII_FAIL, tc(*error_msg));

    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_upload_doc_website(const Global *global, const uint32_t doc_repo_vers, const char_t *tmp_ndoc, String **error_msg)
{
    bool_t ok = TRUE;
    String *local_path = NULL;
    String *hosting_path = NULL;
    cassert_no_null(global);
    local_path = str_cpath("%s/web", tmp_ndoc);
    hosting_path = str_path(ekLINUX, "%s/r%d", tc(global->hosting_docpath), doc_repo_vers);
    ok = i_upload_website(global, tc(local_path), tc(hosting_path), error_msg);
    str_destroy(&local_path);
    str_destroy(&hosting_path);
    return ok;
}

/*---------------------------------------------------------------------------*/

static String *i_ndoc_parse(const String *stdout_, const char_t *word, uint32_t *n)
{
    Stream *stm = stm_from_block((const byte_t *)tc(stdout_), str_len(stdout_));
    Stream *out = stm_memory(1024);
    String *str = NULL;
    cassert_no_null(n);
    *n = 0;
    stm_lines(line, stm)
        if (str_str(line, word) != NULL)
        {
            stm_writef(out, line);
            stm_writef(out, "\n");
            *n += 1;
        }
    stm_next(line, stm)

    str = stm_str(out);
    stm_close(&stm);
    stm_close(&out);
    return str;
}

/*---------------------------------------------------------------------------*/

static bool_t i_generate_doc(const Global *global, const Login *drive, const char_t *project_vers, const uint32_t repo_vers, const uint32_t doc_repo_vers, const WorkPaths *wpaths, Report *report, String **error_msg)
{
    bool_t ok = TRUE;
    bool_t ok_en_ebook = TRUE;
    bool_t ok_es_ebook = TRUE;
    Stream *stdout_ = NULL;
    Stream *stderr_ = NULL;
    String *warns = NULL;
    String *errors = NULL;
    uint32_t nwarns = 0;
    uint32_t nerrors = 0;
    uint32_t ndoc_ret = 0;
    bool_t ndoc_cache = FALSE;
    String *hosting_url = NULL;
    cassert_no_null(error_msg);
    cassert(*error_msg == NULL);
    cassert_no_null(global);
    cassert_no_null(wpaths);

    /* Check if documentation is previously generated */
    if (ssh_file_exists(drive, tc(wpaths->drive_doc), "stdout.txt") == TRUE && ssh_file_exists(drive, tc(wpaths->drive_doc), "stderr.txt") == TRUE)
    {
        stdout_ = ssh_file_cat(drive, tc(wpaths->drive_doc), "stdout.txt");
        stderr_ = ssh_file_cat(drive, tc(wpaths->drive_doc), "stderr.txt");

        if (i_stm_with_data(stdout_) == FALSE)
        {
            ok = FALSE;
            cassert(*error_msg == NULL);
            *error_msg = str_printf("Error reading ndoc stdout.txt from '%s'", tc(wpaths->drive_doc));
        }

        if (ok == TRUE)
        {
            log_printf("%s Documentation '%s%d%s' in cache '%s%s%s'", kASCII_OK, kASCII_VERSION, doc_repo_vers, kASCII_RESET, kASCII_PATH, tc(wpaths->drive_doc), kASCII_RESET);
            ndoc_ret = 0;
            ndoc_cache = TRUE;
        }
    }
    /* Generate the documentation */
    else
    {
        /* Run 'ndoc' to generate docs */
        if (ok == TRUE)
        {
            String *repo_url = str_printf("%s/%s", tc(global->repo_url), tc(global->repo_branch));
            String *cmd = i_ndoc_cmd(project_vers, tc(global->doc_repo_url), tc(global->doc_repo_user), tc(global->doc_repo_pass), doc_repo_vers, tc(repo_url), tc(global->repo_user), tc(global->repo_pass), repo_vers, tc(wpaths->tmp_ndoc));
            String *msg = str_c("ndoc generator");
            String *ndoc_error_msg = NULL;
            RState state;

            log_printf("%s %s. Beginning", kASCII_OK, tc(msg));
            report_doc_ndoc_init(report, doc_repo_vers);
            ndoc_cache = FALSE;
            ndoc_ret = ssh_command(tc(cmd), &stdout_, &stderr_);
            if (ndoc_ret != 0)
            {
                ok = FALSE;
                cassert(*error_msg == NULL);
                ndoc_error_msg = str_printf("%s Error running ndoc process", kASCII_FAIL);
                *error_msg = str_printf("%s Error running '%s'", kASCII_FAIL, tc(cmd));
            }

            report_doc_ndoc_end(report, doc_repo_vers, ok, &ndoc_error_msg);
            report_doc_ndoc_state(report, doc_repo_vers, &state);
            report_state_log(&state, tc(msg));

            {
                String *out = stm_str(stdout_);
                warns = i_ndoc_parse(out, "[Warning]", &nwarns);
                errors = i_ndoc_parse(out, "[Error]", &nerrors);
                str_destroy(&out);
            }

            if (nerrors > 0 && nwarns > 0)
                log_printf("%s %s. %d errors and %d warnings", kASCII_WARN, tc(msg), nerrors, nwarns);
            else if (nerrors > 0)
                log_printf("%s %s. %d errors", kASCII_WARN, tc(msg), nerrors);
            else if (nwarns > 0)
                log_printf("%s %s. %d warnings", kASCII_WARN, tc(msg), nwarns);

            str_destroy(&repo_url);
            str_destroy(&cmd);
            str_destroy(&msg);
        }

        /* Generate ebook English */
        if (ok == TRUE && nwarns == 0 && nerrors == 0)
            ok_en_ebook = i_ebook_event("en", tc(global->project), doc_repo_vers, tc(wpaths->tmp_ndoc), report);

        /* Generate ebook Spanish */
        if (ok == TRUE && nwarns == 0 && nerrors == 0)
            ok_es_ebook = i_ebook_event("es", tc(global->project), doc_repo_vers, tc(wpaths->tmp_ndoc), report);

        /* Copy documentation event */
        if (ok == TRUE)
        {
            String *msg = str_c("Copy doc to drive");
            String *copy_error_msg = NULL;
            RState state;

            log_printf("%s %s. Beginning", kASCII_OK, tc(msg));
            report_doc_copy_init(report, doc_repo_vers);

            /* Compress the documentation */
            if (ok == TRUE)
            {
                String *websrc = str_cpath("%s/web", tc(wpaths->tmp_ndoc));
                String *tarpath = str_printf("%s/%s", tc(wpaths->tmp_ndoc), NBUILD_WEB_TAR);
                ok = ssh_cmake_tar(NULL, tc(websrc), tc(tarpath));
                if (ok == FALSE)
                    copy_error_msg = str_printf("%s Error compressing website to '%s'", kASCII_FAIL, tc(tarpath));

                str_destroy(&websrc);
                str_destroy(&tarpath);
            }

            /* Copy documentation package to drive */
            if (ok == TRUE)
            {
                ok = ssh_copy(NULL, tc(wpaths->tmp_ndoc), NBUILD_WEB_TAR, drive, tc(wpaths->drive_doc), NBUILD_WEB_TAR);
                if (ok == FALSE)
                    copy_error_msg = str_printf("%s Error copying '%s' to '%s'", kASCII_FAIL, NBUILD_WEB_TAR, tc(wpaths->drive_doc));
            }

            /* Copy ndoc stdout to drive */
            if (ok == TRUE)
            {
                ok = ssh_to_file(drive, tc(wpaths->drive_doc), "stdout.txt", stdout_);
                if (ok == FALSE)
                    copy_error_msg = str_printf("%s Error copying ndoc stdout to '%s'", kASCII_FAIL, tc(wpaths->drive_doc));
            }

            /* Copy ndoc stderr to drive */
            if (ok == TRUE)
            {
                ok = ssh_to_file(drive, tc(wpaths->drive_doc), "stderr.txt", stderr_);
                if (ok == FALSE)
                    copy_error_msg = str_printf("%s Error copying ndoc stderr to '%s'", kASCII_FAIL, tc(wpaths->drive_doc));
            }

            if (ok == FALSE)
            {
                cassert_no_null(copy_error_msg);
                cassert(*error_msg == NULL);
                *error_msg = str_copy(copy_error_msg);
            }

            report_doc_copy_end(report, doc_repo_vers, ok, &copy_error_msg);
            report_doc_copy_state(report, doc_repo_vers, &state);
            report_state_log(&state, tc(msg));
            str_destroy(&msg);
        }

        /* Upload documentation to hosting website */
        if (ok == TRUE)
        {
            if (str_empty(global->hosting_url) == FALSE)
            {
                String *msg = str_printf("Upload website to '%s%s:%s%s'", kASCII_TARGET, tc(global->hosting_url), tc(global->hosting_docpath), kASCII_RESET);
                String *upload_error_msg = NULL;
                RState state;
                log_printf("%s %s. Beginning", kASCII_OK, tc(msg));
                report_doc_upload_init(report, doc_repo_vers);
                ok = i_upload_doc_website(global, doc_repo_vers, tc(wpaths->tmp_ndoc), &upload_error_msg);

                if (ok == FALSE)
                {
                    cassert(*error_msg == NULL);
                    *error_msg = str_copy(upload_error_msg);
                }

                report_doc_upload_end(report, doc_repo_vers, ok, &upload_error_msg);
                report_doc_upload_state(report, doc_repo_vers, &state);
                report_state_log(&state, tc(msg));
                hosting_url = str_printf("%s/docs/r%d", tc(global->doc_url), doc_repo_vers);
                str_destroy(&msg);
            }
            else
            {
                log_printf("%s Documentation is not uploaded because no hosting data provided", kASCII_WARN);
            }
        }
    }

    /* Update documentation report */
    {
        String *stdout_b64 = NULL;
        String *stderr_b64 = NULL;
        String *warns_b64 = NULL;
        String *errors_b64 = NULL;

        if (warns == NULL)
        {
            cassert(nwarns == 0);
            warns = str_c("");
        }

        if (errors == NULL)
        {
            cassert(nerrors == 0);
            errors = str_c("");
        }

        if (hosting_url == NULL)
            hosting_url = str_c("");

        if (stdout_ != NULL)
        {
            stdout_b64 = b64_encode_from_stm(stdout_);
            stm_close(&stdout_);
        }
        else
        {
            stdout_b64 = str_c("");
        }

        if (stderr_ != NULL)
        {
            stderr_b64 = b64_encode_from_stm(stderr_);
            stm_close(&stderr_);
        }
        else
        {
            stderr_b64 = str_c("");
        }

        /* ebook errors */
        if (ok_en_ebook == FALSE)
        {
            nerrors += 1;
            str_cat(&errors, "Error generating 'en' ebook\n");
        }

        if (ok_es_ebook == FALSE)
        {
            nerrors += 1;
            str_cat(&errors, "Error generating 'es' ebook\n");
        }

        warns_b64 = b64_encode_from_str(warns);
        errors_b64 = b64_encode_from_str(errors);
        str_destroy(&warns);
        str_destroy(&errors);

        report_doc(report, doc_repo_vers, &hosting_url, &stdout_b64, &stderr_b64, &warns_b64, &errors_b64, ndoc_cache, ndoc_ret, nwarns, nerrors);
    }

    cassert((ok == TRUE && *error_msg == NULL) || (ok == FALSE && *error_msg != NULL));
    return ok;
}

/*---------------------------------------------------------------------------*/

bool_t prdoc_generate(const Global *global, const Login *drive, const char_t *project_vers, const uint32_t repo_vers, const uint32_t doc_repo_vers, const WorkPaths *wpaths, Report *report)
{
    bool_t ok = TRUE;
    RState state;
    cassert_no_null(wpaths);
    report_doc_state(report, doc_repo_vers, &state);

    if (state.done == FALSE)
    {
        String *msg = str_printf("Generate documentation '%s%d%s'", kASCII_VERSION, doc_repo_vers, kASCII_RESET);
        String *error_msg = NULL;
        log_printf("%s %s. Starting", kASCII_OK, tc(msg));
        report_doc_init(report, doc_repo_vers);
        ok = i_generate_doc(global, drive, project_vers, repo_vers, doc_repo_vers, wpaths, report, &error_msg);
        report_doc_end(report, doc_repo_vers, ok, &error_msg);
        report_doc_state(report, doc_repo_vers, &state);
        report_state_log(&state, tc(msg));
        str_destroy(&msg);
    }

    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_delete_report_path(const char_t *reppath)
{
    bool_t ok = TRUE;
    if (hfile_exists(reppath, NULL) == TRUE)
    {
        ok = hfile_dir_destroy(reppath, NULL);
        if (ok == FALSE)
            log_printf("%s Removing ndoc workpath '%s'", kASCII_FAIL, reppath);
    }

    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_checkout_report_skel(const Global *global, const char_t *repsrcpath)
{
    bool_t ok = TRUE;
    uint32_t build_repo_vers = UINT32_MAX;
    cassert_no_null(global);

    if (ok == TRUE)
    {
        build_repo_vers = ssh_repo_version(tc(global->web_report_repo_url), tc(global->web_report_repo_user), tc(global->web_report_repo_pass));
        if (build_repo_vers == UINT32_MAX)
        {
            ok = FALSE;
            log_printf("%s Getting version of build doc repo '%s'.", kASCII_FAIL, tc(global->web_report_repo_url));
        }
    }

    if (ok == TRUE)
        ok = ssh_repo_checkout(NULL, tc(global->web_report_repo_url), tc(global->web_report_repo_user), tc(global->web_report_repo_pass), build_repo_vers, repsrcpath);

    if (ok == FALSE)
        log_printf("%s Unable to checkout repo '%s' in '%s'", kASCII_FAIL, tc(global->web_report_repo_url), repsrcpath);

    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_get_previous_reports(const Login *drive, const char_t *drive_rep, const char_t *repsrc)
{
    bool_t ok = TRUE;
    String *despath = str_printf("%s/builds/en", repsrc);
    ssh_copy_dir(drive, drive_rep, NULL, tc(despath));
    str_destroy(&despath);
    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_generate_current_report(const Global *global, const ArrSt(Job) *jobs, const char_t *repfile, const Report *report, const char_t *project_vers)
{
    bool_t ok = TRUE;
    Stream *stm = report_ndoc_page(report, jobs, global, project_vers);
    const byte_t *data = stm_buffer(stm);
    uint32_t size = stm_buffer_size(stm);
    if (hfile_from_data(repfile, data, size, NULL) == FALSE)
    {
        ok = FALSE;
        log_printf("%s Generating build ndoc report '%s'", kASCII_FAIL, repfile);
    }

    stm_close(&stm);
    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_store_current_report(const Login *drive, const char_t *drive_rep, const char_t *repfile)
{
    bool_t ok = TRUE;
    String *srcpath = NULL;
    String *srcfile = NULL;
    str_split_pathname(repfile, &srcpath, &srcfile);
    ok = ssh_copy(NULL, tc(srcpath), tc(srcfile), drive, drive_rep, tc(srcfile));
    if (ok == FALSE)
        log_printf("%s Storing current report '%s'", kASCII_OK, tc(srcfile));
    str_destroy(&srcpath);
    str_destroy(&srcfile);
    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_update_config_json(const char_t *repsrc)
{
    bool_t ok = TRUE;
    String *json_file = str_cpath("%s/builds/config.json", repsrc);
    ConfJson *json = NULL;

    if (ok == TRUE)
    {
        Stream *stm = hfile_stream(tc(json_file), NULL);
        if (stm != NULL)
        {
            json = json_read(stm, NULL, ConfJson);
            if (json == NULL)
            {
                ok = FALSE;
                log_printf("%s Reading '%s'", kASCII_FAIL, tc(json_file));
            }
            stm_close(&stm);
        }
    }

    if (ok == TRUE)
    {
        if (arrst_size(json->web, WebJson) > 0)
        {
            String *docpath = str_cpath("%s/builds/en", repsrc);
            ArrSt(DirEntry) *files = hfile_dir_list(tc(docpath), FALSE, NULL);
            WebJson *web = arrst_get(json->web, 0, WebJson);
            arrpt_clear(web->docs, str_destroy, String);
            arrst_forback_const(file, files, DirEntry)
                String *filename = NULL;
                str_split_pathext(tc(file->name), NULL, &filename, NULL);
                arrpt_append(web->docs, filename, String);
            arrst_end();
            str_destroy(&docpath);
            arrst_destroy(&files, hfile_dir_entry_remove, DirEntry);
        }
        else
        {
            ok = FALSE;
            log_printf("%s Web report config.json with wrong content", kASCII_FAIL);
        }
    }

    if (ok == TRUE)
    {
        Stream *stm = stm_to_file(tc(json_file), NULL);
        json_write(stm, json, NULL, ConfJson);
        stm_close(&stm);
    }

    json_destopt(&json, ConfJson);
    str_destroy(&json_file);
    return ok;
}

/*---------------------------------------------------------------------------*/

static String *i_ndoc_report_cmd(const char_t *project_vers, const uint32_t repo_vers, const char_t *ndoc_srcpath, const char_t *ndoc_outpath)
{
    char_t exec_path[256];
    String *path = NULL, *ndoc = NULL, *cmd = NULL;
    Stream *stm = stm_memory(512);
    /*
    "args": [ "-v", "version",
              "-p", "/home/fran/nbuild_temp/ndoc_report_src",
              "-o", "/home/fran/nbuild_temp/ndoc_report"],
    */
    bfile_dir_exec(exec_path, sizeof(exec_path));
    str_split_pathname(exec_path, &path, NULL);
    ndoc = str_cpath("%s/%s", tc(path), NDOC_APP);
    stm_writef(stm, tc(ndoc));
    stm_writef(stm, " ");
    stm_printf(stm, "-v %s.%d", project_vers, repo_vers);
    stm_writef(stm, " ");
    stm_printf(stm, "-p %s", ndoc_srcpath);
    stm_writef(stm, " ");
    stm_printf(stm, "-o %s", ndoc_outpath);
    cmd = stm_str(stm);
    stm_close(&stm);
    str_destopt(&path);
    str_destopt(&ndoc);
    return cmd;
}

/*---------------------------------------------------------------------------*/

static bool_t i_build_report_website(const char_t *project_vers, const uint32_t repo_vers, const char_t *tmp_nrep, const char_t *repsrc)
{
    bool_t ok = TRUE;

    if (ok == TRUE)
    {
        String *outpath = str_cpath("%s/ndoc", tmp_nrep);
        String *cmd = i_ndoc_report_cmd(project_vers, repo_vers, repsrc, tc(outpath));
        Stream *stdout_ = NULL;
        Stream *stderr_ = NULL;
        uint32_t ret = ssh_command(tc(cmd), &stdout_, &stderr_);

        if (ret != 0)
        {
            ok = FALSE;
            log_printf("%s Generating build report website '%s'", kASCII_FAIL, tc(outpath));
        }

        stm_close(&stdout_);
        stm_close(&stderr_);
        str_destroy(&outpath);
        str_destroy(&cmd);
    }

    /* Generate bindex.html */
    if (ok == TRUE)
    {
        String *file = str_cpath("%s/ndoc/web/en/builds/bindex.html", tmp_nrep);
        Stream *stm = stm_to_file(tc(file), NULL);
        stm_printf(stm, "<meta http-equiv=\"Refresh\" content=\"0; url='r%d.html'\" />", repo_vers);
        stm_close(&stm);
        str_destroy(&file);
    }

    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_store_generated_website(const Login *drive, const char_t *tmp_nrep, const char_t *drive_rep_web)
{
    bool_t ok = TRUE;

    /* Compress the documentation */
    if (ok == TRUE)
    {
        String *websrc = str_cpath("%s/ndoc/web", tmp_nrep);
        String *tarpath = str_printf("%s/%s", tmp_nrep, NBUILD_REP_TAR);
        ok = ssh_cmake_tar(NULL, tc(websrc), tc(tarpath));
        if (ok == FALSE)
            log_printf("%s Compressing generated report website '%s'", kASCII_FAIL, tc(websrc));
        str_destroy(&websrc);
        str_destroy(&tarpath);
    }

    /* Copy website to drive */
    if (ok == TRUE)
    {
        ok = ssh_copy(NULL, tmp_nrep, NBUILD_REP_TAR, drive, drive_rep_web, NBUILD_REP_TAR);
        if (ok == TRUE)
            log_printf("%s Stored generated report website '%s%s%s'", kASCII_OK, kASCII_PATH, drive_rep_web, kASCII_RESET);
        else
            log_printf("%s Storing generated report website '%s'", kASCII_FAIL, drive_rep_web);
    }

    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_upload_rep_website(const Global *global, const char_t *tmp_nrep, String **error_msg)
{
    bool_t ok = TRUE;
    String *local_path = str_cpath("%s/ndoc/web", tmp_nrep);
    cassert_no_null(global);
    ok = i_upload_website(global, tc(local_path), tc(global->hosting_buildpath), error_msg);
    str_destroy(&local_path);
    return ok;
}

/*---------------------------------------------------------------------------*/

bool_t prdoc_buildrep_generate(const Global *global, const ArrSt(Job) *jobs, const Login *drive, const char_t *project_vers, const uint32_t repo_vers, const WorkPaths *wpaths, const Report *report)
{
    bool_t ok = TRUE;
    String *repsrc = NULL;
    String *repfile = NULL;
    cassert_no_null(global);
    cassert_no_null(wpaths);
    log_printf("%s Beginning nbuild web report '%sr%d.htm%s'", kASCII_OK, kASCII_VERSION, repo_vers, kASCII_RESET);
    repsrc = str_cpath("%s/src", tc(wpaths->tmp_nrep));
    repfile = str_cpath("%s/builds/en/r%d.htm", tc(repsrc), repo_vers);

    if (ok == TRUE)
        ok = i_delete_report_path(tc(wpaths->tmp_nrep));

    if (ok == TRUE)
        ok = i_checkout_report_skel(global, tc(repsrc));

    if (ok == TRUE)
        ok = i_get_previous_reports(drive, tc(wpaths->drive_rep), tc(repsrc));

    if (ok == TRUE)
        ok = i_generate_current_report(global, jobs, tc(repfile), report, project_vers);

    if (ok == TRUE)
        ok = i_store_current_report(drive, tc(wpaths->drive_rep), tc(repfile));

    if (ok == TRUE)
        ok = i_update_config_json(tc(repsrc));

    if (ok == TRUE)
        ok = i_build_report_website(project_vers, repo_vers, tc(wpaths->tmp_nrep), tc(repsrc));

    if (ok == TRUE)
        ok = i_store_generated_website(drive, tc(wpaths->tmp_nrep), tc(wpaths->drive_rep_web));

    if (ok == TRUE)
    {
        if (str_empty(global->hosting_url) == FALSE)
        {
            String *error_msg = NULL;
            ok = i_upload_rep_website(global, tc(wpaths->tmp_nrep), &error_msg);
            str_destopt(&error_msg);
        }
    }

    str_destroy(&repsrc);
    str_destroy(&repfile);
    return ok;
}
