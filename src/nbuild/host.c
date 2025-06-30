/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: host.c
 *
 */

/* Build host (runner) */

#include "host.h"
#include "nbuild.h"
#include "network.h"
#include <nlib/nlib.h>
#include <nlib/ssh.h>
#include <nlib/vers.h>
#include <encode/base64.h>
#include <core/arrst.h>
#include <core/arrpt.h>
#include <core/dbind.h>
#include <core/strings.h>
#include <core/stream.h>
#include <osbs/log.h>
#include <sewer/cassert.h>

typedef enum _generator_t
{
    ekGENERATOR_VS_MSBUILD,
    ekGENERATOR_NINJA,
    ekGENERATOR_NINJA_MULTI_CONFIG,
    ekGENERATOR_MINGW,
    ekGENERATOR_MSYS,
    ekGENERATOR_UNIX_MAKEFILES,
    ekGENERATOR_XCODE
} generator_t;

struct _host_t
{
    String *name;
    String *workpath;
    String *type;
    String *vbox_uuid;
    String *vbox_host;
    String *utm_uuid;
    String *utm_host;
    String *vmware_path;
    String *vmware_host;
    String *macos_host;
    String *macos_volume;
    String *mingw_path;
    Login login;
    ArrPt(String) *generators;
    ArrPt(String) *tags;
};

ArrStDebug(Host);

/*---------------------------------------------------------------------------*/

void host_dbind(void)
{
    dbind(Host, String *, name);
    dbind(Host, String *, workpath);
    dbind(Host, String *, type);
    dbind(Host, String *, vbox_uuid);
    dbind(Host, String *, vbox_host);
    dbind(Host, String *, utm_uuid);
    dbind(Host, String *, utm_host);
    dbind(Host, String *, vmware_path);
    dbind(Host, String *, vmware_host);
    dbind(Host, String *, macos_host);
    dbind(Host, String *, macos_volume);
    dbind(Host, String *, mingw_path);
    dbind(Host, Login, login);
    dbind(Host, ArrPt(String) *, generators);
    dbind(Host, ArrPt(String) *, tags);
}

/*---------------------------------------------------------------------------*/

bool_t host_check_config(const ArrSt(Host) *hosts)
{
    /* Check for duplicated hosts */
    bool_t ok = TRUE;
    const Host *host = arrst_all_const(hosts, Host);
    uint32_t i, j, n = arrst_size(hosts, Host);
    for (i = 0; i < n && ok; ++i)
    {
        for (j = i + 1; j < n && ok; ++j)
        {
            if (str_equ(host[i].name, tc(host[j].name)))
            {
                log_printf("%s Duplicated host '%s'.", kASCII_FAIL, tc(host[i].name));
                ok = FALSE;
            }
        }
    }

    return ok;
}

/*---------------------------------------------------------------------------*/

const char_t *host_name(const Host *host)
{
    cassert_no_null(host);
    return tc(host->name);
}

/*---------------------------------------------------------------------------*/

const char_t *host_type(const Host *host)
{
    cassert_no_null(host);
    return tc(host->type);
}

/*---------------------------------------------------------------------------*/

const char_t *host_vbox_uuid(const Host *host)
{
    cassert_no_null(host);
    return tc(host->vbox_uuid);
}

/*---------------------------------------------------------------------------*/

const char_t *host_vbox_host(const Host *host)
{
    cassert_no_null(host);
    return tc(host->vbox_host);
}

/*---------------------------------------------------------------------------*/

const char_t *host_utm_uuid(const Host *host)
{
    cassert_no_null(host);
    return tc(host->utm_uuid);
}

/*---------------------------------------------------------------------------*/

const char_t *host_utm_host(const Host *host)
{
    cassert_no_null(host);
    return tc(host->utm_host);
}

/*---------------------------------------------------------------------------*/

const char_t *host_vmware_path(const Host *host)
{
    cassert_no_null(host);
    return tc(host->vmware_path);
}

/*---------------------------------------------------------------------------*/

const char_t *host_vmware_host(const Host *host)
{
    cassert_no_null(host);
    return tc(host->vmware_host);
}

/*---------------------------------------------------------------------------*/

const char_t *host_macos_host(const Host *host)
{
    cassert_no_null(host);
    return tc(host->macos_host);
}

/*---------------------------------------------------------------------------*/

const char_t *host_macos_volume(const Host *host)
{
    cassert_no_null(host);
    return tc(host->macos_volume);
}

/*---------------------------------------------------------------------------*/

const Login *host_login(const Host *host)
{
    cassert_no_null(host);
    return &host->login;
}

/*---------------------------------------------------------------------------*/

void host_localhost(ArrSt(Host) *hosts, const ArrSt(uint32_t) *ips)
{
    arrst_foreach(host, hosts, Host)
        network_localhost(&host->login, ips);
    arrst_end()
}

/*---------------------------------------------------------------------------*/

static bool_t i_job_match(const Host *host, const Job *job)
{
    bool_t ok = TRUE;
    cassert_no_null(job);
    cassert_no_null(host);

    /* Check if host has installed the required generator */
    if (ok == TRUE)
    {
        ok = FALSE;
        arrpt_foreach_const(generator, host->generators, String)
            if (str_equ(generator, tc(job->generator)) == TRUE)
            {
                ok = TRUE;
                break;
            }
        arrpt_end()
    }

    /* Check if host has all tags required */
    if (ok == TRUE)
    {
        arrpt_foreach_const(tag, job->tags, String)
            /* Look for a tag in host */
            bool_t found = FALSE;
            arrpt_foreach_const(htag, host->tags, String)
                if (str_equ(tag, tc(htag)) == TRUE)
                {
                    found = TRUE;
                    break;
                }
            arrpt_end()

            /* The host hasn't one tag required by the job */
            if (found == FALSE)
            {
                ok = FALSE;
                break;
            }
        arrpt_end()
    }

    return ok;
}

