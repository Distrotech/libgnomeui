#ifndef GNOME_INIT_H
#define GNOME_INIT_H

BEGIN_GNOME_DECLS

#include "libgnome/gnomelib-init2.h"


extern const char libgnomeui_param_crash_dialog[], libgnomeui_param_display[];
#define LIBGNOMEUI_PARAM_CRASH_DIALOG libgnomeui_param_crash_dialog
#define LIBGNOMEUI_PARAM_DISPLAY libgnomeui_param_display

extern GnomeModuleInfo libgnomeui_module_info, gtk_module_info;

#define LIBGNOMEUI_INIT GNOME_PARAM_MODULE,&libgnomeui_module_info

/* The gnome_init define is in libgnomeui.h so it can be a macro */
int gnome_init_with_popt_table(const char *app_id,
			       const char *app_version,
			       int argc, char **argv,
			       const struct poptOption *options,
			       int flags,
			       poptContext *return_ctx);


END_GNOME_DECLS

#endif
