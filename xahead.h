/** \file */
#define CONF_NO_INDEX 0

/**
 * Configure location of config file
 *
 * \param filename Optionally set a path. NULL causes the default one to be * used
 */
void configure_key_file_name(char *filename);

/**
 * Get 1-based index of first visible window for the given program
 *
 * \param progname Name of the program called, as in `./progname` or `wine progname`
 * \param is_wine Whether or not the program was an argument to wine
 * \return 1-based spawn index of the first visible window. 0 if window is not
 * configured yet
 */
int load_index(char *progname, int is_wine);

/**
 * Save index of first visible window for the given program
 *
 * \param progname Name of the program called, as in `./progname` or `wine progname`
 * \param is_wine Whether or not the program was an argument to wine
 * \param index 1-based spawn index for the first visible window
 */
void save_index(char *progname, int is_wine, int index);

