/* Line/curve item type for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_CANVAS_LINE_H
#define GNOME_CANVAS_LINE_H

#include <libgnome/gnome-defs.h>
#include "gnome-canvas.h"


BEGIN_GNOME_DECLS


#define GNOME_TYPE_CANVAS_LINE            (gnome_canvas_line_get_type ())
#define GNOME_CANVAS_LINE(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_LINE, GnomeCanvasLine))
#define GNOME_CANVAS_LINE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_LINE, GnomeCanvasLineClass))
#define GNOME_IS_CANVAS_LINE(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_LINE))
#define GNOME_IS_CANVAS_LINE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_LINE))


typedef struct _GnomeCanvasLine GnomeCanvasLine;
typedef struct _GnomeCanvasLineClass GnomeCanvasLineClass;

struct _GnomeCanvasLine {
	GnomeCanvasItem item;

	int num_points;		/* Number of points in the line */
	double *coords;		/* Array of coordinates for the line's points.  X coords are in the
				 * even indices, Y coords are in the odd indices.  If the line has
				 * arrowheads then the first and last points have been adjusted to
				 * refer to the necks of the arrowheads rather than their tips.  The
				 * actual endpoints are stored in the first_arrow and last_arrow
				 * arrays, if they exist.
				 */

	double width;		/* Width of the line */

	gulong pixel;		/* Color for line */

	GdkCapStyle cap;	/* Cap style for line */
	GdkJoinStyle join;	/* Join style for line */

	int width_pixels : 1;	/* Is the width specified in pixels or units? */
	int first_arrow : 1;	/* Draw first arrowhead? */
	int last_arrow : 1;	/* Draw last arrowhead? */
	int smooth : 1;		/* Smooth line (with parabolic splines)? */

	double shape_a;		/* Distance from tip of arrowhead to center */
	double shape_b;		/* Distance from tip of arrowhead to trailing point, measured along shaft */
	double shape_c;		/* Distance of trailing points from outside edge of shaft */

	double *first_arrow;	/* Array of points describing polygon for the first arrowhead */
	double *last_arrow;	/* Array of points describing polygon for the last arrowhead */

	int spline_steps;	/* Number of steps in each spline segment */

	GdkGC *gc;		/* GC for drawing line */
	GdkGC *arrow_gc;	/* GC for drawing arrowheads */
};

struct _GnomeCanvasLineClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType gnome_canvas_line_get_type (void);


END_GNOME_DECLS

#endif
