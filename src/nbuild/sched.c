/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: sched.c
 *
 */

/* nbuild scheduler */

#include "sched.h"
#include "host.h"
#include "report.h"
#include "nboot.h"
#include "nbuild.h"
#include <nlib/nlib.h>
#include <nlib/ssh.h>
#include <core/arrst.h>
#include <core/heap.h>
#include <core/hfile.h>
#include <core/strings.h>
#include <osbs/bthread.h>
#include <osbs/bmutex.h>
#include <osbs/log.h>
#include <sewer/cassert.h>

typedef struct _runner_t Runner;
typedef struct _task_t Task;
typedef struct _schedul_t Schedul;

typedef enum _taskst_t
{
    ekTASK_PENDING,
    ekTASK_RUNNING,
    ekTASK_DONE
} taskst_t;

struct _runner_t
{
    /* Access to 'Runner' data from its own thread is safe */
    const Host *host;
    const ArrSt(Host) *all_hosts;
    const Global *global;
    const Drive *drive;
    const ArrSt(Target) *tests;
    const WorkPaths *wpaths;
    const char_t *flowid;
    Report *report;
    uint32_t repo_vers;
    runstate_t state;
    uint32_t thread_id;
    Thread *thread;

    /* Access to 'sched' from runner thread must be mutual exclusion */
    Schedul *sched;
};

struct _task_t
{
    const SJob *sjob;
    const Runner *runner;
    uint32_t job_id;
    taskst_t state;
};

struct _schedul_t
{
    Mutex *mutex;
    ArrSt(Runner) *runners;
    ArrSt(Task) *tasks;
};

/*---------------------------------------------------------------------------*/

DeclSt(Runner);
DeclSt(Task);

/*---------------------------------------------------------------------------*/

static const char_t *i_BUILD_STEP = "build";
static const char_t *i_TEST_STEP = "test";

/*---------------------------------------------------------------------------*/

static uint32_t i_priority(const ArrSt(SJob) *seljobs)
{
    const SJob *sjob = arrst_first_const(seljobs, SJob);
    cassert_no_null(sjob->job);
    return sjob->job->priority;
}

/*---------------------------------------------------------------------------*/

static void i_remove_runner(Runner *runner)
{
    cassert_no_null(runner);
    if (runner->thread != NULL)
        bthread_close(&runner->thread);
}

/*---------------------------------------------------------------------------*/

static Schedul *i_scheduler(void)
{
    Schedul *schel = heap_new0(Schedul);
    schel->mutex = bmutex_create();
    schel->runners = arrst_create(Runner);
    schel->tasks = arrst_create(Task);
    return schel;
}

/*---------------------------------------------------------------------------*/

static void i_destroy_scheduler(Schedul **sched)
{
    cassert_no_null(sched);
    cassert_no_null(*sched);
    arrst_destroy(&(*sched)->runners, i_remove_runner, Runner);
    arrst_destroy(&(*sched)->tasks, NULL, Task);
    bmutex_close(&(*sched)->mutex);
    heap_delete(sched, Schedul);
}

/*---------------------------------------------------------------------------*/

static Runner *i_add_runner(const Global *global, ArrSt(Runner) *runners, const Host *host, const ArrSt(Host) *all_hosts, const Drive *drive, const ArrSt(Target) *tests, const WorkPaths *wpaths, const char_t *flowid, Report *report, uint32_t repo_vers)
{
    /* Check if runner already exists */
    {
        const char_t *hname = host_name(host);
        arrst_foreach(runner, runners, Runner)
            const char_t *rname = host_name(runner->host);
            if (str_equ_c(hname, rname) == TRUE)
            {
                cassert(host == runner->host);
                return runner;
            }
        arrst_end()
    }

    {
        Runner *runner = arrst_new0(runners, Runner);
        runner->global = global;
        runner->host = host;
        runner->all_hosts = all_hosts;
        runner->drive = drive;
        runner->tests = tests;
        runner->wpaths = wpaths;
        runner->flowid = flowid;
        runner->report = report;
        runner->repo_vers = repo_vers;
        runner->state = ekRUNSTATE_NOT_INIT;
        return runner;
    }
}

