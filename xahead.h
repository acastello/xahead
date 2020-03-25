#define CONF_NO_INDEX 0

void configure_key_file_name(char *filename);
int load_index(char *progname, int is_wine);
void save_index(char *progname, int is_wine, int index);

