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
#include "gnome-canvas.h"
#include "gnome-canvas-util.h"
#include <libart_lgpl/art_uta.h>
#include <libart_lgpl/art_svp.h>
#include <libart_lgpl/art_rgb.h>
#include <libart_lgpl/art_rgb_svp.h>
#include <libart_lgpl/art_uta_svp.h>


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
		theta1 = atan2 (y1 - y2, x1 - x2);

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

	dist = 0.5 * width / sin (0.5 * theta);
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

void
gnome_canvas_get_butt_points (double x1, double y1, double x2, double y2,
			      double width, int project,
			      double *bx1, double *by1, double *bx2, double *by2)
{
	double length;
	double dx, dy;

	width *= 0.5;
	dx = x2 - x1;
	dy = y2 - y1;
	length = sqrt (dx * dx + dy * dy);

	if (length < GNOME_CANVAS_EPSILON) {
		*bx1 = *bx2 = x2;
		*by1 = *by2 = y2;
	} else {
		dx = -width * (y2 - y1) / length;
		dy = width * (x2 - x1) / length;

		*bx1 = x2 + dx;
		*bx2 = x2 - dx;
		*by1 = y2 + dy;
		*by2 = y2 - dy;

		if (project) {
			*bx1 += dy;
			*bx2 += dy;
			*by1 -= dx;
			*by2 -= dx;
		}
	}
}

double
gnome_canvas_polygon_to_point (double *poly, int num_points, double x, double y)
{
	double best;
	int intersections;
	int i;
	double *p;
	double dx, dy;

	/* Iterate through all the edges in the polygon, updating best and intersections.
	 *
	 * When computing intersections, include left X coordinate of line within its range, but not
	 * Y coordinate.  Otherwise if the point lies exactly below a vertex we'll count it as two
	 * intersections.
	 */

	best = 1.0e36;
	intersections = 0;

	for (i = num_points, p = poly; i > 1; i--, p += 2) {
		double px, py, dist;

		/* Compute the point on the current edge closest to the point and update the
		 * intersection count.  This must be done separately for vertical edges, horizontal
		 * edges, and others.
		 */

		if (p[2] == p[0]) {
			/* Vertical edge */

			px = p[0];

			if (p[1] >= p[3]) {
				py = MIN (p[1], y);
				py = MAX (py, p[3]);
			} else {
				py = MIN (p[3], y);
				py = MAX (py, p[1]);
			}
		} else if (p[3] == p[1]) {
			/* Horizontal edge */

			py = p[1];

			if (p[0] >= p[2]) {
				px = MIN (p[0], x);
				px = MAX (px, p[2]);

				if ((y < py) && (x < p[0]) && (x >= p[2]))
					intersections++;
			} else {
				px = MIN (p[2], x);
				px = MAX (px, p[0]);

				if ((y < py) && (x < p[2]) && (x >= p[0]))
					intersections++;
			}
		} else {
			double m1, b1, m2, b2;
			int lower;

			/* Diagonal edge.  Convert the edge to a line equation (y = m1*x + b1), then
			 * compute a line perpendicular to this edge but passing through the point,
			 * (y = m2*x + b2).
			 */

			m1 = (p[3] - p[1]) / (p[2] - p[0]);
			b1 = p[1] - m1 * p[0];

			m2 = -1.0 / m1;
			b2 = y - m2 * x;

			px = (b2 - b1) / (m1 - m2);
			py = m1 * px + b1;

			if (p[0] > p[2]) {
				if (px > p[0]) {
					px = p[0];
					py = p[1];
				} else if (px < p[2]) {
					px = p[2];
					py = p[3];
				}
			} else {
				if (px > p[2]) {
					px = p[2];
					py = p[3];
				} else if (px < p[0]) {
					px = p[0];
					py = p[1];
				}
			}

			lower = (m1 * x + b1) > y;

			if (lower && (x >= MIN (p[0], p[2])) && (x < MAX (p[0], p[2])))
				intersections++;
		}

		/* Compute the distance to the closest point, and see if that is the best so far */

		dx = x - px;
		dy = y - py;
		dist = sqrt (dx * dx + dy * dy);
		if (dist < best)
			best = dist;
	}

	/* We've processed all the points.  If the number of intersections is odd, the point is
	 * inside the polygon.
	 */

	if (intersections & 0x1)
		return 0.0;
	else
		return best;
}

