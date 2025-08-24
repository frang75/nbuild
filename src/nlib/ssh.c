/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: ssh.c
 *
 */

/* SSH Commands */

#include "ssh.h"
#include <core/hfile.h>
#include <core/stream.h>
#include <core/strings.h>
#include <osbs/bproc.h>
#include <osbs/bthread.h>
#include <osbs/bsocket.h>
#include <osbs/log.h>
#include <osbs/osbs.h>
#include <sewer/cassert.h>
#include <sewer/bstd.h>
#include <sewer/ptr.h>

typedef struct _proc_std_t ProcStd;

struct _proc_std_t
{
    Proc *proc;
    Stream *stm;
};

#define READ_BUFFER_SIZE 1024

/*
SSH Return Codes
================
0  Operation was successful
1  Generic error, usually because invalid command line options or malformed configuration
2  Connection failed
65 Host not allowed to connect
66 General error in ssh protocol
67 Key exchange failed
68 Reserved
69 MAC error
70 Compression error
71 Service not available
72 Protocol version not supported
73 Host key not verifiable
74 Connection failed
75 Disconnected by application
76 Too many connections
77 Authentication cancelled by user
78 No more authentication methods available
79 Invalid user name

SCP Return Codes
================
0  Operation was successful
1  General error in file copy
2  Destination is not directory, but it should be
3  Maximum symlink level exceeded
4  Connecting to host failed.
5  Connection broken
6  File does not exist
7  No permission to access file.
8  General error in sftp protocol
9  File transfer protocol mismatch
10 No file matches a given criteria
65 Host not allowed to connect
66 General error in ssh protocol
67 Key exchange failed
68 Reserved
69 MAC error
70 Compression error
71 Service not available
72 Protocol version not supported
73 Host key not verifiable
74 Connection failed
75 Disconnected by application
76 Too many connections
77 Authentication cancelled by user
78 No more authentication methods available
79 Invalid user name
*/

/*---------------------------------------------------------------------------*/

static ___INLINE bool_t i_localhost(const Login *login)
{
    if (login == NULL)
        return TRUE;
    return login->localhost;
}

/*---------------------------------------------------------------------------*/

static uint32_t i_std_err(ProcStd *data)
{
    byte_t buffer[READ_BUFFER_SIZE + 1];
    uint32_t rsize;
    cassert_no_null(data);

    while (bproc_eread(data->proc, buffer, READ_BUFFER_SIZE, &rsize, NULL) == TRUE)
        stm_write(data->stm, buffer, rsize);

    return 0;
}

/*---------------------------------------------------------------------------*/

uint32_t ssh_command(const char_t *cmd, Stream **stdout_, Stream **stderr_)
{
    Proc *proc = bproc_exec(cmd, NULL);
    uint32_t return_value = UINT32_MAX;

    if (proc != NULL)
    {
        ProcStd proc_err = {NULL, NULL};
        Thread *thread_err = NULL;
        byte_t buffer[READ_BUFFER_SIZE + 1];
        uint32_t rsize, ret;

        if (stdout_ != NULL)
            *stdout_ = stm_memory(READ_BUFFER_SIZE);

        if (stderr_ != NULL)
        {
            *stderr_ = stm_memory(READ_BUFFER_SIZE);
            proc_err.proc = proc;
            proc_err.stm = *stderr_;
            thread_err = bthread_create(i_std_err, &proc_err, ProcStd);
        }

        while (bproc_read(proc, buffer, READ_BUFFER_SIZE, &rsize, NULL) == TRUE)
        {
            if (stdout_ != NULL)
                stm_write(*stdout_, buffer, rsize);
        }

        if (stderr_ != NULL)
        {
            bproc_eread_close(proc);
        }

        ret = bproc_wait(proc);

        if (thread_err != NULL)
        {
            bthread_wait(thread_err);
            bthread_close(&thread_err);
        }

        bproc_close(&proc);
        return_value = ret;
    }

    return return_value;
}

/*---------------------------------------------------------------------------*/

static String *i_ssh_compose(const Login *login, const char_t *cmd)
{
    String *ssh = NULL;

    if (!i_localhost(login))
    {
        switch (osbs_platform())
        {
        case ekWINDOWS:
            ssh = str_printf("ssh %s@%s \"%s\"", tc(login->user), tc(login->ip), cmd);
            break;

        case ekLINUX:
            cassert_no_null(login);
            if (login->use_sshpass == TRUE)
                ssh = str_printf("sshpass -p '%s' ssh %s@%s '%s'", tc(login->pass), tc(login->user), tc(login->ip), cmd);
            else
                /* Uses ssh certificates */
                ssh = str_printf("ssh %s@%s '%s'", tc(login->user), tc(login->ip), cmd);
            break;

        case ekMACOS:
        case ekIOS:
            cassert_default();
        }
    }
    else
    {
        ssh = str_c(cmd);
    }

    return ssh;
}