/*---------------------------------------------------------------------------*/

static const char_t *i_state_str(const runstate_t state)
{
    switch (state)
    {
    case ekRUNSTATE_NOT_INIT:
        return "NOT_INITIALIZED";
    case ekRUNSTATE_ALREADY_RUNNING:
        return "ALREADY_RUNNING";
    case ekRUNSTATE_VBOX_HOST_DOWN:
        return "VBOX_HOST_DOWN";
    case ekRUNSTATE_VBOX_HOST_SSH:
        return "VBOX_HOST_SSH";
    case ekRUNSTATE_VBOX_HOST_VBOXMANAGE:
        return "ekRUNSTATE_VBOX_HOST_VBOXMANAGE";
    case ekRUNSTATE_VBOX_WAKE_UP:
        return "VIRTUALBOX_WAKEUP";
    case ekRUNSTATE_VBOX_TIMEOUT:
        return "VIRTUALBOX_TIMEOUT";
    case ekRUNSTATE_UTM_HOST_DOWN:
        return "ekRUNSTATE_UTM_HOST_DOWN";
    case ekRUNSTATE_UTM_HOST_SSH:
        return "ekRUNSTATE_UTM_HOST_SSH";
    case ekRUNSTATE_UTM_HOST_UTMCTL:
        return "ekRUNSTATE_UTM_HOST_UTMCTL";
    case ekRUNSTATE_UTM_WAKE_UP:
        return "ekRUNSTATE_UTM_WAKE_UP";
    case ekRUNSTATE_UTM_TIMEOUT:
        return "ekRUNSTATE_UTM_TIMEOUT";
    case ekRUNSTATE_VMWARE_HOST_DOWN:
        return "ekRUNSTATE_VMWARE_HOST_DOWN";
    case ekRUNSTATE_VMWARE_HOST_SSH:
        return "ekRUNSTATE_VMWARE_HOST_SSH";
    case ekRUNSTATE_VMWARE_HOST_VMRUN:
        return "ekRUNSTATE_VMWARE_HOST_VMRUN";
    case ekRUNSTATE_VMWARE_WAKE_UP:
        return "ekRUNSTATE_VMWARE_WAKE_UP";
    case ekRUNSTATE_VMWARE_TIMEOUT:
        return "ekRUNSTATE_VMWARE_TIMEOUT";
    case ekRUNSTATE_MACOS_UNKNOWN_VERSION:
        return "MACOS_UNKNWN_VERSION";
    case ekRUNSTATE_MACOS_NOT_BOOTABLE:
        return "MACOS_NOT_BOOTABLE";
    case ekRUNSTATE_MACOS_WAKE_UP:
        return "MACOS_WAKE_UP";
    case ekRUNSTATE_MACOS_CANT_BOOT_FROM_VOLUME:
        return "MACOS_CANT_BOOT_FROM_VOLUME";
    case ekRUNSTATE_MACOS_TIMEOUT:
        return "MACOS_TIMEOUT";
    case ekRUNSTATE_UNREACHABLE:
        return "UNREACHABLE";
    default:
        cassert_default(state);
    }

    return "Unknown";
}

/*---------------------------------------------------------------------------*/

static Task *i_select_task_for_runner(Runner *runner)
{
    Task *stask = NULL;
    cassert_no_null(runner);
    cassert_no_null(runner->sched);
    bmutex_lock(runner->sched->mutex);
    arrst_foreach(task, runner->sched->tasks, Task)
        cassert(!(task->state == ekTASK_RUNNING && task->runner == runner));
        if (task->state == ekTASK_PENDING && task->runner == runner)
        {
            stask = task;
            task->state = ekTASK_RUNNING;
            break;
        }
    arrst_end()
    bmutex_unlock(runner->sched->mutex);
    return stask;
}

