/* Rectangle and ellipse item types for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas
 * widget.  Tk is copyrighted by the Regents of the University of California,
 * Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Authors: Federico Mena <federico@nuclecu.unam.mx>
 *          Raph Levien <raph@gimp.org>
 */

#ifndef GNOME_CANVAS_RECT_ELLIPSE_H
#define GNOME_CANVAS_RECT_ELLIPSE_H

#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-canvas.h>

BEGIN_GNOME_DECLS


/* Base class for rectangle and ellipse item types.  These are defined by their
 * top-left and bottom-right corners.  Rectangles and ellipses share the
 * following arguments:

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
 * fill_color_rgba	uint		RW		32-bit RGBA color for fill
 * outline_color	string		W		X color specification for outline color,
 *							or NULL pointer for no color (transparent)
 * outline_color_gdk	GdkColor*	RW		Allocated GdkColor for outline
 * outline_color_rgba	uint		RW		32-bit RGBA color for outline
 * fill_stipple		GdkBitmap*	RW		Stipple pattern for fill
 * outline_stipple	GdkBitmap*	RW		Stipple pattern for outline
 * width_pixels		uint		W		Width of the outline in pixels.  The outline
 *							will not be scaled when the canvas zoom
 *							factor is changed.
 * width_units		double		W		Width of the outline in canvas units.  The
 *							outline will be scaled when the canvas zoom
 *							factor is changed.
 * width		double		R		Width of the outline.
 * width_is_in_pixels	boolean		R		Whether the outline width is in pixels
 *							or units.
 */


#define GNOME_TYPE_CANVAS_RE            (gnome_canvas_re_get_type ())
#define GNOME_CANVAS_RE(obj)            (GTK_CHECK_CAST ((obj), 		\
	GNOME_TYPE_CANVAS_RE, GnomeCanvasRE))
#define GNOME_CANVAS_RE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass),		\
	GNOME_TYPE_CANVAS_RE, GnomeCanvasREClass))
#define GNOME_IS_CANVAS_RE(obj)         (GTK_CHECK_TYPE ((obj),			\
	GNOME_TYPE_CANVAS_RE))
#define GNOME_IS_CANVAS_RE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass),		\
	GNOME_TYPE_CANVAS_RE))


typedef struct _GnomeCanvasRE      GnomeCanvasRE;
typedef struct _GnomeCanvasREClass GnomeCanvasREClass;

/* This structure has been converted to use public and private parts.  To avoid
 * breaking binary compatibility, the slots for private fields have been
 * replaced with padding.  Please remove these fields when gnome-libs has
 * reached another major version and it is "fine" to break binary compatibility.
 */
struct _GnomeCanvasRE {
	GnomeCanvasItem item;

	/* Corners of item */
	double x1, y1, x2, y2;

	/* Outline width */
	double width;

	/* Fill and outline colors, RGBA */
	guint fill_color;
	guint outline_color;

	/* Fill and outline pixel values */
	gulong fill_pixel;
	gulong outline_pixel;

	/* Private data */
	gpointer priv; /* was GdkBitmap *fill_stipple */

	gpointer pad1; /* was GdkBitmap *outline_stipple */
	gpointer pad2; /* was GdkGC *fill_gc */
	gpointer pad3; /* was GdkGC *outline_gc */
	gpointer pad4; /* was ArtSVP *fill_svp */
	gpointer pad5; /* was ArtSVP *outline_svp */
	unsigned int pad6 : 1; /* was unsigned int fill_set : 1 */
	unsigned int pad7 : 1; /* was unsigned int outline_set : 1 */
	unsigned int pad8 : 1; /* was unsigned int width_pixels : 1 */
};

struct _GnomeCanvasREClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType gnome_canvas_re_get_type (void);


/* Rectangle item.  No configurable or queryable arguments are available (use
 * those in GnomeCanvasRE).
 */


#define GNOME_TYPE_CANVAS_RECT            (gnome_canvas_rect_get_type ())
#define GNOME_CANVAS_RECT(obj)            (GTK_CHECK_CAST ((obj),		\
	GNOME_TYPE_CANVAS_RECT, GnomeCanvasRect))
#define GNOME_CANVAS_RECT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass),	\
	GNOME_TYPE_CANVAS_RECT, GnomeCanvasRectClass))
#define GNOME_IS_CANVAS_RECT(obj)         (GTK_CHECK_TYPE ((obj),		\
	GNOME_TYPE_CANVAS_RECT))
#define GNOME_IS_CANVAS_RECT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass),	\
	GNOME_TYPE_CANVAS_RECT))


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


/* Ellipse item.  No configurable or queryable arguments are available (use
 * those in GnomeCanvasRE).
 */


#define GNOME_TYPE_CANVAS_ELLIPSE            (gnome_canvas_ellipse_get_type ())
#define GNOME_CANVAS_ELLIPSE(obj)            (GTK_CHECK_CAST ((obj),		\
	GNOME_TYPE_CANVAS_ELLIPSE, GnomeCanvasEllipse))
#define GNOME_CANVAS_ELLIPSE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass),	\
	GNOME_TYPE_CANVAS_ELLIPSE, GnomeCanvasEllipseClass))
#define GNOME_IS_CANVAS_ELLIPSE(obj)         (GTK_CHECK_TYPE ((obj),		\
	GNOME_TYPE_CANVAS_ELLIPSE))
#define GNOME_IS_CANVAS_ELLIPSE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass),	\
	GNOME_TYPE_CANVAS_ELLIPSE))


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