/*---------------------------------------------------------------------------*/

static Stream *i_ssh_command(const Login *login, const char_t *cmd, const bool_t capture_stderr, uint32_t *return_value)
{
    Stream *stdout_ = NULL;
    Stream *stderr_ = NULL;
    uint32_t ret = UINT32_MAX;

    {
        String *ssh = i_ssh_compose(login, cmd);
        ret = ssh_command(tc(ssh), &stdout_, (capture_stderr == TRUE) ? &stderr_ : NULL);
        str_destroy(&ssh);
    }

    if (capture_stderr == TRUE)
    {
        if (stm_buffer_size(stderr_) > 10)
        {
            stm_writef(stdout_, "\nstderr:\n");
            stm_pipe(stderr_, stdout_, stm_buffer_size(stderr_));
        }

        stm_close(&stderr_);
    }

    ptr_assign(return_value, ret);
    return stdout_;
}

/*---------------------------------------------------------------------------*/

static bool_t i_ssh_ok(const Login *login, String **cmd)
{
    uint32_t ret = UINT32_MAX;
    Stream *stm = i_ssh_command(login, tc(*cmd), FALSE, &ret);
    str_destroy(cmd);
    if (stm != NULL)
        stm_close(&stm);

    if (ret == 0)
        return TRUE;
    else
        return FALSE;
}

/*---------------------------------------------------------------------------*/

static bool_t i_ssh_ret(const Login *login, String **cmd, const uint32_t expected_ret)
{
    uint32_t ret = UINT32_MAX;
    Stream *stm = i_ssh_command(login, tc(*cmd), FALSE, &ret);
    str_destroy(cmd);
    if (stm != NULL)
        stm_close(&stm);

    if (ret == expected_ret)
        return TRUE;
    else
        return FALSE;
}

/*---------------------------------------------------------------------------*/

bool_t ssh_ping(const char_t *ip)
{
    bool_t ok = FALSE;
    char_t cmd[128];
    Proc *proc = NULL;

    if (osbs_platform() == ekWINDOWS)
        bstd_sprintf(cmd, sizeof(cmd), "ping %s -n 1", ip);
    else
        bstd_sprintf(cmd, sizeof(cmd), "ping %s -c 1", ip);

    proc = bproc_exec(cmd, NULL);
    if (proc != NULL)
    {
        uint32_t ret = bproc_wait(proc);
        if (ret == 0)
            ok = TRUE;
        bproc_close(&proc);
    }

    return ok;
}

/*---------------------------------------------------------------------------*/

static uint32_t i_repo_version(const char_t *cmd)
{
    Stream *stm = i_ssh_command(NULL, cmd, FALSE, NULL);
    uint32_t ret = UINT32_MAX;

    if (stm != NULL)
    {
        const char_t *line = stm_read_line(stm);
        if (line)
        {
            ret = str_to_u32(line, 10, NULL);
            if (ret == 0)
                ret = UINT32_MAX;
        }
        stm_close(&stm);
    }

    return ret;
}

/*---------------------------------------------------------------------------*/

static String *i_repo_version2(const Login *login, const char_t *cmd)
{
    Stream *stm = i_ssh_command(login, cmd, FALSE, NULL);
    String *ret = NULL;

    if (stm != NULL)
    {
        const char_t *line = stm_read_line(stm);
        if (line)
            ret = str_c(line);
        stm_close(&stm);
    }

    return ret;
}

/*---------------------------------------------------------------------------*/

static uint32_t i_repo_version_4(const Login *login, const char_t *cmd)
{
    Stream *stm = i_ssh_command(login, cmd, FALSE, NULL);
    uint32_t ret = UINT32_MAX;

    if (stm != NULL)
    {
        const char_t *line = stm_read_line(stm);
        if (line)
        {
            String *number = NULL;
            if (str_str(line, ":") != NULL)
                str_split(line, ":", NULL, &number);
            else
                number = str_c(line);

            str_subs(number, 'M', '\0');
            str_subs(number, 'S', '\0');
            str_subs(number, 'P', '\0');

            ret = str_to_u32(tc(number), 10, NULL);
            str_destroy(&number);
        }

        stm_close(&stm);
    }

    return ret;
}

/*---------------------------------------------------------------------------*/

