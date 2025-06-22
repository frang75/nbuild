/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: workflow.c
 *
 */

/* Workflows */

#include "nbuild.h"
#include "workflow.h"
#include "host.h"
#include "target.h"
#include "prdoc.h"
#include "report.h"
#include "sched.h"
#include <nlib/ssh.h>
#include <nlib/nlib.h>
#include <encode/json.h>
#include <core/arrst.h>
#include <core/arrpt.h>
#include <core/dbind.h>
#include <core/heap.h>
#include <core/hfile.h>
#include <core/regex.h>
#include <core/strings.h>
#include <core/stream.h>
#include <osbs/bfile.h>
#include <osbs/log.h>
#include <sewer/cassert.h>

struct _workflow_t
{
    Global global;
    String *version;
    String *build;
    ArrPt(String) *ignore;
    ArrSt(Target) *sources;
    ArrSt(Target) *tests;
    ArrSt(Job) *jobs;
};

struct _workflows_t
{
    ArrPt(String) *workflows;
};

DeclSt(Workflow);
DeclSt(Workflows);

/*---------------------------------------------------------------------------*/

void workflow_dbind(void)
{
    dbind(Global, String *, project);
    dbind(Global, String *, description);
    dbind(Global, uint32_t, start_year);
    dbind(Global, String *, author);
    dbind(Global, ArrPt(String) *, license);
    dbind(Global, String *, flowid);
    dbind(Global, String *, repo_url);
    dbind(Global, String *, repo_branch);
    dbind(Global, String *, repo_user);
    dbind(Global, String *, repo_pass);
    dbind(Global, String *, doc_repo_url);
    dbind(Global, String *, doc_repo_user);
    dbind(Global, String *, doc_repo_pass);
    dbind(Global, String *, doc_url);
    dbind(Global, String *, web_report_repo_url);
    dbind(Global, String *, web_report_repo_user);
    dbind(Global, String *, web_report_repo_pass);
    dbind(Global, String *, hosting_url);
    dbind(Global, String *, hosting_user);
    dbind(Global, String *, hosting_pass);
    dbind(Global, bool_t, hosting_cert);
    dbind(Global, String *, hosting_docpath);
    dbind(Global, String *, hosting_buildpath);
    dbind(Target, String *, name);
    dbind(Target, String *, dest);
    dbind(Target, String *, url);
    dbind(Target, String *, exec);
    dbind(Target, bool_t, legal);
    dbind(Target, bool_t, format);
    dbind(Target, bool_t, analyzer);
    dbind_default(Target, bool_t, legal, FALSE);
    dbind_default(Target, bool_t, format, TRUE);
    dbind_default(Target, bool_t, analyzer, FALSE);
    dbind(Job, uint32_t, priority);
    dbind(Job, String *, name);
    dbind(Job, String *, config);
    dbind(Job, String *, generator);
    dbind(Job, String *, opts);
    dbind(Job, ArrPt(String) *, tags);
    dbind(Workflow, Global, global);
    dbind(Workflow, String *, version);
    dbind(Workflow, String *, build);
    dbind(Workflow, ArrPt(String) *, ignore);
    dbind(Workflow, ArrSt(Target) *, sources);
    dbind(Workflow, ArrSt(Target) *, tests);
    dbind(Workflow, ArrSt(Job) *, jobs);
    dbind(Workflows, ArrPt(String) *, workflows);
}

/*---------------------------------------------------------------------------*/

