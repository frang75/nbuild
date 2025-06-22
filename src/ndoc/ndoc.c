/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: ndoc.c
 *
 */

/* NAppGUI Doc */

#include "ndoc.hxx"
#include "loader.h"
#include "nlog.h"
#include "web.h"
#include <html5/html5.h>
#include <html5/wsite.h>
#include <nlib/nlib.h>
#include <nlib/ssh.h>
#include <encode/json.h>
#include <core/arrpt.h>
#include <core/arrst.h>
#include <core/date.h>
#include <core/dbind.h>
#include <core/hfile.h>
#include <core/stream.h>
#include <core/strings.h>
#include <osbs/bfile.h>
#include <osbs/log.h>
#include <sewer/blib.h>
#include <sewer/bstd.h>
#include <sewer/cassert.h>
#include <sewer/sewer.h>

/*---------------------------------------------------------------------------*/

static void config_dbind(void)
{
    dbind(Lang, String *, lang);
    dbind(Lang, bool_t, build);
    dbind(EBPart, bool_t, build);
    dbind(EBPart, ArrPt(String) *, title);
    dbind(EBPart, ArrPt(String) *, packs);
    dbind(WebSec, bool_t, build);
    dbind(WebSec, ArrPt(String) *, packs);
    dbind(WebSec, ArrPt(String) *, menu);
    dbind(WebSec, ArrPt(String) *, hover);
    dbind(WebSec, ArrPt(String) *, url);
    dbind(Config, ArrSt(Lang) *, langs);
    dbind(Config, String *, project_name);
    dbind(Config, String *, project_author);
    dbind(Config, uint32_t, project_start_year);
    dbind(Config, String *, project_url);
    dbind(Config, String *, project_email);
    dbind(Config, ArrPt(String) *, project_brief);
    dbind(Config, ArrPt(String) *, project_license);
    dbind(Config, ArrPt(String) *, project_license_url);
    dbind(Config, ArrPt(String) *, project_legal_url);
    dbind(Config, ArrPt(String) *, project_license_ebook);
    dbind(Config, ArrPt(String) *, support_email);
    dbind(Config, String *, remote_repo);
    dbind(Config, bool_t, ebook);
    dbind(Config, uint8_t, ebook_funcs_mode);
    dbind(Config, String *, ebook_section_color);
    dbind(Config, String *, ebook_bqback_color);
    dbind(Config, String *, ebook_bqline_color);
    dbind(Config, String *, ebook_funcs_color);
    dbind(Config, String *, ebook_types_color);
    dbind(Config, String *, ebook_constants_color);
    dbind(Config, ArrPt(String) *, ebook_title);
    dbind(Config, ArrPt(String) *, ebook_subtitle);
    dbind(Config, ArrPt(String) *, ebook_langrev);
    dbind(Config, ArrPt(String) *, ebook_chapters);
    dbind(Config, ArrSt(EBPart) *, ebook_parts);
    dbind(Config, bool_t, web);
    dbind(Config, String *, web_site_font);
    dbind(Config, String *, web_head_font);
    dbind(Config, String *, web_mono_font);
    dbind(Config, real32_t, web_lnav_width);
    dbind(Config, real32_t, web_post_width);
    dbind(Config, real32_t, web_post_def_imgwidth);
    dbind(Config, real32_t, web_rcol_width);
    dbind(Config, String *, web_back_color);
    dbind(Config, String *, web_text_color);
    dbind(Config, String *, web_title_color);
    dbind(Config, String *, web_coltext_color);
    dbind(Config, String *, web_colback_color);
    dbind(Config, String *, web_sec_color);
    dbind(Config, String *, web_secback_color);
    dbind(Config, String *, web_ter_color);
    dbind(Config, String *, web_over_color);
    dbind(Config, String *, web_navback_color);
    dbind(Config, String *, web_navtext_color);
    dbind(Config, String *, web_navhover_color);
    dbind(Config, String *, web_funcs_color);
    dbind(Config, String *, web_types_color);
    dbind(Config, String *, web_constants_color);
    dbind(Config, String *, web_analytics);
    dbind(Config, ArrPt(String) *, web_langrev);
    dbind(Config, ArrSt(WebSec) *, web_sections);
}