uint32_t ssh_repo_version(const char_t *repo_url, const char_t *user, const char_t *pass)
{
    String *cmd = str_printf("svn info --show-item last-changed-revision --non-interactive --no-auth-cache --username %s --password %s %s -r HEAD", user, pass, repo_url);
    uint32_t vers = i_repo_version(tc(cmd));
    str_destroy(&cmd);
    return vers;
}

/*---------------------------------------------------------------------------*/

uint32_t ssh_working_version(const Login *login, const char_t *path)
{
    String *cmd = str_printf("svnversion %s", path);
    uint32_t vers = i_repo_version_4(login, tc(cmd));
    str_destroy(&cmd);
    return vers;
}

/*---------------------------------------------------------------------------*/

String *ssh_working_version2(const char_t *path, const char_t *type)
{
    String *version = NULL;
    if (str_equ_c(type, "svn"))
    {
        String *cmd = str_printf("svn info --show-item revision %s", path);
        version = i_repo_version2(NULL, tc(cmd));
        str_destroy(&cmd);
    }
    else if (str_equ_c(type, "git"))
    {
        String *cmd = str_printf("git -C \"%s\" rev-parse --short HEAD", path);
        version = i_repo_version2(NULL, tc(cmd));
        str_destroy(&cmd);
    }
    else
    {
        version = str_c(type);
    }

    if (version == NULL)
        version = str_c("");

    return version;
}

/*---------------------------------------------------------------------------*/

Stream *ssh_repo_list(const char_t *repo_url, const uint32_t repo_vers, const char_t *user, const char_t *pass)
{
    String *cmd = str_printf("svn list --non-interactive --no-auth-cache --username %s --password %s %s -r %d", user, pass, repo_url, repo_vers);
    Stream *stm = i_ssh_command(NULL, tc(cmd), FALSE, NULL);
    str_destroy(&cmd);
    return stm;
}

/*---------------------------------------------------------------------------*/

Stream *ssh_repo_cat(const char_t *repo_url, const uint32_t repo_vers, const char_t *user, const char_t *pass)
{
    String *cmd = str_printf("svn cat --non-interactive --no-auth-cache --username %s --password %s %s -r %d", user, pass, repo_url, repo_vers);
    Stream *stm = i_ssh_command(NULL, tc(cmd), FALSE, NULL);
    str_destroy(&cmd);
    return stm;
}

/*---------------------------------------------------------------------------*/

static bool_t i_repo_node_kind(const char_t *repo_url, const uint32_t repo_vers, const char_t *user, const char_t *pass, const char_t *kind)
{
    bool_t ret = FALSE;
    String *cmd = str_printf("svn info --non-interactive --no-auth-cache --username %s --password %s %s -r %d", user, pass, repo_url, repo_vers);
    Stream *stm = i_ssh_command(NULL, tc(cmd), FALSE, NULL);
    str_destroy(&cmd);

    stm_lines(line, stm)
        String *left, *right;
        bool_t stop = FALSE;
        if (str_split_trim(line, ":", &left, &right) == TRUE)
        {
            if (str_equ_nocase(tc(left), "node kind") == TRUE)
            {
                stop = TRUE;
                if (str_equ_nocase(tc(right), kind) == TRUE)
                    ret = TRUE;
            }
        }
        str_destopt(&left);
        str_destopt(&right);

        if (stop == TRUE)
            break;

    stm_next(line, stm)
    stm_close(&stm);
    return ret;
}

/*---------------------------------------------------------------------------*/

bool_t ssh_repo_is_dir(const char_t *repo_url, const uint32_t repo_vers, const char_t *user, const char_t *pass)
{
    return i_repo_node_kind(repo_url, repo_vers, user, pass, "directory");
}

/*---------------------------------------------------------------------------*/

bool_t ssh_repo_checkout(const Login *login, const char_t *repo_url, const char_t *user, const char_t *pass, const uint32_t repo_vers, const char_t *dest)
{
    /* svn checkout svn://192.168.1.2/svn/NAPPGUI/1_0 C:/wctest -r 1000 */
    String *cmd = str_printf("svn checkout --username %s --password %s --non-interactive --no-auth-cache %s %s -r %d", user, pass, repo_url, dest, repo_vers);
    return i_ssh_ok(login, &cmd);
}

/*---------------------------------------------------------------------------*/