/*---------------------------------------------------------------------------*/

static void i_finish_runner_task(Task *task)
{
    cassert_no_null(task);
    cassert(task->runner != NULL);
    bmutex_lock(task->runner->sched->mutex);
    cassert(task->state == ekTASK_RUNNING);
    task->state = ekTASK_DONE;
    bmutex_unlock(task->runner->sched->mutex);
}

/*---------------------------------------------------------------------------*/

static uint32_t i_run_runner_thread(Runner *runner)
{
    bool_t ok = TRUE;
    const Login *login = NULL;
    cassert_no_null(runner);
    cassert_no_null(runner->global);
    login = host_login(runner->host);

    /* Booting the runner */
    log_printf("%s Runner %s[%d]%s '%s%s%s' booting '%s%s%s'", kASCII_SCHED, kASCII_VERSION, runner->thread_id, kASCII_RESET, kASCII_PATH, host_name(runner->host), kASCII_RESET, kASCII_TARGET, tc(login->ip), kASCII_RESET);
    ok = nboot_boot(runner->host, runner->all_hosts, &runner->state);
    if (ok == FALSE)
        log_printf("%s Runner %s[%d]%s '%s%s%s' cannot be booted '%s%s::%s%s'", kASCII_SCHED_FAIL, kASCII_VERSION, runner->thread_id, kASCII_RESET, kASCII_PATH, host_name(runner->host), kASCII_RESET, kASCII_ERROR, tc(login->ip), i_state_str(runner->state), kASCII_RESET);

    /* The runner is up */
    if (ok == TRUE)
    {
        for (;;)
        {
            Task *task = i_select_task_for_runner(runner);
            if (task != NULL)
            {
                /* Ready for configurable steps/pipeline */
                {
                    RState build_state;
                    report_job_state(runner->report, task->sjob->id, i_BUILD_STEP, &build_state);

                    if (build_state.done == FALSE)
                    {
                        bool_t tok = TRUE;
                        String *msg = str_printf("Job '%s%s%s'", kASCII_TARGET, tc(task->sjob->job->name), kASCII_RESET);
                        String *error_msg = NULL;
                        String *cmake_log = NULL;
                        String *build_log = NULL;
                        String *install_log = NULL;
                        String *warns = NULL;
                        String *errors = NULL;
                        uint32_t nwarns = 0;
                        uint32_t nerrors = 0;
                        const char_t *hostname = host_name(runner->host);
                        report_job_init(runner->report, task->sjob->id, i_BUILD_STEP);
                        log_printf("%s Runner %s[%d]%s '%s%s%s' beginning job %s[%d]%s '%s%s%s'", kASCII_SCHED, kASCII_VERSION, runner->thread_id, kASCII_RESET, kASCII_PATH, host_name(runner->host), kASCII_RESET, kASCII_VERSION, task->job_id, kASCII_RESET, kASCII_TARGET, tc(task->sjob->job->name), kASCII_RESET);
                        tok = host_run_build(runner->host, runner->drive, task->sjob->job, tc(runner->global->project), runner->wpaths, runner->repo_vers, runner->flowid, runner->thread_id, &cmake_log, &build_log, &install_log, &warns, &errors, &nwarns, &nerrors, &error_msg);
                        report_job_end(runner->report, task->sjob->id, i_BUILD_STEP, tok, &error_msg);
                        report_job_state(runner->report, task->sjob->id, i_BUILD_STEP, &build_state);
                        log_printf("%s Runner %s[%d]%s '%s%s%s' complete job %s[%d]%s '%s%s%s'", kASCII_SCHED, kASCII_VERSION, runner->thread_id, kASCII_RESET, kASCII_PATH, host_name(runner->host), kASCII_RESET, kASCII_VERSION, task->job_id, kASCII_RESET, kASCII_TARGET, tc(task->sjob->job->name), kASCII_RESET);
                        report_job(runner->report, task->sjob->id, i_BUILD_STEP, hostname, &cmake_log, &build_log, &install_log, &warns, &errors, nwarns, nerrors);
                        report_state_log(&build_state, tc(msg));
                        str_destroy(&msg);
                    }
                }

                if (report_job_can_test(runner->report, task->sjob->id) == TRUE)
                {
                    RState test_state;
                    bool_t tok = TRUE;
                    String *msg = str_printf("Test '%s%s%s'", kASCII_TARGET, tc(task->sjob->job->name), kASCII_RESET);
                    String *error_msg = NULL;
                    String *cmake_log = NULL;
                    String *build_log = NULL;
                    String *install_log = NULL;
                    String *warns = NULL;
                    String *errors = NULL;
                    uint32_t nwarns = 0;
                    uint32_t nerrors = 0;
                    const char_t *hostname = host_name(runner->host);
                    report_job_init(runner->report, task->sjob->id, i_TEST_STEP);
                    log_printf("%s Runner %s[%d]%s '%s%s%s' beginning test %s[%d]%s '%s%s%s'", kASCII_SCHED, kASCII_VERSION, runner->thread_id, kASCII_RESET, kASCII_PATH, host_name(runner->host), kASCII_RESET, kASCII_VERSION, task->job_id, kASCII_RESET, kASCII_TARGET, tc(task->sjob->job->name), kASCII_RESET);
                    tok = host_run_test(runner->host, task->sjob->job, runner->tests, runner->wpaths, runner->repo_vers, runner->flowid, runner->thread_id, &cmake_log, &build_log, &install_log, &warns, &errors, &nwarns, &nerrors, &error_msg);
                    report_job_end(runner->report, task->sjob->id, i_TEST_STEP, tok, &error_msg);
                    report_job_state(runner->report, task->sjob->id, i_TEST_STEP, &test_state);
                    log_printf("%s Runner %s[%d]%s '%s%s%s' complete test %s[%d]%s '%s%s%s'", kASCII_SCHED, kASCII_VERSION, runner->thread_id, kASCII_RESET, kASCII_PATH, host_name(runner->host), kASCII_RESET, kASCII_VERSION, task->job_id, kASCII_RESET, kASCII_TARGET, tc(task->sjob->job->name), kASCII_RESET);
                    report_job(runner->report, task->sjob->id, i_TEST_STEP, hostname, &cmake_log, &build_log, &install_log, &warns, &errors, nwarns, nerrors);
                    report_state_log(&test_state, tc(msg));
                    str_destroy(&msg);
                }

                i_finish_runner_task(task);
            }
            else
            {
                break;
            }
        }
    }

    /* Shutdown */
    if (ok == TRUE)
    {
        bool_t shutdown = nboot_shutdown(runner->host, runner->all_hosts, runner->state);
        if (shutdown == TRUE)
            log_printf("%s Runner %s[%d]%s '%s%s%s' shutting down", kASCII_SCHED, kASCII_VERSION, runner->thread_id, kASCII_RESET, kASCII_PATH, host_name(runner->host), kASCII_RESET);
    }

    return 0;
}

