/* Image item type for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_CANVAS_IMAGE_H
#define GNOME_CANVAS_IMAGE_H

#include <libgnome/gnome-defs.h>
#include <gtk/gtkpacker.h> /* why the hell is GtkAnchorType here and not in gtkenums.h? */
#include <gdk_imlib.h>
#include "gnome-canvas.h"


BEGIN_GNOME_DECLS


#define GNOME_TYPE_CANVAS_IMAGE            (gnome_canvas_image_get_type ())
#define GNOME_CANVAS_IMAGE(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_IMAGE, GnomeCanvasImage))
#define GNOME_CANVAS_IMAGE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_IMAGE, GnomeCanvasImageClass))
#define GNOME_IS_CANVAS_IMAGE(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_IMAGE))
#define GNOME_IS_CANVAS_IMAGE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_IMAGE))


typedef struct _GnomeCanvasImage GnomeCanvasImage;
typedef struct _GnomeCanvasImageClass GnomeCanvasImageClass;

struct _GnomeCanvasImage {
	GnomeCanvasItem item;

	GdkImlibImage *im;		/* The image to paint */
	GdkPixmap *pixmap;		/* Pixmap rendered from the image */
	GdkBitmap *mask;		/* Mask rendered from the image */

	double x, y;			/* Position at anchor */
	double width, height;		/* Size of image in units */
	GtkAnchorType anchor;		/* Anchor side for image */

	int cx, cy;			/* Top-left canvas coordinates for display */
	int cwidth, cheight;		/* Rendered size in pixels */
	GdkGC *gc;			/* GC for drawing image */

	int need_recalc : 1;		/* Do we need to rescale the image? */
};

struct _GnomeCanvasImageClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType gnome_canvas_image_get_type (void);

/* Notifies an image item that its imlib image has changed and must be re-rendered */
void gnome_canvas_image_update (GnomeCanvasImage *image);


END_GNOME_DECLS

#endif
