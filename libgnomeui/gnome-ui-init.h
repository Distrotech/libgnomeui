#ifndef GNOME_INIT_H
#define GNOME_INIT_H

BEGIN_GNOME_DECLS

#include "libgnome/gnome-popt.h"

/* After these functions return, gnome is initialized and you can do
   whatever you like. */
int gnome_init(const char *app_id, const char *app_version,
	       int argc, char **argv);
/* return_ctx can be NULL if you don't want the poptContext to be returned */
int gnome_init_with_popt_table(const char *app_id,
			       const char *app_version,
			       int argc, char **argv,
			       const struct poptOption *options,
			       int flags,
			       poptContext *return_ctx);

END_GNOME_DECLS

#endif