/*---------------------------------------------------------------------------*/

static bool_t i_dir(const char_t *pathname)
{
    if (hfile_dir_create(pathname, NULL) == TRUE)
    {
        String *msg = str_printf("Directory '%s%s%s'", kASCII_PATH, pathname, kASCII_RESET);
        nlog_ok(&msg);
        return TRUE;
    }
    else
    {
        String *msg = str_printf("Creating directory '%s'", pathname);
        nlog_error(&msg);
        return FALSE;
    }
}

/*---------------------------------------------------------------------------*/

static void i_print_usage(void)
{
    bstd_printf("Use: ndoc -v project_vers\n");
    bstd_printf("          [-p path to source docs]\n");
    bstd_printf("          [-r repo_url repo_user repo_pass repo_vers (0=HEAD)]\n");
    bstd_printf("          [-s src_repo_url src_repo_user src_repo_pass src_repo_vers (0=HEAD)]\n");
    bstd_printf("          [-o output directory]\n");
    bstd_printf("     -p has preference over -r\n");
    bstd_printf("     -s is optional\n");
    bstd_printf("     if -o the executable folder will be used\n");
}

/*---------------------------------------------------------------------------*/

static bool_t i_args(
    const uint32_t argc,
    const char_t **argv,
    const char_t **project_vers,
    const char_t **localdocs_path,
    const char_t **repo_url,
    const char_t **repo_user,
    const char_t **repo_pass,
    uint32_t *repo_vers,
    const char_t **src_repo_url,
    const char_t **src_repo_user,
    const char_t **src_repo_pass,
    uint32_t *src_repo_vers,
    const char_t **output_path)
{
    uint32_t i = 1;
    cassert_no_null(argv);
    cassert_no_null(project_vers);
    cassert_no_null(localdocs_path);
    cassert_no_null(repo_url);
    cassert_no_null(repo_user);
    cassert_no_null(repo_pass);
    cassert_no_null(repo_vers);
    cassert_no_null(src_repo_url);
    cassert_no_null(src_repo_user);
    cassert_no_null(src_repo_pass);
    cassert_no_null(src_repo_vers);
    cassert_no_null(output_path);

    for (;;)
    {
        if (i == argc)
            break;

        if (str_equ_c(argv[i], "-v") == TRUE)
        {
            if (i + 1 >= argc)
            {
                i_print_usage();
                return FALSE;
            }

            *project_vers = argv[i + 1];
            i += 2;
        }
        else if (str_equ_c(argv[i], "-p") == TRUE)
        {
            if (i + 1 >= argc)
            {
                i_print_usage();
                return FALSE;
            }

            *localdocs_path = argv[i + 1];
            i += 2;
        }
        else if (str_equ_c(argv[i], "-r") == TRUE)
        {
            if (i + 4 >= argc)
            {
                i_print_usage();
                return FALSE;
            }

            *repo_url = argv[i + 1];
            *repo_user = argv[i + 2];
            *repo_pass = argv[i + 3];
            *repo_vers = (uint32_t)blib_strtoul(argv[i + 4], NULL, 10, NULL);
            i += 5;
        }
        else if (str_equ_c(argv[i], "-s") == TRUE)
        {
            if (i + 4 >= argc)
            {
                i_print_usage();
                return FALSE;
            }

            *src_repo_url = argv[i + 1];
            *src_repo_user = argv[i + 2];
            *src_repo_pass = argv[i + 3];
            *src_repo_vers = (uint32_t)blib_strtoul(argv[i + 4], NULL, 10, NULL);
            i += 5;
        }
        else if (str_equ_c(argv[i], "-o") == TRUE)
        {
            if (i + 1 >= argc)
            {
                i_print_usage();
                return FALSE;
            }

            *output_path = argv[i + 1];
            i += 2;
        }
        else
        {
            bstd_printf("Unknown %d option '%s'\n", i, argv[i]);
            i_print_usage();
            return FALSE;
        }
    }

    if (*project_vers == NULL)
    {
        bstd_printf("Missing option '-v'\n");
        i_print_usage();
        return FALSE;
    }

    if (*localdocs_path == NULL && *repo_url == NULL)
    {
        bstd_printf("You have to provide '-p' or '-r' documents source input\n");
        i_print_usage();
        return FALSE;
    }

    return TRUE;
}

