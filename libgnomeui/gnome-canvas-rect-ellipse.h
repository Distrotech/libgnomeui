/* GNOME libraries - Rectangle and ellipse items for the GNOME canvas.
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas
 * widget.  Tk is copyrighted by the Regents of the University of California,
 * Sun Microsystems, and other parties.
 *
 * Copyright (C) 1999 The Free Software Foundation
 *
 * Authors: Federico Mena <federico@nuclecu.unam.mx>
 *          Raph Levien <raph@gimp.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
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
#define GNOME_CANVAS_RE(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_RE, GnomeCanvasRE))
#define GNOME_CANVAS_RE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_RE,	\
					 GnomeCanvasREClass))
#define GNOME_IS_CANVAS_RE(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_RE))
#define GNOME_IS_CANVAS_RE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_RE))


typedef struct _GnomeCanvasRE      GnomeCanvasRE;
typedef struct _GnomeCanvasREClass GnomeCanvasREClass;

struct _GnomeCanvasRE {
	GnomeCanvasItem item;

	/* Private data */
	gpointer priv;
};

struct _GnomeCanvasREClass {
	GnomeCanvasItemClass parent_class;
};


GtkType gnome_canvas_re_get_type (void);



/* Rectangle item.  It inherits the configurable and queryable arguments from
 * the base GnomeCanvasRE.
 */

#define GNOME_TYPE_CANVAS_RECT            (gnome_canvas_rect_get_type ())
#define GNOME_CANVAS_RECT(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_RECT,	\
					   GnomeCanvasRect))
#define GNOME_CANVAS_RECT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass),			\
					   GNOME_TYPE_CANVAS_RECT, GnomeCanvasRectClass))
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


GtkType gnome_canvas_rect_get_type (void);



/* Ellipse item.  It inherits the configurable and queryable arguments from the
 * base GnomeCanvasRE.
 */

#define GNOME_TYPE_CANVAS_ELLIPSE            (gnome_canvas_ellipse_get_type ())
#define GNOME_CANVAS_ELLIPSE(obj)            (GTK_CHECK_CAST ((obj),				\
					      GNOME_TYPE_CANVAS_ELLIPSE, GnomeCanvasEllipse))
#define GNOME_CANVAS_ELLIPSE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass),			\
					      GNOME_TYPE_CANVAS_ELLIPSE, GnomeCanvasEllipseClass))
#define GNOME_IS_CANVAS_ELLIPSE(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_ELLIPSE))
#define GNOME_IS_CANVAS_ELLIPSE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass),			\
					      GNOME_TYPE_CANVAS_ELLIPSE))


typedef struct _GnomeCanvasEllipse GnomeCanvasEllipse;
typedef struct _GnomeCanvasEllipseClass GnomeCanvasEllipseClass;

struct _GnomeCanvasEllipse {
	GnomeCanvasRE re;
};

struct _GnomeCanvasEllipseClass {
	GnomeCanvasREClass parent_class;
};


GtkType gnome_canvas_ellipse_get_type (void);



END_GNOME_DECLS

#endif
