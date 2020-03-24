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

static void setup(index_opts *opts, gconstpointer user_data)
{
    setup_object(opts, user_data);
    g_assert_cmpint(0, <=, creat("testkeyfile", S_IRUSR | S_IWUSR));
}

static void teardown_file(index_opts *fixture, gconstpointer user_data)
{
    g_assert_false(remove("testkeyfile"));
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

static void test_save_invalid_file(index_opts *opts, gconstpointer user_data)
{
    configure_key_file_name("");
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

int main(int argc, char *argv[])
{
    g_test_init(&argc, &argv, NULL);

    g_test_add("/save_index/regular_usage", index_opts, NULL, setup, test_save, teardown_file);
    g_test_add("/save_index/bad_file_name", index_opts, NULL, setup, test_save_invalid_file, teardown_file);

    return g_test_run();
}
