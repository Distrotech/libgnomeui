#ifndef LIBGNOMEUI_H
#define LIBGNOMEUI_H

#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-parse.h"
#include "libgnomeui/gnome-uidefs.h"
#include "libgnomeui/gnome-about.h"
#include "libgnomeui/gnome-actionarea.h"
#include "libgnomeui/gnome-app.h"
#include "libgnomeui/gnome-appbar.h"
#include "libgnomeui/gnome-app-helper.h"
#include "libgnomeui/gnome-app-util.h"
#include "libgnomeui/gnome-actionarea.h"
#ifdef GTK_HAVE_FEATURES_1_1_0
#include "libgnomeui/gnome-canvas.h"
#include "libgnomeui/gnome-canvas-image.h"
#include "libgnomeui/gnome-canvas-line.h"
#include "libgnomeui/gnome-canvas-rect-ellipse.h"
#include "libgnomeui/gnome-canvas-text.h"
#include "libgnomeui/gnome-canvas-util.h"
#include "libgnomeui/gnome-canvas-widget.h"
#endif
#include "libgnomeui/gnome-color-selector.h"
#include "libgnomeui/gnome-dentry-edit.h"
#include "libgnomeui/gnome-dialog.h"
#include "libgnomeui/gnome-dialog-util.h"
#include "libgnomeui/gnome-dns.h"
#include "libgnomeui/gnome-entry.h"
#include "libgnomeui/gnome-file-entry.h"
#include "libgnomeui/gnome-font-selector.h"
#include "libgnomeui/gnome-geometry.h"
#include "libgnomeui/gnome-icon-list.h"
#include "libgnomeui/gnome-icon-sel.h"
#include "libgnomeui/gnome-less.h"
#include "libgnomeui/gnome-messagebox.h"
#include "libgnomeui/gnome-net.h"
#include "libgnomeui/gnome-number-entry.h"
#ifdef GTK_HAVE_FEATURES_1_1_0
#include "libgnomeui/gnome-popupmenu.h"
#endif
#include "libgnomeui/gnome-pixmap.h"
#include "libgnomeui/gnome-preferences.h"
#include "libgnomeui/gnome-propertybox.h"
#include "libgnomeui/gnome-properties.h"
#include "libgnomeui/gnome-scores.h"
#include "libgnomeui/gnome-startup.h"
#include "libgnomeui/gnome-types.h"
#include "libgnomeui/gnome-rootwin.h"
#include "libgnomeui/gnome-stock.h"
#include "libgnomeui/gnome-client.h"
#include "libgnomeui/gtkcalendar.h"
#include "libgnomeui/gtkcauldron.h"
#include "libgnomeui/gtk-clock.h"
#include "libgnomeui/gtkdial.h"
#include "libgnomeui/gtklayout.h"
#include "libgnomeui/gtk-plug.h"
#include "libgnomeui/gtk-ted.h"
#include "libgnomeui/gtk-socket.h"
#include "libgnomeui/gtkspell.h"
#include "libgnomeui/gnome-dateedit.h"
#include "libgnomeui/gnome-calculator.h"
#include "libgnomeui/gnome-lamp.h"
#include "libgnomeui/gnome-mdi.h"
#include "libgnomeui/gnome-mdi-child.h"
#include "libgnomeui/gnometypebuiltins.h"

BEGIN_GNOME_DECLS

error_t gnome_init (char *app_id, struct argp *app_parser,
		    int argc, char **argv,
		    unsigned int flags, int *arg_index);
error_t gnome_init_with_data (char *app_id, struct argp *app_parser,
			      int argc, char **argv,
			      unsigned int flags, int *arg_index,
			      void *user_data);

END_GNOME_DECLS

#endif