/*---------------------------------------------------------------------------*/

static bool_t i_prepare_before_runners(const Login *drive, const ArrSt(Task) *tasks, const ArrSt(Target) *tests, const WorkPaths *wpaths)
{
    bool_t ok = TRUE;
    bool_t with_build_tasks = FALSE;
    bool_t with_test_tasks = FALSE;
    cassert_no_null(wpaths);

    if (arrst_size(tasks, Task) > 0)
        with_build_tasks = TRUE;

    arrst_foreach_const(test, tests, Target)
        if (str_empty(test->exec) == FALSE)
        {
            with_test_tasks = TRUE;
            break;
        }
    arrst_end()

    /*
     * Get the source package from drive.
     * Runners will not get the source code from repo, but from previous generated src package.
     * This action will leave the package ready for distribute to runners
     */
    if (with_build_tasks == TRUE)
    {
        String *tarpath = str_cpath("%s/%s", tc(wpaths->tmp_path), NBUILD_SRC_TAR);
        if (hfile_exists(tc(tarpath), NULL) == FALSE)
        {
            ok = ssh_copy(drive, tc(wpaths->drive_path), NBUILD_SRC_TAR, NULL, tc(wpaths->tmp_path), NBUILD_SRC_TAR, FALSE);
            if (ok == FALSE)
                log_printf("%s Error copying '%s' from '%s'", kASCII_SCHED_FAIL, NBUILD_SRC_TAR, tc(wpaths->drive_path));
        }

        str_destroy(&tarpath);
    }

    /* Same for tests */
    if (with_test_tasks == TRUE)
    {
        String *tarpath = str_cpath("%s/%s", tc(wpaths->tmp_path), NBUILD_TEST_TAR);
        if (hfile_exists(tc(tarpath), NULL) == FALSE)
        {
            ok = ssh_copy(drive, tc(wpaths->drive_path), NBUILD_TEST_TAR, NULL, tc(wpaths->tmp_path), NBUILD_TEST_TAR, FALSE);
            if (ok == FALSE)
                log_printf("%s Error copying '%s' from '%s'", kASCII_SCHED_FAIL, NBUILD_TEST_TAR, tc(wpaths->drive_path));
        }

        str_destroy(&tarpath);
    }

    return ok;
}

