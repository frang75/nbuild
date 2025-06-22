/*
 * NBuild CMake-based C/C++ Continuous Integration System
 * 2015-2025 Francisco Garcia Collado
 * MIT Licence
 * https://nappgui.com/en/legal/license.html
 *
 * File: ssh.h
 *
 */

/* SSH Commands */

#include "nlib.hxx"

uint32_t ssh_command(const char_t *cmd, Stream **stdout_, Stream **stderr_);

bool_t ssh_ping(const char_t *ip);

uint32_t ssh_repo_version(const char_t *repo_url, const char_t *user, const char_t *pass);

uint32_t ssh_working_version(const Login *login, const char_t *path);

String *ssh_working_version2(const char_t *path, const char_t *type);

Stream *ssh_repo_list(const char_t *repo_url, const uint32_t repo_vers, const char_t *user, const char_t *pass);

Stream *ssh_repo_cat(const char_t *repo_url, const uint32_t repo_vers, const char_t *user, const char_t *pass);

bool_t ssh_repo_is_dir(const char_t *repo_url, const uint32_t repo_vers, const char_t *user, const char_t *pass);

bool_t ssh_repo_checkout(const Login *login, const char_t *repo_url, const char_t *user, const char_t *pass, const uint32_t repo_vers, const char_t *dest);

bool_t ssh_dir_exists(const Login *login, const char_t *path);

bool_t ssh_file_exists(const Login *login, const char_t *path, const char_t *filename);

bool_t ssh_create_dir(const Login *login, const char_t *dir);

bool_t ssh_create_file(const Login *login, const char_t *path, const char_t *filename, const char_t *content);

bool_t ssh_delete_file(const Login *login, const char_t *path);

bool_t ssh_delete_dir(const Login *login, const char_t *path);

Stream *ssh_file_cat(const Login *login, const char_t *path, const char_t *filename);

bool_t ssh_to_file(const Login *login, const char_t *path, const char_t *filename, const Stream *stm);

bool_t ssh_copy(const Login *from_login, const char_t *from_path, const char_t *from_filename, const Login *to_login, const char_t *to_path, const char_t *to_filename);

bool_t ssh_copy_files(const Login *login, const char_t *from_path, const char_t *to_path);

bool_t ssh_copy_dir(const Login *from_login, const char_t *from_path, const Login *to_login, const char_t *to_path);

bool_t ssh_scp(const Login *from_login, const char_t *from_path, const Login *to_login, const char_t *to_path, const bool_t recursive);

bool_t ssh_upload(const char_t *from_path, const Login *to_login, const char_t *to_path, const bool_t recursive);

bool_t ssh_vbox_check(const Login *login);

bool_t ssh_vbox_start(const Login *login, const char_t *vbox_uuid);

bool_t ssh_vmware_start(const Login *login, const char_t *vmware_path);

Stream *ssh_diskutil_list(const Login *login);

bool_t ssh_mount(const Login *login, const char_t *device_path, const char_t *volume_path);

bool_t ssh_bless(const Login *login, const char_t *volume_path);

bool_t ssh_reboot(const Login *login);

bool_t ssh_shutdown(const Login *login);

bool_t ssh_launchd_load(const Login *login, const char_t *script_path);

bool_t ssh_launchd_unload(const Login *login, const char_t *script_path);

bool_t ssh_cmake_tar(const Login *login, const char_t *src_path, const char_t *tarpath);

bool_t ssh_cmake_untar(const Login *login, const char_t *dest_path, const char_t *tarpath);

Stream *ssh_cmake_version(const Login *login);

uint32_t ssh_cmake_configure(const Login *login, const char_t *envvars, const char_t *src_path, const char_t *build_path, const char_t *generator, const char_t *opts, String **log);

uint32_t ssh_cmake_build(const Login *login, const char_t *envvars, const char_t *build_path, const char_t *opts, String **log);

uint32_t ssh_cmake_install(const Login *login, const char_t *build_path, const char_t *opts, String **log);

uint32_t ssh_cmake_install_make_program(const Login *login, const char_t *build_path, const char_t *install_cmd, String **log);

uint32_t ssh_execute_test(const Login *login, const char_t *test_cmd, String **log);
