/* Rectangle and ellipse item types for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_CANVAS_RECT_ELLIPSE_H
#define GNOME_CANVAS_RECT_ELLIPSE_H

#include <libgnome/gnome-defs.h>
#include "gnome-canvas.h"

#include <libart_lgpl/art_svp.h>

BEGIN_GNOME_DECLS


/* Base class for rectangle and ellipse item types.  These are defined by their top-left and
 * bottom-right corners.  Rectangles and ellipses share the following arguments:
 *
 * name			type		read/write	description
 * ------------------------------------------------------------------------------------------
 * x1			double		RW		Leftmost coordinate of rectangle or ellipse
 * y1			double		RW		Topmost coordinate of rectangle or ellipse
 * x2			double		RW		Rightmost coordinate of rectangle or ellipse
 * y2			double		RW		Bottommost coordinate of rectangle or ellipse
 * fill_color		string		W		X color specification for fill color,
 *							or NULL pointer for no color (transparent)
 * fill_color_gdk	GdkColor*	RW		Allocated GdkColor for fill
 * outline_color	string		W		X color specification for outline color,
 *							or NULL pointer for no color (transparent)
 * outline_color_gdk	GdkColor*	RW		Allocated GdkColor for outline
 * fill_stipple		GdkBitmap*	RW		Stipple pattern for fill
 * outline_stipple	GdkBitmap*	RW		Stipple pattern for outline
 * width_pixels		uint		RW		Width of the outline in pixels.  The outline will
 *							not be scaled when the canvas zoom factor is changed.
 * width_units		double		RW		Width of the outline in canvas units.  The outline
 *							will be scaled when the canvas zoom factor is changed.
 */


#define GNOME_TYPE_CANVAS_RE            (gnome_canvas_re_get_type ())
#define GNOME_CANVAS_RE(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_RE, GnomeCanvasRE))
#define GNOME_CANVAS_RE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_RE, GnomeCanvasREClass))
#define GNOME_IS_CANVAS_RE(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_RE))
#define GNOME_IS_CANVAS_RE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_RE))


typedef struct _GnomeCanvasRE      GnomeCanvasRE;
typedef struct _GnomeCanvasREClass GnomeCanvasREClass;

struct _GnomeCanvasRE {
	GnomeCanvasItem item;

	double x1, y1, x2, y2;		/* Corners of item */
	double width;			/* Outline width */

	guint fill_color;		/* Fill color, RGBA */
	guint outline_color;		/* Outline color, RGBA */

	gulong fill_pixel;		/* Fill color */
	gulong outline_pixel;		/* Outline color */

	GdkBitmap *fill_stipple;	/* Stipple for fill */
	GdkBitmap *outline_stipple;	/* Stipple for outline */

	GdkGC *fill_gc;			/* GC for filling */
	GdkGC *outline_gc;		/* GC for outline */

	/* Antialiased specific stuff follows */

	ArtSVP *fill_svp;		/* The SVP for the filled shape */
	ArtSVP *outline_svp;		/* The SVP for the outline shape */

	/* Configuration flags */

	unsigned int fill_set : 1;	/* Is fill color set? */
	unsigned int outline_set : 1;	/* Is outline color set? */
	unsigned int width_pixels : 1;	/* Is outline width specified in pixels or units? */
};

struct _GnomeCanvasREClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType gnome_canvas_re_get_type (void);


/* Rectangle item.  No configurable or queryable arguments are available (use those in
 * GnomeCanvasRE).
 */


#define GNOME_TYPE_CANVAS_RECT            (gnome_canvas_rect_get_type ())
#define GNOME_CANVAS_RECT(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_RECT, GnomeCanvasRect))
#define GNOME_CANVAS_RECT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_RECT, GnomeCanvasRectClass))
#define GNOME_IS_CANVAS_RECT(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_RECT))
#define GNOME_IS_CANVAS_RECT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_RECT))


typedef struct _GnomeCanvasRect GnomeCanvasRect;
typedef struct _GnomeCanvasRectClass GnomeCanvasRectClass;

struct _GnomeCanvasRect {
	GnomeCanvasRE re;
};

struct _GnomeCanvasRectClass {
	GnomeCanvasREClass parent_class;
};


/* Standard Gtk function */
GtkType gnome_canvas_rect_get_type (void);


/* Ellipse item.  No configurable or queryable arguments are available (use those in
 * GnomeCanvasRE).
 */


#define GNOME_TYPE_CANVAS_ELLIPSE            (gnome_canvas_ellipse_get_type ())
#define GNOME_CANVAS_ELLIPSE(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_ELLIPSE, GnomeCanvasEllipse))
#define GNOME_CANVAS_ELLIPSE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_ELLIPSE, GnomeCanvasEllipseClass))
#define GNOME_IS_CANVAS_ELLIPSE(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_ELLIPSE))
#define GNOME_IS_CANVAS_ELLIPSE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_ELLIPSE))


typedef struct _GnomeCanvasEllipse GnomeCanvasEllipse;
typedef struct _GnomeCanvasEllipseClass GnomeCanvasEllipseClass;

struct _GnomeCanvasEllipse {
	GnomeCanvasRE re;
};

struct _GnomeCanvasEllipseClass {
	GnomeCanvasREClass parent_class;
};


/* Standard Gtk function */
GtkType gnome_canvas_ellipse_get_type (void);


END_GNOME_DECLS

#endif
