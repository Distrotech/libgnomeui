#ifndef GNOME_CANVAS_LOAD_H
#define GNOME_CANVAS_LOAD_H

#include <libgnome/gnome-defs.h>
#include <gdk_imlib.h>

BEGIN_GNOME_DECLS

/* Note that it will only loads png files */
GdkImlibImage *gnome_canvas_load_alpha (const gchar *file);

void gnome_canvas_destroy_image (GdkImlibImage *image);

END_GNOME_DECLS

#endif

