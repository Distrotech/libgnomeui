/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/* Image item type for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_CANVAS_IMAGE_H
#define GNOME_CANVAS_IMAGE_H

#include <libgnome/gnome-defs.h>
#include <gtk/gtkpacker.h> /* why the hell is GtkAnchorType here and not in gtkenums.h? */
#include <libart_lgpl/art_misc.h>
#include <libart_lgpl/art_pixbuf.h>
#include <gdk_imlib.h>
#include "gnome-canvas.h"


BEGIN_GNOME_DECLS


/* Image item for the canvas.  Images are positioned by anchoring them to a point.
 * The following arguments are available:
 *
 * name		type			read/write	description
 * ------------------------------------------------------------------------------------------
 * image	GdkImlibImage*		RW		Pointer to a GdkImlibImage
 * pixbuf	ArtPixBuf*		W		Pointer to an ArtPixBuf (aa-mode)
 * x		double			RW		X coordinate of anchor point
 * y		double			RW		Y coordinate of anchor point
 * width	double			RW		Width to scale image to, in canvas units
 * height	double			RW		Height to scale image to, in canvas units
 * anchor	GtkAnchorType		RW		Anchor side for the image
 */


#define GNOME_TYPE_CANVAS_IMAGE            (gnome_canvas_image_get_type ())
#define GNOME_CANVAS_IMAGE(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_IMAGE, GnomeCanvasImage))
#define GNOME_CANVAS_IMAGE_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_IMAGE, GnomeCanvasImageClass))
#define GNOME_IS_CANVAS_IMAGE(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_IMAGE))
#define GNOME_IS_CANVAS_IMAGE_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_IMAGE))
#define GNOME_CANVAS_IMAGE_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_CANVAS_IMAGE, GnomeCanvasImageClass))


typedef struct _GnomeCanvasImage GnomeCanvasImage;
typedef struct _GnomeCanvasImageClass GnomeCanvasImageClass;

struct _GnomeCanvasImage {
	GnomeCanvasItem item;

	GdkImlibImage *im;		/* The image to paint */
	GdkPixmap *pixmap;		/* Pixmap rendered from the image */
	GdkBitmap *mask;		/* Mask rendered from the image */

	double x, y;			/* Position at anchor, item relative */
	double width, height;		/* Size of image, item relative */
	GtkAnchorType anchor;		/* Anchor side for image */

	int cx, cy;			/* Top-left canvas coordinates for display */
	int cwidth, cheight;		/* Rendered size in pixels */
	GdkGC *gc;			/* GC for drawing image */

	unsigned int need_recalc : 1;	/* Do we need to rescale the image? */

	ArtPixBuf *pixbuf;		/* A pixbuf, for aa rendering */
	double affine[6];               /* The item -> canvas affine */
};

struct _GnomeCanvasImageClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType gnome_canvas_image_get_type (void);


END_GNOME_DECLS

#endif
