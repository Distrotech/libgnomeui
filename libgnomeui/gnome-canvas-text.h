/* Text item type for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_CANVAS_TEXT_H
#define GNOME_CANVAS_TEXT_H

#include <libgnome/gnome-defs.h>
#include <gtk/gtkpacker.h> /* why the hell is GtkAnchorType here and not in gtkenums.h? */
#include "gnome-canvas.h"


BEGIN_GNOME_DECLS


/* Text item for the canvas.  Text items are positioned by an anchor point and an anchor direction.
 * The following object arguments are available:
 *
 * name			type			read/write	description
 * ------------------------------------------------------------------------------------------
 * text			string			RW		The string of the text label
 * x			double			RW		X coordinate of anchor point
 * y			double			RW		Y coordinate of anchor point
 * font			string			W		X logical font descriptor
 * font_gdk		GdkFont*		W		Pointer to a GdkFont
 * anchor		GtkAnchorType		RW		Anchor side for the text
 * justification	GtkJustification	RW		Justification for multiline text
 * fill_color		string			W		X color specification for text
 * fill_color_gdk	GdkColor*		W		Pointer to an allocated GdkColor
 */


#define GNOME_TYPE_CANVAS_TEXT            (gnome_canvas_text_get_type ())
#define GNOME_CANVAS_TEXT(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_TEXT, GnomeCanvasText))
#define GNOME_CANVAS_TEXT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_TEXT, GnomeCanvasTextClass))
#define GNOME_IS_CANVAS_TEXT(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_TEXT))
#define GNOME_IS_CANVAS_TEXT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_TEXT))


typedef struct _GnomeCanvasText GnomeCanvasText;
typedef struct _GnomeCanvasTextClass GnomeCanvasTextClass;

struct _GnomeCanvasText {
	GnomeCanvasItem item;

	char *text;			/* Text to display */

	double x, y;			/* Position at anchor */
	GdkFont *font;			/* Font for text */
	GtkAnchorType anchor;		/* Anchor side for text */
	GtkJustification justification;	/* Justification for text */

	gulong pixel;			/* Fill color */
	GdkGC *gc;			/* GC for drawing text */

	int width;			/* Rendered width in pixels */
	int height;			/* Rendered height in pixels */
	int cx, cy;			/* Top-left canvas coordinates for display */
};

struct _GnomeCanvasTextClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType gnome_canvas_text_get_type (void);


END_GNOME_DECLS

#endif
