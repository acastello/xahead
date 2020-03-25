#include <fcntl.h>
#include <glib.h>
#include <locale.h>
#include <stdio.h>
#include <sys/stat.h>
#include <unistd.h>

#include "xahead.h"

typedef struct {
    char *progname;
    int is_wine;
    int index;
} index_opts;

static void setup_object(index_opts *opts, gconstpointer user_data)
{
    opts->progname = "testexec";
    opts->is_wine = 1;
    opts->index = 3;
    configure_key_file_name("testkeyfile");
}

static void setup_with_file(index_opts *opts, gconstpointer user_data)
{
    setup_object(opts, user_data);
    g_assert_cmpint(0, <=, creat("testkeyfile", S_IRWXU));
}

static void setup_with_bad_file(index_opts *opts, gconstpointer user_data)
{
    setup_object(opts, user_data);
    int fd;
    g_assert_cmpint(0, <=, (fd = creat("testkeyfile", S_IRUSR | S_IWUSR)));
    g_assert_cmpint(0, <, write(fd, "bad!", 4));
}

static void teardown_file(index_opts *fixture, gconstpointer user_data)
{
    g_assert_cmpint(0, ==, remove("testkeyfile"));
}

static void test_load(index_opts *opts, gconstpointer user_data) {
    save_index(opts->progname, opts->is_wine, opts->index);
    g_assert_cmpint(
            3,
            ==,
            load_index(opts->progname, opts->is_wine)
            );
}

static void test_load_no_file(index_opts *opts, gconstpointer user_data) {
    g_test_expect_message(NULL, G_LOG_LEVEL_INFO, "*No such file*");
    g_assert_cmpint(
            CONF_NO_INDEX,
            ==,
            load_index(opts->progname, opts->is_wine)
            );
}

static void test_load_bad_parse(index_opts *opts,gconstpointer user_data) {
    g_test_expect_message(NULL, G_LOG_LEVEL_INFO, "*contains line*bad*");
    g_assert_cmpint(
            CONF_NO_INDEX,
            ==,
            load_index(opts->progname, opts->is_wine)
            );
}

static void test_save(index_opts *opts, gconstpointer user_data) {
    save_index(opts->progname, opts->is_wine, opts->index);

    g_autoptr(GKeyFile) key_file = g_key_file_new();
    g_assert_true(
            g_key_file_load_from_file(key_file, "testkeyfile", 0, NULL)
            );
    g_assert_cmpint(
            3,
            ==,
            g_key_file_get_integer(key_file, "wine", "testexec", NULL)
            );
    g_key_file_unref(key_file);
}

static void test_save_new_file(index_opts *opts, gconstpointer user_data)
{
    save_index(opts->progname, opts->is_wine, opts->index);
    g_assert_true(g_file_test("testkeyfile", G_FILE_TEST_EXISTS));

    g_autoptr(GKeyFile) key_file = g_key_file_new();
    g_assert_true(
            g_key_file_load_from_file(key_file, "testkeyfile", 0, NULL)
            );
    g_assert_cmpint(
            3,
            ==,
            g_key_file_get_integer(key_file, "wine", "testexec", NULL)
            );
    g_key_file_unref(key_file);
}

static void test_save_bad_parse(index_opts *opts, gconstpointer user_data) {
    g_test_expect_message(NULL, G_LOG_LEVEL_WARNING, "*contains line*bad*");
    save_index(opts->progname, opts->is_wine, opts->index);
}

static void test_save_invalid_file(index_opts *opts, gconstpointer user_data)
{
    configure_key_file_name("");
    g_test_expect_message(NULL, G_LOG_LEVEL_WARNING, "*No such file*");
    save_index(opts->progname, opts->is_wine, opts->index);
}

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    remove("testkeyfile");

    g_test_add("/load_index/regular_usage", index_opts, NULL, setup_with_file, test_load, teardown_file);
    g_test_add("/load_index/no_file", index_opts, NULL, setup_object, test_load_no_file, NULL);
    g_test_add("/load_index/bad_parse", index_opts, NULL, setup_with_bad_file, test_load_bad_parse, teardown_file);
    g_test_add("/save_index/regular_usage", index_opts, NULL, setup_with_file, test_save, teardown_file);
    g_test_add("/save_index/new_file", index_opts, NULL, setup_object, test_save_new_file, teardown_file);
    g_test_add("/save_index/bad_parse", index_opts, NULL, setup_with_bad_file, test_save_bad_parse, teardown_file);
    g_test_add("/save_index/bad_file_name", index_opts, NULL, setup_object, test_save_invalid_file, NULL);

    int res = g_test_run();
    return res;
}
