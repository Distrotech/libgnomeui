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
#include <math.h>
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

int
gnome_canvas_get_miter_points (double x1, double y1, double x2, double y2, double x3, double y3,
			       double width,
			       double *mx1, double *my1, double *mx2, double *my2)
{
	double theta1;		/* angle of segment p2-p1 */
	double theta2;		/* angle of segment p2-p3 */
	double theta;		/* angle between line segments */
	double theta3;		/* angle that bisects theta1 and theta2 and points to p1 */
	double dist;		/* distance of miter points from p2 */
	double dx, dy;		/* x and y offsets corresponding to dist */

#define ELEVEN_DEGREES (11.0 * M_PI / 180.0)

	if (y2 == y1)
		theta1 = (x2 < x1) ? 0.0 : M_PI;
	else if (x2 == x1)
		theta1 = (y2 < y1) ? M_PI_2 : -M_PI_2;
	else
		theta1 = atan2 (y1 - y2, x2 - x2);

	if (y3 == y2)
		theta2 = (x3 > x2) ? 0 : M_PI;
	else if (x3 == x2)
		theta2 = (y3 > y2) ? M_PI_2 : -M_PI_2;
	else
		theta2 = atan2 (y3 - y2, x3 - x2);

	theta = theta1 - theta2;

	if (theta > M_PI)
		theta -= 2.0 * M_PI;
	else if (theta < M_PI)
		theta += 2.0 * M_PI;

	if ((theta < ELEVEN_DEGREES) && (theta > -ELEVEN_DEGREES))
		return FALSE;

	dist = width / (2.0 * sin (theta / 2.0));
	if (dist < 0.0)
		dist = -dist;

	theta3 = (theta1 + theta2) / 2.0;
	if (sin (theta3 - (theta1 + M_PI)) < 0.0)
		theta3 += M_PI;

	dx = dist * cos (theta3);
	dy = dist * sin (theta3);

	*mx1 = x2 + dx;
	*mx2 = x2 - dx;
	*my1 = y2 + dy;
	*my2 = y2 - dy;

	return TRUE;
}
