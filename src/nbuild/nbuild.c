/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: nbuild.c
 *
 */

/* NAppGUI Build */

#include "nbuild.h"
#include "host.h"
#include "workflow.h"
#include "network.h"
#include "report.h"
#include "prdoc.h"
#include <nlib/ssh.h>
#include <nlib/nlib.h>
#include <encode/json.h>
#include <core/arrst.h>
#include <core/core.h>
#include <core/hfile.h>
#include <core/strings.h>
#include <core/stream.h>
#include <osbs/btime.h>
#include <osbs/log.h>
#include <sewer/sewer.h>
#include <sewer/cassert.h>

/*---------------------------------------------------------------------------*/

const char_t *NBUILD_TMP_FOLDER = "nbuild_master_tmp";
const char_t *NBUILD_REPORT_JSON = "report.json";
const char_t *NBUILD_LOCKFILE = "nbuild.lock";
const char_t *NBUILD_SRC_TAR = "src.tar.gz";
const char_t *NBUILD_TEST_TAR = "test.tar.gz";
const char_t *NBUILD_WEB_TAR = "web.tar.gz";
const char_t *NBUILD_REP_TAR = "rep.tar.gz";
const char_t *NDOC_APP = "ndoc";

/*---------------------------------------------------------------------------*/

static char_t *i_opt(int argc, char *argv[], const char_t *opt)
{
    int i = 0;
    for (i = 0; i < argc; ++i)
    {
        if (str_equ_c(argv[i], opt) && i < argc - 1)
            return argv[i + 1];
    }

    return NULL;
}

/*---------------------------------------------------------------------------*/

static void i_print_usage(void)
{
    log_printf("Use: nbuild -n network.json -w workflow.json\n");
}

/*---------------------------------------------------------------------------*/

static Network *i_network(const char_t *network_file, const ArrSt(uint32_t) *ips)
{
    Stream *stm = stm_from_file(network_file, NULL);
    if (stm != NULL)
    {
        Network *network = json_read(stm, NULL, Network);
        stm_close(&stm);

        if (network != NULL)
        {
            network_localhost(&network->drive.login, ips);
            host_localhost(network->hosts, ips);
        }

        return network;
    }
    else
    {
        log_printf("%s Opening '%s' file.", kASCII_FAIL, network_file);
        return NULL;
    }
}

/*---------------------------------------------------------------------------*/

static bool_t i_check_network(const Network *network)
{
    bool_t ok = TRUE;
    cassert_no_null(network);
    if (ok == TRUE)
    {
        if (network->drive.login.platform != ekLINUX)
        {
            log_printf("%s Drive must be a LINUX machine", kASCII_FAIL);
            ok = FALSE;
        }
    }

    if (ok == TRUE)
        ok = host_check_config(network->hosts);

    return ok;
}

/*---------------------------------------------------------------------------*/

int main(int argc, char *argv[])
{
    bool_t ok = TRUE;
    String *forced_jobs = NULL;
    String *tmppath = NULL;
    String *logpath = NULL;
    String *logfile = NULL;
    Network *network = NULL;

    core_start();
    network_dbind();
    workflow_dbind();
    report_dbind();
    prdoc_dbind();

    /* Config logger */
    if (ok == TRUE)
    {
        Date date;
        String *fname = NULL;
        btime_date(&date);
        fname = str_printf("%04d_%02d_%02d_%02d_%02d_log.txt", date.year, date.month, date.mday, date.hour, date.minute);
        logfile = hfile_appdata(tc(fname));
        str_destroy(&fname);
        log_file(tc(logfile));
    }

    log_printf("nbuild %s", sewer_nappgui_version(TRUE));
    tmppath = hfile_home_dir(NBUILD_TMP_FOLDER);

    /* Create the nbuild temporal folder */
    if (ok == TRUE)
    {
        if (hfile_exists(tc(tmppath), NULL) == FALSE)
        {
            ok = hfile_dir_create(tc(tmppath), NULL);
            if (ok == TRUE)
                log_printf("%s Created '%s%s%s' tmp folder", kASCII_OK, kASCII_PATH, tc(tmppath), kASCII_RESET);
            else
                log_printf("%s Creating '%s' temp folder", kASCII_FAIL, tc(tmppath));
        }
    }

    /* Load build network */
    if (ok == TRUE)
    {
        const char_t *network_file = i_opt(argc, argv, "-n");
        if (network_file != NULL)
        {
            ArrSt(uint32_t) *ips = network_local_ips();

            if (ips != NULL && arrst_size(ips, uint32_t) > 0)
            {
                network = i_network(network_file, ips);
                ok = i_check_network(network);
            }
            else
            {
                log_printf("%s Cannot resolve local ip addresses.", kASCII_FAIL);
            }

            if (network == NULL)
            {
                log_printf("%s Cannot load '%s' network file", kASCII_FAIL, network_file);
                ok = FALSE;
            }

            arrst_destopt(&ips, NULL, uint32_t);
        }
        else
        {
            log_printf("[-n] network file is not provided");
            i_print_usage();
            ok = FALSE;
        }
    }

    if (ok == TRUE)
    {
        const char_t *jobs = i_opt(argc, argv, "-j");
        if (jobs != NULL)
            forced_jobs = str_c(jobs);
        else
            forced_jobs = str_c("");
    }

    /* Load workflow file */
    if (ok == TRUE)
    {
        const char_t *workflow_file = i_opt(argc, argv, "-w");
        if (workflow_file != NULL)
        {
            Workflows *workflows = workflow_create(workflow_file);
            if (workflows != NULL)
            {
                log_printf("%s Drive '%s' '%s%s%s'", kASCII_OK, tc(network->drive.name), kASCII_PATH, tc(network->drive.path), kASCII_RESET);
                logpath = workflow_run(workflows, network, tc(forced_jobs), tc(logfile), tc(tmppath));
            }
            else
            {
                log_printf("%s Cannot load '%s' workflow file.", kASCII_FAIL, workflow_file);
                ok = FALSE;
            }

            json_destopt(&workflows, Workflows);
        }
        else
        {
            log_printf("[-w] workflow file is not provided");
            i_print_usage();
            ok = FALSE;
        }
    }

    /* Copy the log file to the drive */
    if (ok == TRUE)
    {
        if (logpath != NULL)
        {
            String *path = NULL;
            String *file = NULL;
            str_split_pathname(tc(logfile), &path, &file);
            if (ssh_copy(NULL, tc(path), tc(file), &network->drive.login, tc(logpath), tc(file)) == FALSE)
            {
                log_printf("%s Error copy logfile '%s' in '%s' directory.\n", kASCII_FAIL, tc(file), tc(logpath));
                ok = FALSE;
            }
            str_destroy(&path);
            str_destroy(&file);
        }
    }

    log_printf("%s", "");
    log_printf("%s", "");
    json_destopt(&network, Network);
    str_destopt(&forced_jobs);
    str_destroy(&logfile);
    str_destopt(&logpath);
    str_destopt(&tmppath);
    core_finish();
    return ok ? 0 : 1;
}
