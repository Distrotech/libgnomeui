/* Miscellaneous utility functions for the GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_CANVAS_UTIL_H
#define GNOEM_CANVAS_UTIL_H

#include <libgnome/gnome-defs.h>


BEGIN_GNOME_DECLS


/* This structure defines an array of points.  X coordinates are stored in the even-numbered
 * indices, and Y coordinates are stored in the odd-numbered indices.  num_points indicates the
 * number of points, so the array is 2*num_points elements big.
 */
typedef struct {
	int num_points;
	double *coords;
} GnomeCanvasPoints;


/* Allocate a new GnomeCanvasPoints structure with enough space for the specified number of points */
GnomeCanvasPoints *gnome_canvas_points_new (int num_points);

/* Free a points structure */
void gnome_canvas_points_free (GnomeCanvasPoints *points);


END_GNOME_DECLS

#endif