static bool_t i_check_targets(ArrSt(Target) *targets, const char_t *repo_url, const char_t *repo_user, const char_t *repo_pass)
{
    bool_t ok = TRUE;

    arrst_foreach(target, targets, Target)
        String *target_url = str_printf("%s/%s", repo_url, tc(target->name));
        target->repo_vers = ssh_repo_version(tc(target_url), repo_user, repo_pass);

        if (target->repo_vers == UINT32_MAX)
        {
            log_printf("%s Unable to get repo version '%s'.", kASCII_FAIL, tc(target_url));
            ok = FALSE;
        }

        str_destroy(&target_url);

    arrst_end()

    /* Check for duplicated targets */
    if (ok == TRUE)
    {
        const Target *target = arrst_all_const(targets, Target);
        uint32_t i, j, n = arrst_size(targets, Target);
        for (i = 0; i < n && ok; ++i)
        {
            for (j = i + 1; j < n && ok; ++j)
            {
                if (str_equ(target[i].name, tc(target[j].name)))
                {
                    log_printf("%s Duplicated target '%s'.", kASCII_FAIL, tc(target[i].name));
                    ok = FALSE;
                }
            }
        }
    }

    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_job_coherent(const Job *job, const ArrSt(Job) *jobs)
{
    cassert_no_null(job);
    if (str_empty(job->name) == TRUE)
    {
        log_printf("%s job '%d' with empty name", kASCII_FAIL, job->id);
        return FALSE;
    }

    /* Look for duplicated */
    arrst_foreach_const(cjob, jobs, Job)
        if (cjob != job)
        {
            if (str_equ(cjob->name, tc(job->name)) == TRUE)
            {
                log_printf("%s Duplicated job '%s'", kASCII_FAIL, tc(job->name));
                return FALSE;
            }
        }
    arrst_end()
    return TRUE;
}

/*---------------------------------------------------------------------------*/

static bool_t i_check_jobs(ArrSt(Job) *jobs)
{
    bool_t ok = TRUE;
    arrst_foreach(job, jobs, Job)
        job->id = job_i;
        if (i_job_coherent(job, jobs) == FALSE)
            ok = FALSE;
    arrst_end()
    return ok;
}

/*---------------------------------------------------------------------------*/

static uint32_t i_repo_vers(const ArrSt(Target) *targets, const ArrSt(Target) *tests, String **repo_vers_info)
{
    uint32_t repo_vers = 0;
    const char_t *info = NULL;
    cassert_no_null(repo_vers_info);

    arrst_foreach_const(target, targets, Target)
        if (target->repo_vers > repo_vers)
        {
            repo_vers = target->repo_vers;
            info = tc(target->name);
        }
    arrst_end()

    arrst_foreach_const(test, tests, Target)
        if (test->repo_vers > repo_vers)
        {
            repo_vers = test->repo_vers;
            info = tc(test->name);
        }
    arrst_end()

    if (info != NULL)
    {
        *repo_vers_info = str_c(info);
        return repo_vers;
    }
    else
    {
        *repo_vers_info = str_c("");
        return UINT32_MAX;
    }
}

/*---------------------------------------------------------------------------*/

static String *i_project_vers(const char_t *repo_url, const uint32_t repo_vers, const char_t *vers_file, const char_t *repo_user, const char_t *repo_pass)
{
    String *vers = NULL;
    String *url = str_printf("%s/%s", repo_url, vers_file);
    Stream *stm = ssh_repo_cat(tc(url), repo_vers, repo_user, repo_pass);

    if (stm != NULL)
    {
        const char_t *line = stm_read_trim(stm);
        if (str_empty_c(line) == FALSE)
            vers = str_c(line);
        stm_close(&stm);
    }

    str_destroy(&url);
    return vers;
}

/*---------------------------------------------------------------------------*/

static bool_t i_ssh_dir(const Login *login, const char_t *dir)
{
    if (ssh_create_dir(login, dir) == TRUE)
    {
        log_printf("%s Directory '%s%s%s'", kASCII_OK, kASCII_PATH, dir, kASCII_RESET);
        return TRUE;
    }
    else
    {
        log_printf("%s Creating directory '%s%s%s'", kASCII_FAIL, kASCII_PATH, dir, kASCII_RESET);
        return FALSE;
    }
}

/*---------------------------------------------------------------------------*/

static bool_t i_local_dir(const char_t *dir)
{
    if (hfile_dir_create(dir, NULL) == TRUE)
    {
        log_printf("%s Directory '%s%s%s'", kASCII_OK, kASCII_PATH, dir, kASCII_RESET);
        return TRUE;
    }
    else
    {
        log_printf("%s Creating directory '%s%s%s'", kASCII_FAIL, kASCII_PATH, dir, kASCII_RESET);
        return FALSE;
    }
}

/*---------------------------------------------------------------------------*/

static void i_destroy_workpaths(WorkPaths **paths)
{
    cassert_no_null(paths);
    if (*paths != NULL)
    {
        str_destopt(&(*paths)->tmp_path);
        str_destopt(&(*paths)->tmp_src);
        str_destopt(&(*paths)->tmp_test);
        str_destopt(&(*paths)->tmp_ndoc);
        str_destopt(&(*paths)->tmp_nrep);
        str_destopt(&(*paths)->drive_path);
        str_destopt(&(*paths)->drive_inf);
        str_destopt(&(*paths)->drive_doc);
        str_destopt(&(*paths)->drive_rep);
        str_destopt(&(*paths)->drive_rep_web);
        heap_delete(paths, WorkPaths);
    }
}

/*---------------------------------------------------------------------------*/

static WorkPaths *i_workpaths(const Drive *drive, const char_t *tmppath, const char_t *flowid, const uint32_t repo_vers, const uint32_t doc_repo_vers)
{
    WorkPaths *path = heap_new0(WorkPaths);
    path->tmp_path = str_cpath("%s/%s", tmppath, flowid);
    path->tmp_src = str_cpath("%s/%s", tc(path->tmp_path), "src");
    path->tmp_test = str_cpath("%s/%s", tc(path->tmp_path), "test");
    path->tmp_ndoc = str_cpath("%s/%s", tc(path->tmp_path), "ndoc_out");
    path->tmp_nrep = str_cpath("%s/%s", tc(path->tmp_path), "ndoc_rep");
    path->drive_path = str_path(drive->login.platform, "%s/%s/r%d", tc(drive->path), flowid, repo_vers);
    path->drive_inf = str_path(drive->login.platform, "%s/%s", tc(path->drive_path), "inf");

    if (doc_repo_vers != UINT32_MAX)
    {
        path->drive_doc = str_path(drive->login.platform, "%s/%s-DOC/r%d", tc(drive->path), flowid, doc_repo_vers);
    }

    path->drive_rep = str_path(drive->login.platform, "%s/%s-REP", tc(drive->path), flowid);
    path->drive_rep_web = str_path(drive->login.platform, "%s/%s-REPWEB/r%d", tc(drive->path), flowid, repo_vers);
    return path;
}

/*---------------------------------------------------------------------------*/

static bool_t i_create_temp_paths(const WorkPaths *paths, const char_t *flowid, bool_t *remove_lockfile)
{
    bool_t ok = TRUE;
    String *lockpath = NULL;
    cassert_no_null(paths);
    cassert_no_null(remove_lockfile);
    cassert(*remove_lockfile == FALSE);

    /* Check if another 'nbuild' instance is running for this flow */
    if (ok == TRUE)
    {
        lockpath = str_cpath("%s/%s", tc(paths->tmp_path), NBUILD_LOCKFILE);
        if (hfile_exists(tc(lockpath), NULL) == TRUE)
        {
            log_printf("%s another instance of nbuild is running for this flow '%s'", kASCII_FAIL, flowid);
            ok = FALSE;
        }
    }

    /* Remove previous nbuild temporal folder */
    if (ok == TRUE)
    {
        if (hfile_exists(tc(paths->tmp_path), NULL) == TRUE)
        {
            ok = hfile_dir_destroy(tc(paths->tmp_path), NULL);
            if (ok == FALSE)
                log_printf("%s Removing previous '%s' temp folder", kASCII_FAIL, tc(paths->tmp_path));
        }
    }

    /* Create the nbuild temporal folder */
    if (ok == TRUE)
        ok = i_local_dir(tc(paths->tmp_path));

    /* Set the cwd in temporal folder */
    if (ok == TRUE)
    {
        ok = bfile_dir_set_work(tc(paths->tmp_path), NULL);
        if (ok == FALSE)
            log_printf("%s Error setting working directory '%s'", kASCII_FAIL, tc(paths->tmp_path));
    }

    /* Create the lockfile */
    if (ok == TRUE)
    {
        if (hfile_from_string(tc(lockpath), lockpath, NULL) == TRUE)
        {
            *remove_lockfile = TRUE;
        }
        else
        {
            ok = FALSE;
            log_printf("%s Error creating lockfile '%s'", kASCII_FAIL, tc(lockpath));
        }
    }

    /* Temporal subfolders */
    if (ok == TRUE)
        ok = i_local_dir(tc(paths->tmp_src));

    if (ok == TRUE)
        ok = i_local_dir(tc(paths->tmp_test));

    if (ok == TRUE)
        ok = i_local_dir(tc(paths->tmp_ndoc));

    if (ok == TRUE)
        ok = i_local_dir(tc(paths->tmp_nrep));

    str_destopt(&lockpath);
    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_create_remote_paths(const WorkPaths *paths, const Login *drive)
{
    bool_t ok = TRUE;
    cassert_no_null(paths);
    if (ok == TRUE)
        ok = i_ssh_dir(drive, tc(paths->drive_path));

    if (ok == TRUE)
        ok = i_ssh_dir(drive, tc(paths->drive_inf));

    if (ok == TRUE && str_empty(paths->drive_doc) == FALSE)
        ok = i_ssh_dir(drive, tc(paths->drive_doc));

    if (ok == TRUE)
        ok = i_ssh_dir(drive, tc(paths->drive_rep));

    if (ok == TRUE)
        ok = i_ssh_dir(drive, tc(paths->drive_rep_web));

    return ok;
}

/*---------------------------------------------------------------------------*/

static void i_save_report(const Report *report, const Login *login, const char_t *infdir)
{
    Stream *stm_json = stm_memory(2048);
    json_write(stm_json, report, NULL, Report);

    /*
    {
        const byte_t *buffer = stm_buffer(stm_json);
        uint32_t size = stm_buffer_size(stm_json);
        bstd_printf(">>>>>>>>>>\n");
        bstd_write(buffer, size, NULL);
        bstd_printf("\n");
        bstd_printf("<<<<<<<<<<\n");
    }
    */

    if (ssh_to_file(login, infdir, NBUILD_REPORT_JSON, stm_json) == FALSE)
        log_printf("%s Writing '%s'.", kASCII_FAIL, NBUILD_REPORT_JSON);

    stm_close(&stm_json);
}

/*---------------------------------------------------------------------------*/

static ArrPt(RegEx) *i_ignore_regex(const ArrPt(String) *ignore)
{
    ArrPt(RegEx) *regex = arrpt_create(RegEx);
    arrpt_foreach_const(ign, ignore, String)
        RegEx *reg = regex_create(tc(ign));
        arrpt_append(regex, reg, RegEx);
    arrpt_end()
    return regex;
}

/*---------------------------------------------------------------------------*/

static bool_t i_with_tests(const ArrSt(Target) *tests)
{
    arrst_foreach_const(test, tests, Target)
        if (str_empty(test->exec) == FALSE)
            return TRUE;
    arrst_end()
    return FALSE;
}

/*---------------------------------------------------------------------------*/

static String *i_run(const Workflow *workflow, const Network *network, const char_t *forced_jobs, const char_t *logfile, const char_t *tmppath)
{
    bool_t ok = TRUE;
    bool_t remove_lockfile = FALSE;
    const Global *global = NULL;
    const Drive *drive = NULL;
    String *repo_url = NULL;
    uint32_t repo_vers_branch = UINT32_MAX;
    uint32_t repo_vers = UINT32_MAX;
    uint32_t doc_repo_vers = UINT32_MAX;
    String *repo_vers_info = NULL;
    String *project_vers = NULL;
    ArrPt(RegEx) *ignore_regex = NULL;
    WorkPaths *wpaths = NULL;
    Report *report = NULL;
    cassert_no_null(workflow);
    global = &workflow->global;
    drive = &network->drive;

    /* Current repo version (build branch) */
    if (ok == TRUE)
    {
        repo_url = str_printf("%s/%s", tc(global->repo_url), tc(global->repo_branch));
        repo_vers_branch = ssh_repo_version(tc(repo_url), tc(global->repo_user), tc(global->repo_pass));
        if (repo_vers_branch == UINT32_MAX)
        {
            log_printf("%s Unable to get repo version '%s'.", kASCII_FAIL, tc(repo_url));
            ok = FALSE;
        }
    }

    /* Check if workflow inputs are correct */
    if (ok == TRUE)
        ok = i_check_targets(workflow->sources, tc(repo_url), tc(global->repo_user), tc(global->repo_pass));

    if (ok == TRUE)
        ok = i_check_targets(workflow->tests, tc(repo_url), tc(global->repo_user), tc(global->repo_pass));

    if (ok == TRUE)
        ok = i_check_jobs(workflow->jobs);

    /* Get the repo version to build */
    if (ok == TRUE)
    {
        repo_vers = i_repo_vers(workflow->sources, workflow->tests, &repo_vers_info);
        if (repo_vers == UINT32_MAX)
        {
            log_printf("%s Nothing to build. Empty targets/paths", kASCII_FAIL);
            ok = FALSE;
        }
    }

    /* Get the project version from version file */
    if (ok == TRUE)
    {
        project_vers = i_project_vers(tc(repo_url), repo_vers, tc(workflow->version), tc(global->repo_user), tc(global->repo_pass));
        if (project_vers == NULL)
        {
            log_printf("%s Unable to get project version '%s'", kASCII_FAIL, tc(workflow->version));
            ok = FALSE;
        }
    }

    /* Get the documentation repo version */
    if (ok == TRUE)
    {
        if (str_empty(global->doc_repo_url) == FALSE)
        {
            doc_repo_vers = ssh_repo_version(tc(global->doc_repo_url), tc(global->doc_repo_user), tc(global->doc_repo_pass));
            if (doc_repo_vers == UINT32_MAX)
            {
                log_printf("%s Unable to get documentation repo version '%s'.", kASCII_FAIL, tc(global->doc_repo_url));
                ok = FALSE;
            }
        }
    }

    /* Welcome messages */
    if (ok == TRUE)
    {
        log_printf("%s %s (%s) - %s", kASCII_OK, tc(global->project), tc(project_vers), tc(global->description));
        log_printf("%s Branch: %s%d%s '%s%s%s'", kASCII_OK, kASCII_VERSION, repo_vers_branch, kASCII_RESET, kASCII_PATH, tc(global->repo_branch), kASCII_RESET);
        log_printf("%s Repo vers: %s%d%s '%s%s%s'", kASCII_OK, kASCII_VERSION, repo_vers, kASCII_RESET, kASCII_PATH, tc(repo_vers_info), kASCII_RESET);
        if (doc_repo_vers != UINT32_MAX)
            log_printf("%s Doc repo vers: %s%d%s", kASCII_OK, kASCII_VERSION, doc_repo_vers, kASCII_RESET);
        else
            log_printf("%s No documentation will be generated ('doc_repo_url')", kASCII_WARN);
    }

    /* Ignore regular expresions */
    if (ok == TRUE)
        ignore_regex = i_ignore_regex(workflow->ignore);

    /* Directories */
    if (ok == TRUE)
        wpaths = i_workpaths(drive, tmppath, tc(global->flowid), repo_vers, doc_repo_vers);

    if (ok == TRUE)
        ok = i_create_temp_paths(wpaths, tc(global->flowid), &remove_lockfile);

    if (ok == TRUE)
        ok = i_create_remote_paths(wpaths, &drive->login);

    /* Loading report */
    if (ok == TRUE)
    {
        if (ssh_file_exists(&drive->login, tc(wpaths->drive_inf), NBUILD_REPORT_JSON) == TRUE)
        {
            Stream *stm = ssh_file_cat(&drive->login, tc(wpaths->drive_inf), NBUILD_REPORT_JSON);
            if (stm != NULL)
            {
                report = json_read(stm, NULL, Report);
                cassert_no_null(report);
                log_printf("%s Readed '%sreport.json%s'", kASCII_OK, kASCII_TARGET, kASCII_RESET);
                stm_close(&stm);
                report_loop_incr(report);
            }
            else
            {
                log_printf("%s Reading '%s'", kASCII_FAIL, NBUILD_REPORT_JSON);
                ok = FALSE;
            }
        }
        else
        {
            report = dbind_create(Report);
            report_init(report, tc(repo_url), repo_vers);
            log_printf("%s Created '%sreport.json%s'", kASCII_OK, kASCII_TARGET, kASCII_RESET);
        }
    }

    /* CI loop */
    if (ok == TRUE)
    {
        uint32_t loop_id = report_loop_current(report);
        log_printf("%s CI/CD current Loop: %s%d%s", kASCII_OK, kASCII_VERSION, loop_id, kASCII_RESET);
        report_loop_init(report);
    }

    /* Target source files */
    if (ok == TRUE)
    {
        String *format_file = target_clang_format_file(workflow->sources, tc(repo_url), tc(global->repo_user), tc(global->repo_pass), repo_vers, tc(wpaths->tmp_path));
        arrst_foreach_const(target, workflow->sources, Target)
            const String *name = str_empty(target->dest) ? target->name : target->dest;
            REvent *event = report_target_event(report, tc(name));
            RState state;
            report_event_state(report, event, &state);
            if (state.done == FALSE)
            {
                bool_t formated = FALSE;
                bool_t legalized = FALSE;
                ok = target_target(target, global, ignore_regex, repo_vers, tc(format_file), tc(wpaths->tmp_src), "Source", event, report, &formated, &legalized);
                report_target_set(report, tc(name), legalized, formated, target->analyzer);
            }
            if (ok == FALSE)
                break;
        arrst_end()
        str_destopt(&format_file);
    }

    /* Copy the tests */
    if (ok == TRUE)
    {
        String *format_file = target_clang_format_file(workflow->tests, tc(repo_url), tc(global->repo_user), tc(global->repo_pass), repo_vers, tc(wpaths->tmp_path));
        arrst_foreach_const(test, workflow->tests, Target)
            const String *name = str_empty(test->dest) ? test->name : test->dest;
            REvent *event = report_test_event(report, tc(name));
            RState state;
            report_event_state(report, event, &state);
            if (state.done == FALSE)
            {
                bool_t formated = FALSE;
                bool_t legalized = FALSE;
                ok = target_target(test, global, ignore_regex, repo_vers, NULL, tc(wpaths->tmp_test), "Test", event, report, &formated, &legalized);
                report_test_set(report, tc(name), legalized, formated, test->analyzer);
            }
            if (ok == FALSE)
                break;
        arrst_end()
        str_destopt(&format_file);
    }

    /* Copy 'build.txt' */
    if (ok == TRUE)
        ok = target_build_file(tc(workflow->build), repo_vers, wpaths, report);

    /* Compress source package */
    if (ok == TRUE)
    {
        REvent *event = report_src_tar_event(report);
        ok = target_tar(&drive->login, wpaths, tc(wpaths->tmp_src), NBUILD_SRC_TAR, event, report);
    }

    /* Compress test package */
    if (ok == TRUE)
    {
        if (arrst_size(workflow->tests, Target) > 0)
        {
            REvent *event = report_test_tar_event(report);
            ok = target_tar(&drive->login, wpaths, tc(wpaths->tmp_test), NBUILD_TEST_TAR, event, report);
        }
    }

    /* 'ndoc' project documentation */
    if (ok == TRUE && doc_repo_vers != UINT32_MAX)
        ok = prdoc_generate(global, &drive->login, tc(project_vers), repo_vers, doc_repo_vers, wpaths, report);

    /* Jobs */
    if (ok == TRUE && report_can_start_jobs(report, doc_repo_vers) == TRUE)
    {
        ArrSt(SJob) *seljobs = arrst_create(SJob);
        bool_t with_tests = i_with_tests(workflow->tests);

        if (str_empty_c(forced_jobs) == TRUE)
        {
            report_select_jobs(report, workflow->jobs, seljobs, with_tests);
        }
        else
        {
            log_printf("%s Forced jobs with pattern '%s%s%s'", kASCII_OK, kASCII_TARGET, forced_jobs, kASCII_RESET);
            report_force_jobs(report, forced_jobs, workflow->jobs, seljobs, with_tests);
        }

        if (arrst_size(seljobs, SJob) > 0)
        {
            sched_start(seljobs, network->hosts, drive, workflow->tests, wpaths, tc(global->flowid), repo_vers, report);
        }
        else
        {
            log_printf("%s No jobs pending, nothing to do", kASCII_WARN);
        }

        arrst_destroy(&seljobs, NULL, SJob);
    }

    /* Update report */
    if (ok == TRUE)
    {
        report_loop_end(report, logfile);
        i_save_report(report, &drive->login, tc(wpaths->drive_inf));
    }

    /* Generate build report web page */
    if (ok == TRUE)
    {
        if (str_empty(global->web_report_repo_url) == FALSE)
            ok = prdoc_buildrep_generate(global, workflow->jobs, &drive->login, tc(project_vers), repo_vers, wpaths, report);
        else
            log_printf("%s No web report will be generated ('web_report_repo_url')", kASCII_WARN);
    }

    /* Show report */
    if (ok == TRUE)
    {
        log_printf("%s", "");
        report_log(report, global, repo_vers);
    }

    if (remove_lockfile == TRUE)
    {
        String *lockpath = str_cpath("%s/%s", tc(wpaths->tmp_path), NBUILD_LOCKFILE);
        bfile_delete(tc(lockpath), NULL);
        str_destroy(&lockpath);
    }

    arrpt_destopt(&ignore_regex, regex_destroy, RegEx);
    str_destopt(&repo_vers_info);
    str_destopt(&project_vers);
    str_destopt(&repo_url);
    dbind_destopt(&report, Report);

    if (wpaths != NULL)
    {
        String *drive_inf = str_copy(wpaths->drive_inf);
        i_destroy_workpaths(&wpaths);
        return drive_inf;
    }
    else
    {
        return str_cpath("%s/%s", tmppath, tc(global->flowid));
    }
}

/*---------------------------------------------------------------------------*/

Workflows *workflow_create(const char_t *workflow_file)
{
    /* Only one workflow */
    Workflows *workflows = heap_new(Workflows);
    String *workflow = str_c(workflow_file);
    workflows->workflows = arrpt_create(String);
    arrpt_append(workflows->workflows, workflow, String);
    return workflows;
}

/*---------------------------------------------------------------------------*/

String *workflow_run(Workflows *workflows, const Network *network, const char_t *forced_jobs, const char_t *logfile, const char_t *tmppath)
{
    String *infdir = NULL;
    cassert_no_null(workflows);
    cassert(arrpt_size(workflows->workflows, String) == 1);
    arrpt_foreach(pfile, workflows->workflows, String)
        Stream *stm = stm_from_file(tc(pfile), NULL);
        if (stm)
        {
            Workflow *workflow = json_read(stm, NULL, Workflow);
            if (workflow)
            {
                log_printf("%s Running workflow '%s%s%s'", kASCII_OK, kASCII_PATH, tc(pfile), kASCII_RESET);
                infdir = i_run(workflow, network, forced_jobs, logfile, tmppath);
                json_destroy(&workflow, Workflow);
            }
            else
            {
                log_printf("%s Parsing workflow file '%s'", kASCII_FAIL, tc(pfile));
            }
            stm_close(&stm);
        }
        else
        {
            log_printf("%s Reading workflow file '%s'", kASCII_FAIL, tc(pfile));
        }
    arrpt_end()

    return infdir;
}