static bool_t i_exists(const Login *login, const char_t *file, const file_type_t type)
{
    bool_t exists = FALSE;
    String *cmd = NULL;
    Stream *stm = NULL;
    cassert_no_null(login);

    if (login->platform == ekMACOS || login->platform == ekLINUX)
    {
        cmd = str_printf("[ ! %s %s ] && echo NOT_EXISTS", type == ekARCHIVE ? "-f" : "-d", file);
    }
    else
    {
        cmd = str_printf("IF EXIST %s (echo Yes) ELSE (echo NOT_EXISTS)", file);
        str_subs(cmd, '/', '\\');
    }

    stm = i_ssh_command(login, tc(cmd), FALSE, NULL);
    if (stm != NULL)
    {
        const char_t *line = stm_read_line(stm);
        if (line == NULL || str_str(line, "NOT_EXISTS") == NULL)
            exists = TRUE;
        stm_close(&stm);
    }

    str_destroy(&cmd);
    return exists;
}

/*---------------------------------------------------------------------------*/

bool_t ssh_dir_exists(const Login *login, const char_t *path)
{
    return i_exists(login, path, ekDIRECTORY);
}

/*---------------------------------------------------------------------------*/

bool_t ssh_file_exists(const Login *login, const char_t *path, const char_t *filename)
{
    String *f = NULL;
    bool_t ok = FALSE;
    cassert_no_null(login);
    f = str_path(login->platform, "%s/%s", path, filename);
    ok = i_exists(login, tc(f), ekARCHIVE);
    str_destroy(&f);
    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_win_mkdir(const Login *login, char_t *path)
{
    bool_t ok = TRUE;
    if (ssh_dir_exists(login, path) == FALSE)
    {
        uint32_t size = str_len_c(path);
        uint32_t i = size;
        uint32_t dirs = 0;
        char_t sep[64];
        while (i > 0)
        {
            if (path[i] == '\\' || path[i] == '/')
            {
                sep[dirs] = path[i];
                path[i] = '\0';
                dirs += 1;
                if (ssh_dir_exists(login, path) == TRUE)
                    break;
            }

            i -= 1;
        }

        while (i < size)
        {
            if (path[i] == '\0')
            {
                cassert(dirs > 0);
                dirs -= 1;
                path[i] = sep[dirs];

                if (ok == TRUE)
                {
                    String *cmd = NULL;
                    Stream *stm = NULL;
                    cmd = str_printf("mkdir %s", path);
                    stm = i_ssh_command(login, tc(cmd), TRUE, NULL);
                    if (stm != NULL)
                    {
                        ptr_destopt(stm_close, &stm, Stream);
                    }
                    else
                    {
                        ok = FALSE;
                    }
                    str_destroy(&cmd);
                }
            }

            i += 1;
        }

        cassert(dirs == 0);
    }

    return ok;
}

/*---------------------------------------------------------------------------*/

bool_t ssh_create_dir(const Login *login, const char_t *dir)
{
    if (login->platform == ekWINDOWS)
    {
        return i_win_mkdir(login, cast(dir, char_t));
    }
    else
    {
        String *cmd = str_printf("mkdir -p %s", dir);
        return i_ssh_ok(login, &cmd);
    }
}

/*---------------------------------------------------------------------------*/

bool_t ssh_create_file(const Login *login, const char_t *path, const char_t *filename, const char_t *content)
{
    String *cmd = NULL;
    if (login->platform == ekWINDOWS)
        cmd = str_printf("echo \"%s\" > %s\\%s", content, path, filename);
    else
        cmd = str_printf("echo \"%s\" > %s/%s", content, path, filename);
    return i_ssh_ok(login, &cmd);
}

/*---------------------------------------------------------------------------*/

bool_t ssh_delete_file(const Login *login, const char_t *path)
{
    String *cmd = NULL;
    cassert_no_null(login);
    if (login->platform == ekWINDOWS)
        cmd = str_printf("del %s", path);
    else
        cmd = str_printf("rm %s", path);
    return i_ssh_ok(login, &cmd);
}

/*---------------------------------------------------------------------------*/

bool_t ssh_delete_dir(const Login *login, const char_t *path)
{
    String *cmd = NULL;
    cassert_no_null(login);
    if (login->platform == ekWINDOWS)
        cmd = str_printf("rd /s /q %s", path);
    else
        cmd = str_printf("rm -rf %s", path);
    return i_ssh_ok(login, &cmd);
}

/*---------------------------------------------------------------------------*/

Stream *ssh_file_cat(const Login *login, const char_t *path, const char_t *filename)
{
    String *cmd = NULL;
    Stream *stm = NULL;
    cassert_no_null(login);

    if (login->platform == ekMACOS || login->platform == ekLINUX)
        cmd = str_path(login->platform, "cat %s/%s", path, filename);
    else
        cmd = str_path(login->platform, "type %s/%s", path, filename);

    stm = i_ssh_command(login, tc(cmd), FALSE, NULL);
    str_destroy(&cmd);
    return stm;
}

/*---------------------------------------------------------------------------*/

static ___INLINE String *i_join_file(const Login *login, const char_t *path, const char_t *filename)
{
    if (i_localhost(login) == TRUE)
        return str_cpath("%s/%s", path, filename);
    else
        return str_path(login->platform, "%s/%s", path, filename);
}

/*---------------------------------------------------------------------------*/

bool_t ssh_to_file(const Login *login, const char_t *path, const char_t *filename, const Stream *stm)
{
    const byte_t *data = stm_buffer(stm);
    uint32_t size = stm_buffer_size(stm);
    String *dest = i_join_file(login, path, filename);
    bool_t ok = FALSE;

    if (i_localhost(login))
    {
        Stream *file = stm_to_file(tc(dest), NULL);
        if (file != NULL)
        {
            stm_write(file, data, size);
            stm_close(&file);
            ok = TRUE;
        }
        else
        {
            ok = FALSE;
        }
    }
    else
    {
        String *tmp = hfile_appdata("temp_file");
        Stream *file = stm_to_file(tc(tmp), NULL);
        stm_write(file, data, size);
        stm_close(&file);
        ok = ssh_scp(NULL, tc(tmp), login, tc(dest), FALSE);
        str_destroy(&tmp);
    }

    str_destroy(&dest);
    return ok;
}

/*---------------------------------------------------------------------------*/

static bool_t i_login_equal(const Login *login1, const Login *login2)
{
    if (i_localhost(login1) == TRUE && i_localhost(login2) == TRUE)
        return TRUE;
    if (login1 == NULL || login2 == NULL)
        return FALSE;
    if (str_equ(login1->ip, tc(login2->ip)) == TRUE)
        return TRUE;
    return FALSE;
}

/*---------------------------------------------------------------------------*/

bool_t ssh_copy(const Login *from_login, const char_t *from_path, const char_t *from_filename, const Login *to_login, const char_t *to_path, const char_t *to_filename)
{
    String *from = i_join_file(from_login, from_path, from_filename);
    String *to = i_join_file(to_login, to_path, to_filename);
    bool_t ok = FALSE;

    /* If 'from' and 'to' are in same node, don't use scp */
    if (i_login_equal(from_login, to_login) == TRUE)
        ok = ssh_copy_files(from_login, tc(from), tc(to));
    else
        ok = ssh_scp(from_login, tc(from), to_login, tc(to), FALSE);

    str_destroy(&from);
    str_destroy(&to);
    return ok;
}

/*---------------------------------------------------------------------------*/

static platform_t i_login_platform(const Login *login)
{
    if (login != NULL)
        return login->platform;
    return osbs_platform();
}

/*---------------------------------------------------------------------------*/

bool_t ssh_copy_files(const Login *login, const char_t *from_path, const char_t *to_path)
{
    platform_t platform = i_login_platform(login);
    String *cmd = NULL;
    if (platform == ekWINDOWS)
        cmd = str_printf("copy %s %s", from_path, to_path);
    else
        cmd = str_printf("cp %s %s", from_path, to_path);
    return i_ssh_ok(login, &cmd);
}

/*---------------------------------------------------------------------------*/

bool_t ssh_copy_dir(const Login *from_login, const char_t *from_path, const Login *to_login, const char_t *to_path)
{
    bool_t ok = FALSE;
    String *from = i_join_file(from_login, from_path, "*");
    ok = ssh_scp(from_login, tc(from), to_login, to_path, TRUE);
    str_destroy(&from);
    return ok;
}

/*---------------------------------------------------------------------------*/

static ___INLINE String *i_scp_op(const Login *login, const char_t *path)
{
    cassert_no_null(path);
    /* SCP needs the path with Unix separator '/' independent the src or dest machine */
    if (i_localhost(login) == TRUE)
        return str_path(ekLINUX, "%s", path);
    else
        return str_path(ekLINUX, "%s@%s:%s", tc(login->user), tc(login->ip), path);
}

/*---------------------------------------------------------------------------*/

bool_t ssh_scp(const Login *from_login, const char_t *from_path, const Login *to_login, const char_t *to_path, const bool_t recursive)
{
    bool_t ok = FALSE;
    String *from = i_scp_op(from_login, from_path);
    String *to = i_scp_op(to_login, to_path);
    const char_t *scp = (recursive == TRUE) ? "scp -r" : "scp";
    String *cmd = str_printf("%s %s %s", scp, tc(from), tc(to));
    Proc *proc = bproc_exec(tc(cmd), NULL);

    if (proc != NULL)
    {
        uint32_t ret = UINT32_MAX;
        bproc_read_close(proc);
        bproc_eread_close(proc);
        ret = bproc_wait(proc);
        bproc_close(&proc);
        if (ret == 0)
            ok = TRUE;
    }

    str_destroy(&from);
    str_destroy(&to);
    str_destroy(&cmd);
    return ok;
}

/*---------------------------------------------------------------------------*/

bool_t ssh_upload(const char_t *from_path, const Login *to_login, const char_t *to_path, const bool_t recursive)
{
    bool_t ok = FALSE;
    String *from = i_scp_op(NULL, from_path);
    String *to = i_scp_op(to_login, to_path);
    const char_t *scp = (recursive == TRUE) ? "scp -r" : "scp";
    String *cmd = NULL;
    Proc *proc = NULL;

    cassert_no_null(to_login);
    if (to_login->use_sshpass == TRUE)
        cmd = str_printf("sshpass -p '%s' %s %s %s", tc(to_login->pass), scp, tc(from), tc(to));
    else
        cmd = str_printf("%s %s %s", scp, tc(from), tc(to));

    proc = bproc_exec(tc(cmd), NULL);

    if (proc != NULL)
    {
        uint32_t ret = UINT32_MAX;
        bproc_read_close(proc);
        bproc_eread_close(proc);
        ret = bproc_wait(proc);
        bproc_close(&proc);
        if (ret == 0)
            ok = TRUE;
    }

    str_destroy(&from);
    str_destroy(&to);
    str_destroy(&cmd);
    return ok;
}

/*---------------------------------------------------------------------------*/

bool_t ssh_vbox_check(const Login *login)
{
    String *cmd = str_c("vboxmanage -version");
    return i_ssh_ok(login, &cmd);
}

/*---------------------------------------------------------------------------*/

bool_t ssh_vbox_start(const Login *login, const char_t *vbox_uuid)
{
    uint32_t ret = 1;
    String *cmd = str_printf("vboxmanage startvm %s --type headless", vbox_uuid);
    Stream *stm = i_ssh_command(login, tc(cmd), FALSE, &ret);
    stm_close(&stm);
    str_destroy(&cmd);
    return (bool_t)(ret == 0);
}

/*---------------------------------------------------------------------------*/

bool_t ssh_vmware_start(const Login *login, const char_t *vmware_path)
{
    uint32_t ret = 1;
    String *cmd = str_printf("vmrun start \"%s\"", vmware_path);
    Stream *stm = i_ssh_command(login, tc(cmd), FALSE, &ret);
    stm_close(&stm);
    str_destroy(&cmd);
    return (bool_t)(ret == 0);
}

/*---------------------------------------------------------------------------*/

Stream *ssh_diskutil_list(const Login *login)
{
    cassert_no_null(login);
    if (login->platform == ekMACOS)
    {
        String *cmd = str_printf("/usr/sbin/diskutil list");
        Stream *stm = i_ssh_command(login, tc(cmd), FALSE, NULL);
        str_destroy(&cmd);
        return stm;
    }

    return NULL;
}

/*---------------------------------------------------------------------------*/

bool_t ssh_mount(const Login *login, const char_t *device_path, const char_t *volume_path)
{
    cassert_no_null(login);
    unref(volume_path);
    if (login->platform == ekMACOS)
    {
        String *cmd = str_printf("echo %s | sudo -S /usr/sbin/diskutil mountDisk %s", tc(login->pass), device_path);
        return i_ssh_ok(login, &cmd);
    }

    return FALSE;
}

/*---------------------------------------------------------------------------*/

bool_t ssh_bless(const Login *login, const char_t *volume_path)
{
    cassert_no_null(login);
    if (login->platform == ekMACOS)
    {
        String *cmd = str_printf("echo %s | sudo -S /usr/sbin/bless --mount %s --setBoot --nextonly", tc(login->pass), volume_path);
        return i_ssh_ok(login, &cmd);
    }

    return FALSE;
}

/*---------------------------------------------------------------------------*/

bool_t ssh_reboot(const Login *login)
{
    cassert_no_null(login);
    if (login->platform == ekMACOS)
    {
        String *cmd = str_printf("echo %s | sudo -S /sbin/reboot", tc(login->pass));
        /* ssh reboot returns '255' instead 0. Perhaps because the connection is closed by remote */
        return i_ssh_ret(login, &cmd, 255);
    }

    return FALSE;
}

/*---------------------------------------------------------------------------*/

bool_t ssh_shutdown(const Login *login)
{
    cassert_no_null(login);
    switch (login->platform)
    {
    case ekLINUX:
    {
        String *cmd = str_printf("echo %s | sudo -S shutdown -h now", tc(login->pass));
        return i_ssh_ok(login, &cmd);
    }

    case ekWINDOWS:
    {
        String *cmd = str_printf("shutdown -s -t 00 -f");
        return i_ssh_ok(login, &cmd);
    }

    case ekMACOS:
    {
        String *cmd = str_printf("echo %s | sudo -S shutdown -h now", tc(login->pass));
        return i_ssh_ok(login, &cmd);
    }

        cassert_default();
    }

    return FALSE;
}

/*---------------------------------------------------------------------------*/

bool_t ssh_launchd_load(const Login *login, const char_t *script_path)
{
    String *cmd = NULL;
    cassert_no_null(login);
    cassert(login->platform == ekMACOS);
    cmd = str_printf("launchctl load %s", script_path);
    return i_ssh_ok(login, &cmd);
}

/*---------------------------------------------------------------------------*/

bool_t ssh_launchd_unload(const Login *login, const char_t *script_path)
{
    String *cmd = NULL;
    cassert_no_null(login);
    cassert(login->platform == ekMACOS);
    cmd = str_printf("launchctl unload %s", script_path);
    return i_ssh_ok(login, &cmd);
}

/*---------------------------------------------------------------------------*/

bool_t ssh_cmake_tar(const Login *login, const char_t *src_path, const char_t *tarpath)
{
    platform_t platform = i_login_platform(login);
    String *cmake = NULL;
    String *cmd = NULL;
    cmake = str_printf("cmake -E tar cvzf %s .", tarpath);
    if (platform == ekWINDOWS)
        cmd = str_printf("cd %s & %s", src_path, tc(cmake));
    else
        cmd = str_printf("cd %s ; %s", src_path, tc(cmake));

    str_destroy(&cmake);
    return i_ssh_ok(login, &cmd);
}

/*---------------------------------------------------------------------------*/

bool_t ssh_cmake_untar(const Login *login, const char_t *dest_path, const char_t *tarpath)
{
    platform_t platform = i_login_platform(login);
    String *cmake = NULL;
    String *cmd = NULL;
    cmake = str_printf("cmake -E tar xvzf %s", tarpath);
    if (platform == ekWINDOWS)
        cmd = str_printf("cd %s & %s", dest_path, tc(cmake));
    else
        cmd = str_printf("cd %s ; %s", dest_path, tc(cmake));

    str_destroy(&cmake);
    return i_ssh_ok(login, &cmd);
}

/*---------------------------------------------------------------------------*/

static ___INLINE const char_t *i_qt(const Login *login)
{
    if (osbs_platform() == ekWINDOWS && i_localhost(login) == FALSE)
        return "\\\"";
    else
        return "\"";
}

/*---------------------------------------------------------------------------*/

Stream *ssh_cmake_version(const Login *login)
{
    return i_ssh_command(login, "cmake --version", FALSE, NULL);
}

/*---------------------------------------------------------------------------*/

uint32_t ssh_cmake_configure(const Login *login, const char_t *envvars, const char_t *src_path, const char_t *build_path, const char_t *generator, const char_t *opts, String **log)
{
    platform_t platform = i_login_platform(login);
    const char_t *qt = i_qt(login);
    String *cmake = NULL;
    String *cmd = NULL;
    Stream *stm = NULL;
    uint32_t ret = UINT32_MAX;
    cmake = str_printf("cmake -G %s%s%s %s %s", qt, generator, qt, opts, src_path);
    if (str_empty_c(envvars) == FALSE)
    {
        if (platform == ekWINDOWS)
            cmd = str_printf("%s&cd %s&%s", envvars, build_path, tc(cmake));
        else
            cmd = str_printf("%s;cd %s;%s", envvars, build_path, tc(cmake));
    }
    else
    {
        if (platform == ekWINDOWS)
            cmd = str_printf("cd %s&%s", build_path, tc(cmake));
        else
            cmd = str_printf("cd %s;%s", build_path, tc(cmake));
    }

    stm = i_ssh_command(login, tc(cmd), TRUE, &ret);
    if (stm != NULL)
    {
        if (log != NULL)
        {
            Stream *stm_log = stm_memory(2024);
            uint32_t size = stm_buffer_size(stm);
            stm_printf(stm_log, "%s", tc(cmd));
            stm_writef(stm_log, "\n\n");
            stm_pipe(stm, stm_log, size);
            *log = stm_str(stm_log);
            stm_close(&stm_log);
        }
        stm_close(&stm);
    }
    else
    {
        if (log != NULL)
            *log = str_c("");
    }

    str_destroy(&cmake);
    str_destroy(&cmd);
    return ret;
}

/*---------------------------------------------------------------------------*/

uint32_t ssh_cmake_build(const Login *login, const char_t *envvars, const char_t *build_path, const char_t *opts, String **log)
{
    platform_t platform = i_login_platform(login);
    uint32_t ret = UINT32_MAX;
    String *cmd = NULL;
    Stream *stm = NULL;

    if (str_empty_c(envvars) == FALSE)
    {
        if (platform == ekWINDOWS)
            cmd = str_printf("%s&cmake --build %s %s", envvars, build_path, opts);
        else
            cmd = str_printf("%s;cmake --build %s %s", envvars, build_path, opts);
    }
    else
    {
        cmd = str_printf("cmake --build %s %s", build_path, opts);
    }

    stm = i_ssh_command(login, tc(cmd), TRUE, &ret);

    if (stm != NULL)
    {
        if (log != NULL)
        {
            Stream *stm_log = stm_memory(2024);
            uint32_t size = stm_buffer_size(stm);
            stm_printf(stm_log, "%s", tc(cmd));
            stm_writef(stm_log, "\n\n");
            stm_pipe(stm, stm_log, size);
            *log = stm_str(stm_log);
            stm_close(&stm_log);
        }
        stm_close(&stm);
    }
    else
    {
        if (log != NULL)
            *log = str_c("");
    }

    str_destroy(&cmd);
    return ret;
}

/*---------------------------------------------------------------------------*/

uint32_t ssh_cmake_install(const Login *login, const char_t *build_path, const char_t *opts, String **log)
{
    uint32_t ret = UINT32_MAX;
    String *cmd = str_printf("cmake --install %s %s", build_path, opts);
    Stream *stm = i_ssh_command(login, tc(cmd), TRUE, &ret);
    if (stm != NULL)
    {
        if (log != NULL)
        {
            Stream *stm_log = stm_memory(2024);
            uint32_t size = stm_buffer_size(stm);
            stm_printf(stm_log, "%s", tc(cmd));
            stm_writef(stm_log, "\n\n");
            stm_pipe(stm, stm_log, size);
            *log = stm_str(stm_log);
            stm_close(&stm_log);
        }
        stm_close(&stm);
    }
    else
    {
        if (log != NULL)
            *log = str_c("");
    }

    str_destroy(&cmd);
    return ret;
}

/*---------------------------------------------------------------------------*/

uint32_t ssh_cmake_install_make_program(const Login *login, const char_t *build_path, const char_t *install_cmd, String **log)
{
    platform_t platform = i_login_platform(login);
    String *cmd = NULL;
    Stream *stm = NULL;
    uint32_t ret = UINT32_MAX;

    if (platform == ekWINDOWS)
        cmd = str_printf("cd %s&%s", build_path, install_cmd);
    else
        cmd = str_printf("cd %s;%s", build_path, install_cmd);

    stm = i_ssh_command(login, tc(cmd), TRUE, &ret);
    if (stm != NULL)
    {
        if (log != NULL)
        {
            Stream *stm_log = stm_memory(2024);
            uint32_t size = stm_buffer_size(stm);
            stm_printf(stm_log, "%s", tc(cmd));
            stm_writef(stm_log, "\n\n");
            stm_pipe(stm, stm_log, size);
            *log = stm_str(stm_log);
            stm_close(&stm_log);
        }
        stm_close(&stm);
    }
    else
    {
        if (log != NULL)
            *log = str_c("");
    }

    str_destroy(&cmd);
    return ret;
}

/*---------------------------------------------------------------------------*/

uint32_t ssh_execute_cmd(const Login *login, const char_t *test_cmd, String **log)
{
    uint32_t ret = UINT32_MAX;
    Stream *stm = i_ssh_command(login, test_cmd, TRUE, &ret);
    if (stm != NULL)
    {
        if (log != NULL)
        {
            Stream *stm_log = stm_memory(2024);
            uint32_t size = stm_buffer_size(stm);
            stm_printf(stm_log, "%s", test_cmd);
            stm_writef(stm_log, "\n\n");
            stm_pipe(stm, stm_log, size);
            *log = stm_str(stm_log);
            stm_close(&stm_log);
        }
        stm_close(&stm);
    }
    else
    {
        if (log != NULL)
            *log = str_c("");
    }

    return ret;
}