/*---------------------------------------------------------------------------*/

const Host *host_match_job(const ArrSt(Host) *hosts, const Job *job)
{
    arrst_foreach_const(host, hosts, Host)
        if (i_job_match(host, job) == TRUE)
            return host;
    arrst_end()
    return NULL;
}

/*---------------------------------------------------------------------------*/

const Host *host_by_name(const ArrSt(Host) *hosts, const char_t *name)
{
    arrst_foreach_const(host, hosts, Host)
        if (str_equ(host->name, name) == TRUE)
            return host;
    arrst_end()
    return NULL;
}

/*---------------------------------------------------------------------------*/

const Host *host_macos_alive(const ArrSt(Host) *hosts, const char_t *macos_host)
{
    /* Look for some booted macOS volume in a Mac computer */
    arrst_foreach_const(host, hosts, Host)
        if (str_equ(host->type, "macos") == TRUE)
        {
            if (str_equ(host->macos_host, macos_host) == TRUE)
            {
                bool_t ping = ssh_ping(tc(host->login.ip));
                if (ping == TRUE)
                    return host;
            }
        }
    arrst_end()
    return NULL;
}

/*---------------------------------------------------------------------------*/

macos_t host_macos_version(const Host *host)
{
    cassert_no_null(host);
    if (host->login.platform == ekMACOS)
    {
        arrpt_foreach_const(tag, host->tags, String)
            if (str_equ(tag, "sequoia") == TRUE)
                return ekMACOS_SEQUOIA;
            if (str_equ(tag, "sonoma") == TRUE)
                return ekMACOS_SONOMA;
            if (str_equ(tag, "ventura") == TRUE)
                return ekMACOS_VENTURA;
            if (str_equ(tag, "monterey") == TRUE)
                return ekMACOS_MONTEREY;
            if (str_equ(tag, "bigsur") == TRUE)
                return ekMACOS_BIG_SUR;
            if (str_equ(tag, "catalina") == TRUE)
                return ekMACOS_CATALINA;
            if (str_equ(tag, "mojave") == TRUE)
                return ekMACOS_MOJAVE;
            if (str_equ(tag, "high_sierra") == TRUE)
                return ekMACOS_HIGH_SIERRA;
            if (str_equ(tag, "sierra") == TRUE)
                return ekMACOS_SIERRA;
            if (str_equ(tag, "el_capitan") == TRUE)
                return ekMACOS_EL_CAPITAN;
            if (str_equ(tag, "yosemite") == TRUE)
                return ekMACOS_YOSEMITE;
            if (str_equ(tag, "mavericks") == TRUE)
                return ekMACOS_MAVERICKS;
            if (str_equ(tag, "mountain_lion") == TRUE)
                return ekMACOS_MOUNTAIN_LION;
            if (str_equ(tag, "lion") == TRUE)
                return ekMACOS_LION;
            if (str_equ(tag, "snow_leopard") == TRUE)
                return ekMACOS_SNOW_LEOPARD;
            if (str_equ(tag, "leopard") == TRUE)
                return ekMACOS_LEOPARD;
        arrpt_end()
    }

    return ekMACOS_UNKNOWN;
}

/*---------------------------------------------------------------------------*/

