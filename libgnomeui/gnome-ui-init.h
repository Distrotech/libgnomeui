#ifndef GNOME_INIT_H
#define GNOME_INIT_H

BEGIN_GNOME_DECLS

error_t gnome_init             	   (char *app_id, struct argp *app_parser,
			       	    int argc, char **argv,
			       	    unsigned int flags, int *arg_index);
error_t gnome_init_with_data   	   (char *app_id, struct argp *app_parser,
			       	    int argc, char **argv,
			       	    unsigned int flags, int *arg_index,
			       	    void *user_data);
void    gnomeui_register_arguments (void);

END_GNOME_DECLS

#endif
