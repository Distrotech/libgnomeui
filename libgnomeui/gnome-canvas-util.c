/* Miscellaneous utility functions for the GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>
#include <glib.h>
#include "gnome-canvas-util.h"


GnomeCanvasPoints *
gnome_canvas_points_new (int num_points)
{
	GnomeCanvasPoints *points;

	g_return_val_if_fail (num_points > 1, NULL);

	points = g_new (GnomeCanvasPoints, 1);
	points->num_points = num_points;
	points->coords = g_new (double, 2 * num_points);

	return points;
}

void
gnome_canvas_points_free (GnomeCanvasPoints *points)
{
	g_return_if_fail (points != NULL);

	g_free (points->coords);
	g_free (points);
}