/*---------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
    bool_t ok = TRUE;
    const char_t *project_vers = NULL;
    const char_t *localdocs_path = NULL;
    const char_t *repo_url = NULL;
    const char_t *repo_user = NULL;
    const char_t *repo_pass = NULL;
    uint32_t repo_vers = 0;
    const char_t *src_repo_url = NULL;
    const char_t *src_repo_user = NULL;
    const char_t *src_repo_pass = NULL;
    uint32_t src_repo_vers = 0;
    const char_t *output_path = NULL;
    String *workpath = NULL;
    String *docpath = NULL;
    String *webpath = NULL;
    String *texpath = NULL;
    String *tmppath = NULL;
    Config *config = NULL;

    html5_start();
    nlog_init();
    config_dbind();
    loader_dbind();
    log_printf("nbuild %s", sewer_nappgui_version(TRUE));

    ok = i_args((uint32_t)argc, dcast_const(argv, char_t), &project_vers, &localdocs_path, &repo_url, &repo_user, &repo_pass, &repo_vers, &src_repo_url, &src_repo_user, &src_repo_pass, &src_repo_vers, &output_path);

    /* Documentation repo version */
    if (ok == TRUE)
    {
        if (repo_url != NULL && repo_vers == 0)
        {
            repo_vers = ssh_repo_version(repo_url, repo_user, repo_pass);
            if (repo_vers == UINT32_MAX)
            {
                String *msg = str_printf("Unable to get repo version '%s'.", repo_url);
                nlog_error(&msg);
                ok = FALSE;
            }
        }
    }

    /* Source code repo version */
    if (ok == TRUE)
    {
        if (src_repo_url != NULL && src_repo_vers == 0)
        {
            src_repo_vers = ssh_repo_version(src_repo_url, src_repo_user, src_repo_pass);
            if (src_repo_vers == UINT32_MAX)
            {
                String *msg = str_printf("Unable to get source repo version '%s'.", src_repo_url);
                nlog_error(&msg);
                ok = FALSE;
            }
        }
    }

    /* Generate the working paths */
    if (ok == TRUE)
    {
        if (output_path != NULL)
        {
            workpath = str_c(output_path);
        }
        else
        {
            char_t execdir[512];
            String *path = NULL;
            bfile_dir_exec(execdir, sizeof(execdir));
            str_split_pathname(execdir, &path, NULL);
            workpath = str_cpath("%s/ndoc_build", tc(path));
            str_destroy(&path);
        }

        docpath = str_cpath("%s/src", tc(workpath));
        webpath = str_cpath("%s/web", tc(workpath));
        texpath = str_cpath("%s/tex", tc(workpath));
        tmppath = str_cpath("%s/tmp", tc(workpath));

        /* Delete the previous working directory */
        if (hfile_exists(tc(workpath), NULL) == TRUE)
        {
            ok = hfile_dir_destroy(tc(workpath), NULL);
            if (ok == TRUE)
            {
                String *msg = str_printf("Removed working dir '%s%s%s'.", kASCII_PATH, tc(workpath), kASCII_RESET);
                nlog_ok(&msg);
            }
            else
            {
                String *msg = str_printf("Removing working dir '%s%s%s'.", kASCII_PATH, tc(workpath), kASCII_RESET);
                nlog_error(&msg);
            }
        }

        /* Create */
        ok &= i_dir(tc(workpath));
        ok &= i_dir(tc(docpath));
        ok &= i_dir(tc(webpath));
        ok &= i_dir(tc(texpath));
        ok &= i_dir(tc(tmppath));
    }

    /* Checkout documentation sources */
    if (ok == TRUE)
    {
        /* Sources from local filesystem */
        if (localdocs_path != NULL)
        {
            {
                String *msg = str_printf("Copy document sources from '%s%s%s' in '%s%s%s'", kASCII_TARGET, localdocs_path, kASCII_RESET, kASCII_PATH, tc(docpath), kASCII_RESET);
                nlog_ok(&msg);
            }

            ok = hfile_dir_sync(localdocs_path, tc(docpath), TRUE, FALSE, NULL, 0, NULL);
        }
        /* Sources in repository */
        else
        {
            {
                String *msg = str_printf("Checkout '%s%s%s' in '%s%s%s'", kASCII_TARGET, repo_url, kASCII_RESET, kASCII_PATH, tc(docpath), kASCII_RESET);
                nlog_ok(&msg);
            }

            ok = ssh_repo_checkout(NULL, repo_url, repo_user, repo_pass, repo_vers, tc(docpath));

            if (ok == FALSE)
            {
                String *msg = str_printf("Unable to checkout repo '%s' in '%s'.", repo_url, tc(docpath));
                nlog_error(&msg);
            }
        }
    }

    /* Load the config file */
    if (ok == TRUE)
    {
        String *config_file = str_cpath("%s/config/config.json", tc(docpath));
        Stream *stm = stm_from_file(tc(config_file), NULL);
        if (stm != NULL)
        {
            config = json_read(stm, NULL, Config);
            if (config != NULL)
            {
                String *msg = str_printf("Readed '%s%s%s'.", kASCII_TARGET, tc(config_file), kASCII_RESET);
                nlog_ok(&msg);
            }
            else
            {
                String *msg = str_printf("Parsing '%s'.", tc(config_file));
                nlog_error(&msg);
                ok = FALSE;
            }

            stm_close(&stm);
        }
        else
        {
            String *msg = str_printf("Reading '%s'", tc(config_file));
            nlog_error(&msg);
            ok = FALSE;
        }

        str_destroy(&config_file);
    }

    /* Hello message */
    if (ok == TRUE)
    {
        String *msg = str_printf("Documentation revision %s%d%s of %s%s%s %s%s.%d%s", kASCII_VERSION, repo_vers, kASCII_RESET, kASCII_TARGET, tc(config->project_name), kASCII_RESET, kASCII_VERSION, project_vers, src_repo_vers, kASCII_RESET);
        nlog_ok(&msg);
    }

    if (ok == TRUE)
    {
        ArrPt(Loader) *loaders = arrpt_create(Loader);
        char_t repo_vers_str[32];

        bstd_sprintf(repo_vers_str, sizeof(repo_vers_str), "%d", repo_vers);

        /* Load all documents in all languages */
        arrst_foreach(lang, config->langs, Lang)
            if (lang->build == TRUE)
            {
                Loader *loader = loader_run(tc(docpath), src_repo_url, src_repo_user, src_repo_pass, src_repo_vers, config, lang_i);
                arrpt_append(loaders, loader, Loader);
            }
        arrst_end()

        /* Check the coherence of all languages */
        if (arrpt_size(loaders, Loader) > 1)
        {
            const Loader *first = arrpt_get(loaders, 0, Loader);
            uint32_t i, n = arrpt_size(loaders, Loader);
            for (i = 1; i < n; ++i)
            {
                const Loader *loader = arrpt_get(loaders, i, Loader);
                loader_compare(first, loader);
            }
        }

        if (config->ebook == TRUE)
        {
            arrpt_foreach(loader, loaders, Loader)
                loader_latex(loader, tc(texpath), project_vers);
            arrpt_end()
        }

        if (config->web == TRUE && arrpt_size(loaders, Loader) > 0)
        {
            WSite *site = web_start(tc(docpath), tc(webpath), tc(tmppath), config);
            arrpt_foreach(loader, loaders, Loader)
                loader_web(loader, site, project_vers);
            arrpt_end()
            wsite_destroy(&site);
        }

        arrpt_destopt(&loaders, loader_destroy, Loader);
    }

    dbind_destopt(&config, Config);
    str_destopt(&workpath);
    str_destopt(&docpath);
    str_destopt(&webpath);
    str_destopt(&texpath);
    str_destopt(&tmppath);
    html5_finish();
    return ok ? 0 : 1;
}