static bool_t i_create_build_dirs(const Host *host, const char_t *flowpath, const uint32_t runner_id, String **error_msg)
{
    bool_t ok = TRUE;
    cassert_no_null(host);
    cassert_no_null(error_msg);
    cassert(*error_msg == NULL);
    if (ok == TRUE)
    {
        ok = ssh_create_dir(&host->login, tc(host->workpath));
        if (ok == FALSE)
            *error_msg = str_printf("Error creating host '%s' directory '%s'", tc(host->name), tc(host->workpath));
    }

    if (ok == TRUE)
    {
        if (ssh_dir_exists(&host->login, flowpath) == TRUE)
        {
            ok = ssh_delete_dir(&host->login, flowpath);
            if (ok == FALSE)
                *error_msg = str_printf("Error removing host '%s' directory '%s'", tc(host->name), flowpath);
        }
    }

    if (ok == TRUE)
    {
        ok = ssh_create_dir(&host->login, flowpath);
        if (ok == FALSE)
            *error_msg = str_printf("Error creating host '%s' directory '%s'", tc(host->name), flowpath);
    }

    if (ok == TRUE)
        log_printf("%s Runner %s[%d]%s '%s%s%s' created '%s%s%s' build directory", kASCII_SCHED, kASCII_VERSION, runner_id, kASCII_RESET, kASCII_PATH, tc(host->name), kASCII_RESET, kASCII_PATH, flowpath, kASCII_RESET);

    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_get_source_package(const Host *host, const WorkPaths *wpaths, const char_t *flowpath, const char_t *tarname, const char_t *destpath, const uint32_t runner_id, String **error_msg)
{
    bool_t ok = TRUE;
    cassert_no_null(host);
    cassert_no_null(wpaths);
    cassert_no_null(error_msg);
    cassert(*error_msg == NULL);

    /* Copy the source package from master to host */
    if (ok == TRUE)
    {
        ok = ssh_copy(NULL, tc(wpaths->tmp_path), tarname, &host->login, flowpath, tarname);
        if (ok == FALSE)
            *error_msg = str_printf("Error copying '%s' to '%s'", tarname, flowpath);
    }

    /* Create the source uncompressed dir */
    if (ok == TRUE)
    {
        ok = ssh_create_dir(&host->login, destpath);
        if (ok == FALSE)
            *error_msg = str_printf("Error creating '%s'", destpath);
    }

    /* De-compress the source package */
    if (ok == TRUE)
    {
        String *tarpath = str_path(host->login.platform, "%s/%s", flowpath, tarname);
        ok = ssh_cmake_untar(&host->login, destpath, tc(tarpath));
        if (ok == FALSE)
            *error_msg = str_printf("Error uncompressing '%s' into '%s'", tarname, destpath);
        str_destroy(&tarpath);
    }

    if (ok == TRUE)
        log_printf("%s Runner %s[%d]%s '%s%s%s' source code ready", kASCII_SCHED, kASCII_VERSION, runner_id, kASCII_RESET, kASCII_PATH, tc(host->name), kASCII_RESET);

    return ok;
}

/*---------------------------------------------------------------------------*/

static generator_t i_generator(const char_t *generator)
{
    if (str_is_prefix(generator, "Visual Studio") == TRUE)
        return ekGENERATOR_VS_MSBUILD;

    if (str_equ_c(generator, "Ninja Multi-Config") == TRUE)
        return ekGENERATOR_NINJA_MULTI_CONFIG;

    if (str_equ_c(generator, "Ninja") == TRUE)
        return ekGENERATOR_NINJA;

    if (str_equ_c(generator, "MinGW Makefiles") == TRUE)
        return ekGENERATOR_MINGW;

    if (str_equ_c(generator, "MSYS Makefiles") == TRUE)
        return ekGENERATOR_MSYS;

    if (str_equ_c(generator, "Unix Makefiles") == TRUE)
        return ekGENERATOR_UNIX_MAKEFILES;

    if (str_equ_c(generator, "Xcode") == TRUE)
        return ekGENERATOR_XCODE;

    return ENUM_MAX(generator_t);
}

/*---------------------------------------------------------------------------*/

static bool_t i_generator_multi_config(const generator_t generator)
{
    switch (generator)
    {
    case ekGENERATOR_VS_MSBUILD:
    case ekGENERATOR_NINJA_MULTI_CONFIG:
    case ekGENERATOR_XCODE:
        return TRUE;
    case ekGENERATOR_NINJA:
    case ekGENERATOR_MINGW:
    case ekGENERATOR_MSYS:
    case ekGENERATOR_UNIX_MAKEFILES:
        return FALSE;
        cassert_default();
    }

    return FALSE;
}

/*---------------------------------------------------------------------------*/

static bool_t i_exist_tag(const ArrPt(String) *tags, const char_t *tag)
{
    arrpt_foreach_const(etag, tags, String)
        if (str_equ(etag, tag) == TRUE)
            return TRUE;
    arrpt_end()
    return FALSE;
}

/*---------------------------------------------------------------------------*/

static bool_t i_ninja_windows(const generator_t generator, const Login *login)
{
    cassert_no_null(login);
    return ((generator == ekGENERATOR_NINJA || generator == ekGENERATOR_NINJA_MULTI_CONFIG) && login->platform == ekWINDOWS);
}

/*---------------------------------------------------------------------------*/

static String *i_cmake_envvars(const Host *host, const ArrPt(String) *tags, const generator_t generator, const uint32_t njobs)
{
    Stream *stm = stm_memory(512);
    String *str = NULL;
    cassert_no_null(host);

    if (generator == ekGENERATOR_MINGW && host->login.platform == ekWINDOWS)
        stm_printf(stm, "PATH=%s\\bin;%%PATH%%", tc(host->mingw_path));

    if (njobs != UINT32_MAX)
    {
        if (stm_bytes_written(stm) > 0)
        {
            if (host->login.platform == ekWINDOWS)
                stm_writef(stm, "&");
            else
                stm_writef(stm, ";");
        }

        if (host->login.platform == ekWINDOWS)
            stm_printf(stm, "set CMAKE_BUILD_PARALLEL_LEVEL=%d", njobs);
        else
            stm_printf(stm, "export CMAKE_BUILD_PARALLEL_LEVEL=%d", njobs);
    }

    /* For build in Windows with Ninja, we need to set the compilers and also the VS envvars */
    if (i_ninja_windows(generator, &host->login))
    {
        const char_t *arch = NULL;
        if (i_exist_tag(tags, "x64") == TRUE)
            arch = "x64";
        else if (i_exist_tag(tags, "x86") == TRUE)
            arch = "x86";

        if (arch != NULL)
        {
            if (stm_bytes_written(stm) > 0)
                stm_writef(stm, "&");

            if (i_exist_tag(tags, "msvc2022") == TRUE)
            {
                if (str_equ_c(arch, "x64") == TRUE)
                    stm_writef(stm, "vs2022_x64_vars");
                else
                    stm_writef(stm, "vs2022_x86_vars");
            }
            else if (i_exist_tag(tags, "msvc2019") == TRUE)
            {
                if (str_equ_c(arch, "x64") == TRUE)
                    stm_writef(stm, "vs2019_x64_vars");
                else
                    stm_writef(stm, "vs2019_x86_vars");
            }
            else if (i_exist_tag(tags, "msvc2017") == TRUE)
            {
                if (str_equ_c(arch, "x64") == TRUE)
                    stm_writef(stm, "vs2017_x64_vars");
                else
                    stm_writef(stm, "vs2017_x86_vars");
            }
            else if (i_exist_tag(tags, "msvc2015") == TRUE)
            {
                if (str_equ_c(arch, "x64") == TRUE)
                    stm_writef(stm, "vs2015_x64_vars");
                else
                    stm_writef(stm, "vs2015_x86_vars");
            }
            else if (i_exist_tag(tags, "msvc2013") == TRUE)
            {
                if (str_equ_c(arch, "x64") == TRUE)
                    stm_writef(stm, "vs2013_x64_vars");
                else
                    stm_writef(stm, "vs2013_x86_vars");
            }
            else if (i_exist_tag(tags, "msvc2012") == TRUE)
            {
                if (str_equ_c(arch, "x64") == TRUE)
                    stm_writef(stm, "vs2012_x64_vars");
                else
                    stm_writef(stm, "vs2012_x86_vars");
            }
            else if (i_exist_tag(tags, "msvc2010") == TRUE)
            {
                if (str_equ_c(arch, "x64") == TRUE)
                    stm_writef(stm, "vs2010_x64_vars");
                else
                    stm_writef(stm, "vs2010_x86_vars");
            }
        }
    }

    str = stm_str(stm);
    stm_close(&stm);
    return str;
}

/*---------------------------------------------------------------------------*/

static bool_t i_cmake_configure(const Host *host, const Job *job, const generator_t generator, const char_t *srcpath, const char_t *buildpath, const uint32_t runner_id, String **cmake_log, String **error_msg)
{
    bool_t ok = TRUE;
    cassert_no_null(host);
    cassert_no_null(job);
    cassert_no_null(error_msg);
    cassert(*error_msg == NULL);

    /* Create the cmake build directory */
    if (ok == TRUE)
    {
        if (ssh_dir_exists(&host->login, buildpath) == FALSE)
            ok = ssh_create_dir(&host->login, buildpath);

        if (ok == FALSE)
            *error_msg = str_printf("Error creating build dir '%s'", buildpath);
    }

    if (ok == TRUE)
    {
        String *cmake_envvars = NULL;
        Stream *stm_opts = NULL;
        String *cmake_opts = NULL;

        cmake_envvars = i_cmake_envvars(host, job->tags, generator, UINT32_MAX);
        stm_opts = stm_memory(512);

        stm_printf(stm_opts, "%s ", tc(job->opts));
        if (i_generator_multi_config(generator) == FALSE)
            stm_printf(stm_opts, "-DCMAKE_BUILD_TYPE=%s ", tc(job->config));

        if (i_ninja_windows(generator, &host->login))
            stm_printf(stm_opts, "-DCMAKE_C_COMPILER=cl -DCMAKE_CXX_COMPILER=cl ");

        if (generator == ekGENERATOR_VS_MSBUILD && i_exist_tag(job->tags, "x64") == TRUE)
            stm_printf(stm_opts, "-A x64 ");
        else if (generator == ekGENERATOR_VS_MSBUILD && i_exist_tag(job->tags, "x86") == TRUE)
            stm_printf(stm_opts, "-A Win32 ");

        cmake_opts = stm_str(stm_opts);

        if (ok == TRUE)
        {
            uint32_t ret = ssh_cmake_configure(&host->login, tc(cmake_envvars), srcpath, buildpath, tc(job->generator), tc(cmake_opts), cmake_log);
            if (ret != 0)
            {
                ok = FALSE;
                *error_msg = str_printf("Error running cmake");
            }
        }

        str_destroy(&cmake_envvars);
        str_destroy(&cmake_opts);
        stm_close(&stm_opts);
    }

    if (ok == TRUE)
        log_printf("%s Runner %s[%d]%s '%s%s%s' cmake generate", kASCII_SCHED, kASCII_VERSION, runner_id, kASCII_RESET, kASCII_PATH, tc(host->name), kASCII_RESET);

    return ok;
}

/*---------------------------------------------------------------------------*/

static uint32_t i_get_messages(const String *msgbuf, const char_t **msg, const uint32_t nmsgs, String **msg_log)
{
    const byte_t *data = cast_const(tc(msgbuf), byte_t);
    uint32_t size = str_len(msgbuf);
    Stream *stm = stm_from_block(data, size);
    Stream *out = stm_memory(1024);
    uint32_t i, n = 0;
    cassert_no_null(msg_log);
    cassert(*msg_log == NULL);

    stm_lines(line, stm)
        for (i = 0; i < nmsgs; ++i)
        {
            if (str_str(line, msg[i]) != NULL)
            {
                stm_writef(out, line);
                stm_writef(out, "\n");
                n += 1;
                break;
            }
        }
    stm_next(line, stm)

    *msg_log = stm_str(out);
    stm_close(&stm);
    stm_close(&out);
    return n;
}

/*---------------------------------------------------------------------------*/

static bool_t i_cmake_build(const Host *host, const Job *job, const generator_t generator, const char_t *buildpath, const uint32_t runner_id, String **build_log, String **warns, String **errors, uint32_t *nwarns, uint32_t *nerrors, String **error_msg)
{
    bool_t ok = TRUE;
    cassert_no_null(host);
    cassert_no_null(job);
    cassert_no_null(error_msg);
    cassert(*error_msg == NULL);
    cassert_no_null(nwarns);
    cassert_no_null(nerrors);

    if (ok == TRUE)
    {
        String *cmake_envvars = NULL;
        String *build_opts = NULL;

        cmake_envvars = i_cmake_envvars(host, job->tags, generator, 4);

        if (i_generator_multi_config(generator) == TRUE)
            build_opts = str_printf("--config %s", tc(job->config));
        else
            build_opts = str_c("");

        ssh_cmake_build(&host->login, tc(cmake_envvars), buildpath, tc(build_opts), build_log);
        str_destroy(&cmake_envvars);
        str_destroy(&build_opts);

        if (*build_log != NULL)
        {
            macos_t macos = host_macos_version(host);
            if (macos >= ekMACOS_SONOMA)
            {
                /* Please Apple, don't use non ascii chars in logs */
                String *log = str_repl(tc(*build_log), "➜", "->", NULL);
                str_destroy(build_log);
                *build_log = log;
            }
        }
    }

    if (ok == TRUE)
    {
        const char_t *warnmsgs[] = {"warning:", "warning LNK"};
        const char_t *errmsgs[] = {"error:", "error LNK"};
        *nwarns = i_get_messages(*build_log, warnmsgs, sizeof(warnmsgs) / sizeof(char_t *), warns);
        *nerrors = i_get_messages(*build_log, errmsgs, sizeof(errmsgs) / sizeof(char_t *), errors);
    }

    if (ok == TRUE)
    {
        if (*nerrors > 0)
        {
            ok = FALSE;
            *error_msg = str_printf("Build with '%d' errors", *nerrors);
            log_printf("%s Runner %s[%d]%s '%s%s%s' build with '%s%d%s' errors", kASCII_SCHED_FAIL, kASCII_VERSION, runner_id, kASCII_RESET, kASCII_PATH, tc(host->name), kASCII_RESET, kASCII_RED, *nerrors, kASCII_RESET);
        }
        else if (*nwarns > 0)
        {
            log_printf("%s Runner %s[%d]%s '%s%s%s' build with '%s%d%s' warnings", kASCII_SCHED_WARN, kASCII_VERSION, runner_id, kASCII_RESET, kASCII_PATH, tc(host->name), kASCII_RESET, kASCII_YELLOW, *nwarns, kASCII_RESET);
        }
        else
        {
            log_printf("%s Runner %s[%d]%s '%s%s%s' build", kASCII_SCHED, kASCII_VERSION, runner_id, kASCII_RESET, kASCII_PATH, tc(host->name), kASCII_RESET);
        }
    }

    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_cmake_install(const Host *host, const Job *job, const generator_t generator, const Vers *cmake_vers, const char_t *makeprogram, const char_t *buildpath, const char_t *instpath, const uint32_t runner_id, String **install_log, String **error_msg)
{
    bool_t ok = TRUE;
    cassert_no_null(host);
    cassert_no_null(job);
    cassert_no_null(error_msg);
    cassert_unref(*error_msg == NULL, error_msg);

    if (ok == TRUE)
    {
        /* Use the "modern" version of cmake, with --install option */
        if (vers_gte(cmake_vers, 3, 15, 0) == TRUE)
        {
            String *install_opts = NULL;
            uint32_t ret = UINT32_MAX;
            if (i_generator_multi_config(generator) == TRUE)
                install_opts = str_printf("--config %s --prefix %s", tc(job->config), instpath);
            else
                install_opts = str_printf("--prefix %s", instpath);

            ret = ssh_cmake_install(&host->login, buildpath, tc(install_opts), install_log);
            str_destroy(&install_opts);
            unref(ret);
        }
        else
        {
            String *install_cmd = NULL;
            cassert(str_empty_c(makeprogram) == FALSE);
            switch (generator)
            {
            case ekGENERATOR_NINJA:
            case ekGENERATOR_UNIX_MAKEFILES:
                install_cmd = str_printf("DESTDIR=%s %s install", instpath, makeprogram);
                break;

            case ekGENERATOR_XCODE:
                install_cmd = str_printf("DESTDIR=%s %s -target install -config %s", instpath, makeprogram, tc(job->config));
                break;

            case ekGENERATOR_VS_MSBUILD:
            case ekGENERATOR_NINJA_MULTI_CONFIG:
            case ekGENERATOR_MINGW:
            case ekGENERATOR_MSYS:
                ok = FALSE;
                *error_msg = str_printf("No supported native install for '%s' generator (%s)", tc(job->generator), makeprogram);
                break;
                cassert_default();
            }

            if (ok == TRUE)
            {
                uint32_t ret = ssh_cmake_install_make_program(&host->login, buildpath, tc(install_cmd), install_log);
                unref(ret);
            }

            str_destopt(&install_cmd);
        }
    }

    if (ok == TRUE)
        log_printf("%s Runner %s[%d]%s '%s%s%s' installed", kASCII_SCHED, kASCII_VERSION, runner_id, kASCII_RESET, kASCII_PATH, tc(host->name), kASCII_RESET);

    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_copy_to_drive(const Host *host, const Drive *drive, const Job *job, const char_t *flowpath, const char_t *instpath, const WorkPaths *wpaths, const uint32_t runner_id, String **error_msg)
{
    bool_t ok = TRUE;
    String *tarname = NULL;
    String *tarpath = NULL;
    cassert_no_null(host);
    cassert_no_null(drive);
    cassert_no_null(job);
    cassert_no_null(error_msg);
    cassert(*error_msg == NULL);
    tarname = str_printf("%s.tar.gz", tc(job->name));
    tarpath = str_path(host->login.platform, "%s/%s", flowpath, tc(tarname));

    if (ok == TRUE)
    {
        ok = ssh_cmake_tar(&host->login, instpath, tc(tarpath));
        if (ok == FALSE)
            *error_msg = str_printf("Error creating '%s'", tc(tarpath));
    }

    if (ok == TRUE)
    {
        ok = ssh_copy(&host->login, flowpath, tc(tarname), &drive->login, tc(wpaths->drive_path), tc(tarname));
        if (ok == FALSE)
            *error_msg = str_printf("Error copying '%s' into '%s'", tc(tarname), tc(wpaths->drive_path));
    }

    if (ok == TRUE)
        log_printf("%s Runner %s[%d]%s '%s%s%s' '%s%s%s' copied into drive", kASCII_SCHED, kASCII_VERSION, runner_id, kASCII_RESET, kASCII_PATH, tc(host->name), kASCII_RESET, kASCII_TARGET, tc(tarname), kASCII_RESET);

    str_destroy(&tarname);
    str_destroy(&tarpath);
    return ok;
}

/*---------------------------------------------------------------------------*/

static Vers i_cmake_version(const Login *login)
{
    Vers vers = {0, 0, 0};
    Stream *stm = ssh_cmake_version(login);
    if (stm != NULL)
    {
        vers = vers_from_stm(stm);
        stm_close(&stm);
    }

    return vers;
}

/*---------------------------------------------------------------------------*/

static String *i_cmake_make_program(const Host *host, const Job *job, const char_t *tempath, String **error_msg)
{
    static const char_t *make_envvar = "CMAKE_MAKE_PROGRAM";
    String *makeprogram = NULL;
    String *cmake_log = NULL;
    bool_t ok = TRUE;
    cassert_no_null(host);
    cassert_no_null(job);
    cassert_no_null(error_msg);
    cassert(*error_msg == NULL);
    if (ok == TRUE)
    {
        ok = ssh_create_dir(&host->login, tempath);
        if (ok == FALSE)
            *error_msg = str_printf("Error creating host '%s' directory '%s'", tc(host->name), tempath);
    }

    if (ok == TRUE)
    {
        Stream *stm = stm_memory(512);
        stm_printf(stm, "message(\"%s=${%s}\")\n", make_envvar, make_envvar);
        ok = ssh_to_file(&host->login, tempath, "CMakeLists.txt", stm);
        if (ok == FALSE)
            *error_msg = str_printf("Error creating host '%s' CMakeLists.txt '%s'", tc(host->name), tempath);
        stm_close(&stm);
    }

    if (ok == TRUE)
    {
        uint32_t ret = ssh_cmake_configure(&host->login, NULL, tempath, tempath, tc(job->generator), "", &cmake_log);
        if (ret != 0)
        {
            ok = FALSE;
            *error_msg = str_printf("Error running cmake, in make_program");
        }
    }

    if (ok == TRUE)
    {
        const char_t *str = str_str(tc(cmake_log), make_envvar);
        if (str != NULL)
        {
            const char_t *end = NULL;
            str += str_len_c(make_envvar) + 1;
            end = str;
            while (*end != '\n')
                end += 1;
            makeprogram = str_cn(str, (uint32_t)(end - str));
        }
        else
        {
            ok = FALSE;
            *error_msg = str_printf("'%s' not found in make_program", make_envvar);
        }
    }

    str_destopt(&cmake_log);
    cassert((ok == TRUE && makeprogram != NULL) || (ok == FALSE && makeprogram == NULL));
    return makeprogram;
}

/*---------------------------------------------------------------------------*/

static bool_t i_run_build(const Host *host, const Drive *drive, const Job *job, const WorkPaths *wpaths, const char_t *flowid, const uint32_t runner_id, String **cmake_log, String **build_log, String **install_log, String **warns, String **errors, uint32_t *nwarns, uint32_t *nerrors, String **error_msg)
{
    bool_t ok = TRUE;
    Vers cmake_vers;
    String *flowpath = NULL;
    String *srcpath = NULL;
    String *buildpath = NULL;
    String *instpath = NULL;
    String *makeprogram = NULL;
    generator_t generator;
    cassert_no_null(host);
    cassert_no_null(job);
    flowpath = str_path(host->login.platform, "%s/%s/%s", tc(host->workpath), flowid, tc(job->name));
    srcpath = str_path(host->login.platform, "%s/src", tc(flowpath));
    buildpath = str_path(host->login.platform, "%s/build", tc(flowpath));
    instpath = str_path(host->login.platform, "%s/install", tc(flowpath));
    generator = i_generator(tc(job->generator));

    if (generator == ENUM_MAX(generator_t))
    {
        cassert_no_null(error_msg);
        cassert(*error_msg == NULL);
        *error_msg = str_printf("Unknown generator '%s'", tc(job->generator));
        ok = FALSE;
    }

    if (ok == TRUE)
        ok = i_create_build_dirs(host, tc(flowpath), runner_id, error_msg);

    if (ok == TRUE)
        ok = i_get_source_package(host, wpaths, tc(flowpath), NBUILD_SRC_TAR, tc(srcpath), runner_id, error_msg);

    if (ok == TRUE)
        cmake_vers = i_cmake_version(&host->login);

    if (ok == TRUE)
    {
        /*
         * cmake versions lower than 3.15.0 doesn't have the --install option
         * we need use the native build command to install
         */
        if (vers_lt(&cmake_vers, 3, 15, 0) == TRUE)
        {
            String *tempath = str_path(host->login.platform, "%s/makeprog", tc(flowpath));
            makeprogram = i_cmake_make_program(host, job, tc(tempath), error_msg);
            str_destroy(&tempath);

            if (makeprogram == NULL)
                ok = FALSE;
        }
    }

    if (ok == TRUE)
        ok = i_cmake_configure(host, job, generator, tc(srcpath), tc(buildpath), runner_id, cmake_log, error_msg);

    if (ok == TRUE)
        ok = i_cmake_build(host, job, generator, tc(buildpath), runner_id, build_log, warns, errors, nwarns, nerrors, error_msg);

    if (ok == TRUE)
        ok = i_cmake_install(host, job, generator, &cmake_vers, tc(makeprogram), tc(buildpath), tc(instpath), runner_id, install_log, error_msg);

    if (ok == TRUE)
        ok = i_copy_to_drive(host, drive, job, tc(flowpath), tc(instpath), wpaths, runner_id, error_msg);

    str_destroy(&flowpath);
    str_destroy(&srcpath);
    str_destroy(&buildpath);
    str_destroy(&instpath);
    str_destopt(&makeprogram);
    return ok;
}

/*---------------------------------------------------------------------------*/

static String *i_test_envvars(const Host *host, const generator_t generator, const char_t *instpath)
{
    Stream *stm = stm_memory(512);
    String *vars = NULL;
    cassert_no_null(host);
    switch (host->login.platform)
    {
    case ekWINDOWS:
        stm_printf(stm, "PATH=%s\\bin;", instpath);
        if (generator == ekGENERATOR_MINGW)
            stm_printf(stm, "%s\\bin;", tc(host->mingw_path));
        stm_printf(stm, "%%PATH%%");
        break;

    case ekMACOS:
        stm_printf(stm, "export DYLD_LIBRARY_PATH=%s/bin:$DYLD_LIBRARY_PATH", instpath);
        break;

    case ekLINUX:
        stm_printf(stm, "export LD_LIBRARY_PATH=%s/bin:$LD_LIBRARY_PATH", instpath);
        break;

        cassert_default();
    }

    vars = stm_str(stm);
    stm_close(&stm);
    return vars;
}

/*---------------------------------------------------------------------------*/

static bool_t i_execute_test(const Host *host, const Job *job, const char_t *buildpath, const char_t *envvars, const char_t *exec, Stream *stm, String **error_msg)
{
    bool_t ok = TRUE;
    String *cmd = NULL;
    String *log = NULL;
    uint32_t ret = UINT32_MAX;
    cassert_no_null(host);
    cassert_no_null(job);
    cassert_no_null(error_msg);
    cmd = str_path(host->login.platform, "%s/%s/bin/%s", buildpath, tc(job->config), exec);
    if (str_empty_c(envvars) == FALSE)
    {
        String *ncmd = NULL;
        if (host->login.platform == ekWINDOWS)
            ncmd = str_printf("%s&%s", envvars, tc(cmd));
        else
            ncmd = str_printf("%s;%s", envvars, tc(cmd));
        str_destroy(&cmd);
        cmd = ncmd;
    }

    ret = ssh_execute_test(&host->login, tc(cmd), &log);
    if (ret == 0)
    {
        stm_writef(stm, tc(log));
        stm_writef(stm, "\n");
    }
    else
    {
        ok = FALSE;
        *error_msg = str_printf("%s: Fatal error running test '%s'", exec, tc(log));
    }

    str_destroy(&cmd);
    str_destroy(&log);
    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_execute_tests(const Host *host, const Job *job, const generator_t generator, const char_t *buildpath, const char_t *instpath, const ArrSt(Target) *tests, String **test_log, String **warns, String **errors, uint32_t *nwarns, uint32_t *nerrors, String **error_msg)
{
    String *envvars = i_test_envvars(host, generator, instpath);
    Stream *stm = stm_memory(2048);
    bool_t ok = TRUE;
    cassert_no_null(job);
    cassert_no_null(test_log);
    cassert_no_null(warns);
    cassert_no_null(errors);
    cassert_no_null(nwarns);
    cassert_no_null(nerrors);

    arrst_foreach_const(test, tests, Target)
        if (str_empty(test->exec) == FALSE)
        {
            ok = i_execute_test(host, job, buildpath, tc(envvars), tc(test->exec), stm, error_msg);
            if (ok == FALSE)
                break;
        }
    arrst_end()

    if (ok == TRUE)
    {
        const char_t *warnmsgs[] = {"[WARN]"};
        const char_t *errmsgs[] = {"[FAIL]"};
        *test_log = stm_str(stm);
        *nwarns = i_get_messages(*test_log, warnmsgs, sizeof(warnmsgs) / sizeof(char_t *), warns);
        *nerrors = i_get_messages(*test_log, errmsgs, sizeof(errmsgs) / sizeof(char_t *), errors);
    }

    str_destroy(&envvars);
    stm_close(&stm);
    return ok;
}

/*---------------------------------------------------------------------------*/

static String *i_unify_b64(String **str1, String **str2, const uint32_t v1, const uint32_t v2, uint32_t *t)
{
    String *str = NULL;
    cassert_no_null(str1);
    cassert_no_null(str2);
    cassert_no_null(t);
    if (!str_empty(*str1) && !str_empty(*str2))
    {
        str = str_printf("%s\n%s", tc(*str1), tc(*str2));
        str_destroy(str1);
        str_destroy(str2);
    }
    else if (!str_empty(*str1))
    {
        str = *str1;
        str_destopt(str2);
    }
    else if (!str_empty(*str2))
    {
        str = *str2;
        str_destopt(str1);
    }
    else
    {
        str_destopt(str1);
        str_destopt(str2);
    }

    *t = v1 + v2;
    if (str != NULL)
    {
        String *b64 = b64_encode_from_str(str);
        cassert(*t > 0);
        str_destroy(&str);
        return b64;
    }
    else
    {
        cassert(*t == 0);
        return NULL;
    }
}

/*---------------------------------------------------------------------------*/

static bool_t i_run_test(const Host *host, const Job *job, const ArrSt(Target) *tests, const WorkPaths *wpaths, const char_t *flowid, const uint32_t runner_id, String **cmake_log, String **build_log, String **test_log, String **warns, String **errors, uint32_t *nwarns, uint32_t *nerrors, String **error_msg)
{
    bool_t ok = TRUE;
    String *flowpath = NULL;
    String *testpath = NULL;
    String *buildpath = NULL;
    String *instpath = NULL;
    String *warbuild = NULL;
    String *errbuild = NULL;
    String *wartest = NULL;
    String *errtest = NULL;
    uint32_t nwarbuild = 0, nerrbuild = 0;
    uint32_t nwartest = 0, nerrtest = 0;
    generator_t generator;
    cassert_no_null(host);
    cassert_no_null(job);
    cassert_no_null(warns);
    cassert_no_null(errors);
    cassert_no_null(nwarns);
    cassert_no_null(nerrors);
    flowpath = str_path(host->login.platform, "%s/%s/%s", tc(host->workpath), flowid, tc(job->name));
    testpath = str_path(host->login.platform, "%s/test", tc(flowpath));
    buildpath = str_path(host->login.platform, "%s/test_build", tc(flowpath));
    instpath = str_path(host->login.platform, "%s/install", tc(flowpath));
    generator = i_generator(tc(job->generator));

    if (generator == ENUM_MAX(generator_t))
    {
        cassert_no_null(error_msg);
        cassert(*error_msg == NULL);
        *error_msg = str_printf("Unknown generator '%s'", tc(job->generator));
        ok = FALSE;
    }

    if (ok == TRUE)
        ok = i_get_source_package(host, wpaths, tc(flowpath), NBUILD_TEST_TAR, tc(testpath), runner_id, error_msg);

    if (ok == TRUE)
    {
        String *opts = str_copy(job->opts);
        String *nopts = str_printf("%s -DCMAKE_INSTALL_PREFIX=%s", tc(opts), tc(instpath));
        str_upd(&cast(job, Job)->opts, tc(nopts));
        ok = i_cmake_configure(host, job, generator, tc(testpath), tc(buildpath), runner_id, cmake_log, error_msg);
        str_upd(&cast(job, Job)->opts, tc(opts));
        str_destroy(&opts);
        str_destroy(&nopts);
    }

    if (ok == TRUE)
        ok = i_cmake_build(host, job, generator, tc(buildpath), runner_id, build_log, &warbuild, &errbuild, &nwarbuild, &nerrbuild, error_msg);

    if (ok == TRUE)
    {
        ok = i_execute_tests(host, job, generator, tc(buildpath), tc(instpath), tests, test_log, &wartest, &errtest, &nwartest, &nerrtest, error_msg);
        if (str_empty(*test_log) == FALSE)
        {
            String *b64 = b64_encode_from_str(*test_log);
            str_destroy(test_log);
            *test_log = b64;
        }
    }

    *warns = i_unify_b64(&warbuild, &wartest, nwarbuild, nwartest, nwarns);
    *errors = i_unify_b64(&errbuild, &errtest, nerrbuild, nerrtest, nerrors);
    str_destroy(&flowpath);
    str_destroy(&testpath);
    str_destroy(&buildpath);
    str_destroy(&instpath);
    return ok;
}

/*---------------------------------------------------------------------------*/

bool_t host_run_build(const Host *host, const Drive *drive, const Job *job, const WorkPaths *wpaths, const uint32_t repo_vers, const char_t *flowid, const uint32_t runner_id, String **cmake_log, String **build_log, String **install_log, String **warns, String **errors, uint32_t *nwarns, uint32_t *nerrors, String **error_msg)
{
    unref(repo_vers);
    return i_run_build(host, drive, job, wpaths, flowid, runner_id, cmake_log, build_log, install_log, warns, errors, nwarns, nerrors, error_msg);
}

/*---------------------------------------------------------------------------*/

bool_t host_run_test(const Host *host, const Job *job, const ArrSt(Target) *tests, const WorkPaths *wpaths, const uint32_t repo_vers, const char_t *flowid, const uint32_t runner_id, String **cmake_log, String **build_log, String **install_log, String **warns, String **errors, uint32_t *nwarns, uint32_t *nerrors, String **error_msg)
{
    unref(repo_vers);
    return i_run_test(host, job, tests, wpaths, flowid, runner_id, cmake_log, build_log, install_log, warns, errors, nwarns, nerrors, error_msg);
}