/*---------------------------------------------------------------------------*/

void sched_start(const Global *global, ArrSt(SJob) *seljobs, const ArrSt(Host) *hosts, const Drive *drive, const ArrSt(Target) *tests, const WorkPaths *wpaths, const char_t *flowid, const uint32_t repo_vers, Report *report)
{
    bool_t ok = TRUE;
    Schedul *sched = i_scheduler();

    cassert_no_null(drive);

    /* Initial log messages */
    log_printf("%s Beginning jobs with %s%d%s priority", kASCII_SCHED, kASCII_VERSION, i_priority(seljobs), kASCII_RESET);
    arrst_foreach_const(sjob, seljobs, SJob)
        cassert_no_null(sjob->job);
        log_printf("%s %s[%d]%s '%s%s%s'", kASCII_SCHED, kASCII_VERSION, sjob_i, kASCII_RESET, kASCII_TARGET, tc(sjob->job->name), kASCII_RESET);
    arrst_end()

    /* Look for runners that can run the jobs */
    arrst_foreach_const(sjob, seljobs, SJob)
        Task *task = arrst_new(sched->tasks, Task);
        const char_t *hostname = report_job_host(report, sjob);
        const Host *host = NULL;

        task->sjob = sjob;
        task->runner = NULL;
        task->job_id = sjob_i;
        task->state = ekTASK_PENDING;

        /* First, we check if this job has a pre-assigned host */
        if (str_empty_c(hostname) == FALSE)
            host = host_by_name(hosts, hostname);
        else
            host = host_match_job(hosts, sjob->job);

        if (host != NULL)
            task->runner = i_add_runner(global, sched->runners, host, hosts, drive, tests, wpaths, flowid, report, repo_vers);

    arrst_end()

    if (ok == TRUE)
    {
        if (arrst_size(sched->runners, Runner) == 0)
        {
            log_printf("%s No work can be started, as there is no host capable of doing so", kASCII_FAIL);
            ok = FALSE;
        }
    }

    if (ok == TRUE)
        ok = i_prepare_before_runners(&drive->login, sched->tasks, tests, wpaths);

    if (ok == TRUE)
    {
        arrst_foreach(runner, sched->runners, Runner)
            cassert(runner->thread == NULL);
            cassert(runner->sched == NULL);
            runner->thread_id = runner_i;
            runner->sched = sched;
            runner->thread = bthread_create(i_run_runner_thread, runner, Runner);
        arrst_end()

        /* Wait all threads for finish */
        arrst_foreach(runner, sched->runners, Runner)
            bthread_wait(runner->thread);
        arrst_end()
    }

    i_destroy_scheduler(&sched);
}

/*---------------------------------------------------------------------------*/
