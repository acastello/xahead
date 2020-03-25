#define _GNU_SOURCE
#include <glib.h>
#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>
#include <errno.h>

#include "xahead.h"

#define DEFAULT_GROUP "default"
#define WINE_GROUP "wine"

gchar *key_file_name = "/home/alex/.config/xahead";

void configure_key_file_name(char *filename)
{
    if (filename) {
        key_file_name = filename;
    }
    // TODO: make envvar-set, multidir-searching configuration
}

int load_index(char *progname, int is_wine)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GKeyFile) key_file = g_key_file_new();

    gboolean success = g_key_file_load_from_file(
            key_file,
            key_file_name,
            G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
            &error
            );

    if (!success) {
        if (error->code == G_FILE_ERROR_NOENT) {
            g_info("%s\n", error->message);
            return CONF_NO_INDEX;
        }
        if (error->domain == G_KEY_FILE_ERROR) {
            g_info("%s\n", error->message);
            return CONF_NO_INDEX;
        }
        g_warning("unhandled error: %s\n", error->message);
        return CONF_NO_INDEX;
    }

    return g_key_file_get_integer(
            key_file,
            is_wine ? WINE_GROUP : DEFAULT_GROUP,
            basename(progname),
            &error
            );
}

#define WARN g_warning("%s:%d %s: %s\n", __FILE__, __LINE__, __func__, strerror(errno)); return;
void save_index(char *progname, int is_wine, int index)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GKeyFile) key_file = g_key_file_new();

    int fd;
    if (0 >= (fd = open(key_file_name, O_CREAT | O_RDONLY, S_IRUSR))) {
        WARN;
    }

    if (flock(fd, LOCK_SH)) {
        WARN;
    }

    if (
            !g_key_file_load_from_file(
                key_file,
                key_file_name,
                G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                &error
                )
       ) {
        g_warning("failed to read key file \"%s\": %s", key_file_name, error->message);
        return;
    }
    g_key_file_set_integer(
            key_file,
            is_wine ? WINE_GROUP : DEFAULT_GROUP,
            progname,
            index
            );
    if (
            !g_key_file_save_to_file(
                key_file,
                key_file_name,
                &error
                )
       ) {
        g_warning("failed to write key file \"%s\": %s", key_file_name, error->message);
    }

    if (flock(fd, LOCK_UN)) {
        WARN;
    }

    if (close(fd)) {
        WARN;
    }
}

