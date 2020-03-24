#define _GNU_SOURCE
#include <glib.h>
#include <stdio.h>
#include <sys/file.h>
#include <unistd.h>

#include "xahead.h"

#define DEFAULT_GROUP "default"
#define WINE_GROUP "wine"

gchar *key_file_name = "/home/alex/.config/xahead";

void configure_key_file_name(char *filename)
{
    key_file_name = filename;
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
            g_clear_error(&error);
            // TODO: use CONF_CREATE
            return CONF_NO_EXEC;
        }
        if (error->domain == G_KEY_FILE_ERROR) {
            // TODO: warn
            g_clear_error(&error);
            return CONF_NO_EXEC;
        }
        return CONF_NO_EXEC;
    }

    return g_key_file_get_integer(
            key_file,
            is_wine ? WINE_GROUP : DEFAULT_GROUP,
            basename(progname),
            &error
            );
}

void save_index(char *progname, int is_wine, int index)
{
    g_autoptr(GError) error = NULL;
    g_autoptr(GKeyFile) key_file = g_key_file_new();

    int fd;
    if (!(fd = open(key_file_name, 0))) {
        perror("save_index");
        return;
    }

    if (!flock(fd, LOCK_SH)) {
        perror("save_index");
        return;
    }

    if (
            !g_key_file_load_from_file(
                key_file,
                key_file_name,
                G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS,
                &error
                )
       ) {
        fprintf(stderr, "Cannot read key file \"%s\": %s\n", key_file_name, error->message);
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
        fprintf(stderr, "Cannot write key file \"%s\": %s\n", key_file_name, error->message);
    }

    if (!flock(fd, LOCK_UN)) {
        perror("save_index");
        return;
    }

    if (!close(fd)) {
        perror("save_index");
    }
}