/* Here are some helper functions for aa rendering: */

/**
 * gnome_canvas_render_svp:
 * @buf: the canvas buffer to render over
 * @svp: the vector path to render
 * @rgba: the rgba color to render
 *
 * Render the svp over the buf.
 **/
void
gnome_canvas_render_svp (GnomeCanvasBuf *buf,
			 ArtSVP *svp,
			 guint32 rgba)
{
	guint32 fg_color, bg_color;

	if (buf->is_bg) {
		bg_color = buf->bg_color;
		fg_color = rgba >> 8; /* FIXME: this needs to be a composite */
		art_rgb_svp_aa (svp,
				buf->rect.x0, buf->rect.y0, buf->rect.x1, buf->rect.y1,
				fg_color, bg_color,
				buf->buf, buf->buf_rowstride,
				NULL);
		buf->is_bg = 0;
		buf->is_buf = 1;
	} else {
		art_rgb_svp_alpha (svp,
				   buf->rect.x0, buf->rect.y0, buf->rect.x1, buf->rect.y1,
				   rgba,
				   buf->buf, buf->buf_rowstride,
				   NULL);
	}
}

/**
 * gnome_canvas_update_svp:
 * @canvas: the canvas containing the svp that needs updating.
 * @p_svp: a pointer to the existing svp
 * @new_svp: the new svp
 *
 * Sets the svp to the new value, requesting repaint on what's changed.
 **/
void
gnome_canvas_update_svp (GnomeCanvas *canvas, ArtSVP **p_svp, ArtSVP *new_svp)
{
	ArtSVP *old_svp;
	ArtSVP *diff;
	ArtUta *repaint_uta;

	old_svp = *p_svp;
	if (old_svp != NULL && new_svp != NULL) {
#if 0
		/* should work, but cores :( */
		diff = art_svp_diff (old_svp, new_svp);
		art_svp_free (old_svp);
		repaint_uta = art_uta_from_svp (diff);
		art_svp_free (diff);
		gnome_canvas_request_redraw_uta (canvas, repaint_uta);
#else
		repaint_uta = art_uta_from_svp (old_svp);
		gnome_canvas_request_redraw_uta (canvas, repaint_uta);
		repaint_uta = art_uta_from_svp (new_svp);
		gnome_canvas_request_redraw_uta (canvas, repaint_uta);
#endif
	} else if (old_svp != NULL) {
		repaint_uta = art_uta_from_svp (old_svp);
		art_svp_free (old_svp);
		gnome_canvas_request_redraw_uta (canvas, repaint_uta);
	}
	*p_svp = new_svp;
}

/**
 * gnome_canvas_update_bbox:
 * @canvas: the canvas needing update
 * @x1, @y1, @x2, @y2: the new bbox
 *
 * Sets the bbox to the new value, requesting full repaint.
 **/
void
gnome_canvas_update_bbox (GnomeCanvasItem *item,
			  int x1, int y1, int x2, int y2)
{
	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
	item->x1 = x1;
	item->y1 = y1;
	item->x2 = x2;
	item->y2 = y2;
	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);
}

/**
 * gnome_canvas_ensure_buf
 * @buf: the buf that needs to be represened in RGB format
 *
 * Ensure that the buffer is in RGB format, suitable for compositing.
 **/
void
gnome_canvas_buf_ensure_buf (GnomeCanvasBuf *buf)
{
	guchar *bufptr;
	int y;

	if (!buf->is_buf) {
		bufptr = buf->buf;
		for (y = buf->rect.y0; y < buf->rect.y1; y++) {
			art_rgb_fill_run (buf->buf, buf->bg_color >> 16, (buf->bg_color >> 8) & 0xff, buf->bg_color & 0xff,
					  buf->rect.x1 - buf->rect.x0);
		}
		bufptr += buf->buf_rowstride;
		buf->is_buf = 1;
	}
}
