#ifndef LIBGNOMEUI_H
#define LIBGNOMEUI_H

#include "libgnome/gnome-defs.h"
#include "libgnomeui/gnome-about.h"
#include "libgnomeui/gnome-actionarea.h"
#include "libgnomeui/gnome-app.h"
#include "libgnomeui/gnome-app-helper.h"
#include "libgnomeui/gnome-actionarea.h"
#include "libgnomeui/gnome-color-selector.h"
#include "libgnomeui/gnome-dns.h"
#include "libgnomeui/gnome-entry.h"
#include "libgnomeui/gnome-font-selector.h"
#include "libgnomeui/gnome-messagebox.h"
#include "libgnomeui/gnome-net.h"
#include "libgnomeui/gnome-pixmap.h"
#include "libgnomeui/gnome-properties.h"
#include "libgnomeui/gnome-scores.h"
#include "libgnomeui/gnome-startup.h"
#include "libgnomeui/gnome-rootwin.h"
#include "libgnomeui/gnome-stock.h"
#include "libgnomeui/gnome-client.h"
#include "libgnomeui/gtk-clock.h"
#include "libgnomeui/gtk-ted.h"
#include "libgnomeui/gtk-plug.h"
#include "libgnomeui/gtk-socket.h"

BEGIN_GNOME_DECLS

error_t gnome_init (char *app_id, struct argp *app_parser,
		    int argc, char **argv,
		    unsigned int flags, int *arg_index);

END_GNOME_DECLS

#endif
