/* Text item type for GnomeCanvas widget
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <config.h>
#include <math.h>
#include "gnome-canvas-text.h"
#include <gdk/gdkx.h> /* for BlackPixel */

#include "libart_lgpl/art_affine.h"
#include "libart_lgpl/art_rgb.h"
#include "libart_lgpl/art_rgb_bitmap_affine.h"
#include "gnome-canvas-util.h"

/* This defines a line of text */
struct line {
	char *text;	/* Line's text, it is a pointer into the text->text string */
	int length;	/* Line's length in characters */
	int width;	/* Line's width in pixels */
};

enum {
	ARG_0,
	ARG_TEXT,
	ARG_X,
	ARG_Y,
	ARG_FONT,
	ARG_FONT_GDK,
	ARG_ANCHOR,
	ARG_JUSTIFICATION,
	ARG_CLIP_WIDTH,
	ARG_CLIP_HEIGHT,
	ARG_CLIP,
	ARG_X_OFFSET,
	ARG_Y_OFFSET,
	ARG_FILL_COLOR,
	ARG_FILL_COLOR_GDK,
	ARG_FILL_COLOR_RGBA,
	ARG_FILL_STIPPLE,
	ARG_TEXT_WIDTH,
	ARG_TEXT_HEIGHT
};


static void gnome_canvas_text_class_init (GnomeCanvasTextClass *class);
static void gnome_canvas_text_init       (GnomeCanvasText      *text);
static void gnome_canvas_text_destroy    (GtkObject            *object);
static void gnome_canvas_text_set_arg    (GtkObject            *object,
					  GtkArg               *arg,
					  guint                 arg_id);
static void gnome_canvas_text_get_arg    (GtkObject            *object,
					  GtkArg               *arg,
					  guint                 arg_id);

static void   gnome_canvas_text_update      (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags);
static void   gnome_canvas_text_realize     (GnomeCanvasItem *item);
static void   gnome_canvas_text_unrealize   (GnomeCanvasItem *item);
static void   gnome_canvas_text_draw        (GnomeCanvasItem *item, GdkDrawable *drawable,
					     int x, int y, int width, int height);
static double gnome_canvas_text_point       (GnomeCanvasItem *item, double x, double y,
					     int cx, int cy, GnomeCanvasItem **actual_item);
static void   gnome_canvas_text_translate   (GnomeCanvasItem *item, double dx, double dy);
static void   gnome_canvas_text_bounds      (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2);
static void   gnome_canvas_text_render      (GnomeCanvasItem *item, GnomeCanvasBuf *buf);

static GnomeCanvasTextSuckFont * gnome_canvas_suck_font (GdkFont *font);
static void gnome_canvas_suck_font_free (GnomeCanvasTextSuckFont *suckfont);


static GnomeCanvasItemClass *parent_class;


GtkType
gnome_canvas_text_get_type (void)
{
	static GtkType text_type = 0;

	if (!text_type) {
		GtkTypeInfo text_info = {
			"GnomeCanvasText",
			sizeof (GnomeCanvasText),
			sizeof (GnomeCanvasTextClass),
			(GtkClassInitFunc) gnome_canvas_text_class_init,
			(GtkObjectInitFunc) gnome_canvas_text_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		text_type = gtk_type_unique (gnome_canvas_item_get_type (), &text_info);
	}

	return text_type;
}

static void
gnome_canvas_text_class_init (GnomeCanvasTextClass *class)
{
	GtkObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) class;
	item_class = (GnomeCanvasItemClass *) class;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gtk_object_add_arg_type ("GnomeCanvasText::text", GTK_TYPE_STRING, GTK_ARG_READWRITE, ARG_TEXT);
	gtk_object_add_arg_type ("GnomeCanvasText::x", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_X);
	gtk_object_add_arg_type ("GnomeCanvasText::y", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_Y);
	gtk_object_add_arg_type ("GnomeCanvasText::font", GTK_TYPE_STRING, GTK_ARG_WRITABLE, ARG_FONT);
	gtk_object_add_arg_type ("GnomeCanvasText::font_gdk", GTK_TYPE_GDK_FONT, GTK_ARG_READWRITE, ARG_FONT_GDK);
	gtk_object_add_arg_type ("GnomeCanvasText::anchor", GTK_TYPE_ANCHOR_TYPE, GTK_ARG_READWRITE, ARG_ANCHOR);
	gtk_object_add_arg_type ("GnomeCanvasText::justification", GTK_TYPE_JUSTIFICATION, GTK_ARG_READWRITE, ARG_JUSTIFICATION);
	gtk_object_add_arg_type ("GnomeCanvasText::clip_width", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_CLIP_WIDTH);
	gtk_object_add_arg_type ("GnomeCanvasText::clip_height", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_CLIP_HEIGHT);
	gtk_object_add_arg_type ("GnomeCanvasText::clip", GTK_TYPE_BOOL, GTK_ARG_READWRITE, ARG_CLIP);
	gtk_object_add_arg_type ("GnomeCanvasText::x_offset", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_X_OFFSET);
	gtk_object_add_arg_type ("GnomeCanvasText::y_offset", GTK_TYPE_DOUBLE, GTK_ARG_READWRITE, ARG_Y_OFFSET);
	gtk_object_add_arg_type ("GnomeCanvasText::fill_color", GTK_TYPE_STRING, GTK_ARG_WRITABLE, ARG_FILL_COLOR);
	gtk_object_add_arg_type ("GnomeCanvasText::fill_color_gdk", GTK_TYPE_GDK_COLOR, GTK_ARG_READWRITE, ARG_FILL_COLOR_GDK);
	gtk_object_add_arg_type ("GnomeCanvasText::fill_color_rgba", GTK_TYPE_UINT, GTK_ARG_READWRITE, ARG_FILL_COLOR_RGBA);
	gtk_object_add_arg_type ("GnomeCanvasText::fill_stipple", GTK_TYPE_GDK_WINDOW, GTK_ARG_READWRITE, ARG_FILL_STIPPLE);
	gtk_object_add_arg_type ("GnomeCanvasText::text_width", GTK_TYPE_DOUBLE, GTK_ARG_READABLE, ARG_TEXT_WIDTH);
	gtk_object_add_arg_type ("GnomeCanvasText::text_height", GTK_TYPE_DOUBLE, GTK_ARG_READABLE, ARG_TEXT_HEIGHT);

	object_class->destroy = gnome_canvas_text_destroy;
	object_class->set_arg = gnome_canvas_text_set_arg;
	object_class->get_arg = gnome_canvas_text_get_arg;

	item_class->update = gnome_canvas_text_update;
	item_class->realize = gnome_canvas_text_realize;
	item_class->unrealize = gnome_canvas_text_unrealize;
	item_class->draw = gnome_canvas_text_draw;
	item_class->point = gnome_canvas_text_point;
	item_class->translate = gnome_canvas_text_translate;
	item_class->bounds = gnome_canvas_text_bounds;
	item_class->render = gnome_canvas_text_render;
}

static void
gnome_canvas_text_init (GnomeCanvasText *text)
{
	text->x = 0.0;
	text->y = 0.0;
	text->font = gdk_font_load ("fixed"); /* lame default */
	g_assert (text->font != NULL);
	text->anchor = GTK_ANCHOR_CENTER;
	text->justification = GTK_JUSTIFY_LEFT;
	text->clip_width = 0.0;
	text->clip_height = 0.0;
	text->xofs = 0.0;
	text->yofs = 0.0;
}

static void
gnome_canvas_text_destroy (GtkObject *object)
{
	GnomeCanvasText *text;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_CANVAS_TEXT (object));

	text = GNOME_CANVAS_TEXT (object);

	if (text->text)
		g_free (text->text);

	if (text->lines)
		g_free (text->lines);

	gdk_font_unref (text->font);

	if (text->stipple)
		gdk_bitmap_unref (text->stipple);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
get_bounds_item_relative (GnomeCanvasText *text, double *px1, double *py1, double *px2, double *py2)
{
	GnomeCanvasItem *item;
	double x, y;
	double clip_x, clip_y;

	item = GNOME_CANVAS_ITEM (text);

	x = text->x;
	y = text->y;

	clip_x = x;
	clip_y = y;

	/* Calculate text dimensions */

	if (text->text)
		text->height = (text->font->ascent + text->font->descent) * text->num_lines;
	else
		text->height = 0;

	/* Anchor text */

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		x -= text->max_width / 2;
		clip_x -= text->clip_width / 2;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		x -= text->max_width;
		clip_x -= text->clip_width;
		break;
	}

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		y -= text->height / 2;
		clip_y -= text->clip_height / 2;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		y -= text->height;
		clip_y -= text->clip_height;
		break;
	}

	/* Bounds */

	if (text->clip) {
		/* maybe do bbox intersection here? */
		*px1 = clip_x;
		*py1 = clip_y;
		*px2 = clip_x + text->clip_width;
		*py2 = clip_y + text->clip_height;
	} else {
		*px1 = x;
		*py1 = y;
		*px2 = x + text->max_width;
		*py2 = y + text->height;
	}
}

static void
get_bounds (GnomeCanvasText *text, double *px1, double *py1, double *px2, double *py2)
{
	GnomeCanvasItem *item;
	double wx, wy;

	item = GNOME_CANVAS_ITEM (text);

	/* Get canvas pixel coordinates for text position */

	wx = text->x;
	wy = text->y;
	gnome_canvas_item_i2w (item, &wx, &wy);
	gnome_canvas_w2c (item->canvas, wx + text->xofs, wy + text->yofs, &text->cx, &text->cy);

	/* Get canvas pixel coordinates for clip rectangle position */

	gnome_canvas_w2c (item->canvas, wx, wy, &text->clip_cx, &text->clip_cy);
	text->clip_cwidth = text->clip_width * item->canvas->pixels_per_unit;
	text->clip_cheight = text->clip_height * item->canvas->pixels_per_unit;

	/* Calculate text dimensions */

	if (text->text)
		text->height = (text->font->ascent + text->font->descent) * text->num_lines;
	else
		text->height = 0;

	/* Anchor text */

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		text->cx -= text->max_width / 2;
		text->clip_cx -= text->clip_cwidth / 2;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		text->cx -= text->max_width;
		text->clip_cx -= text->clip_cwidth;
		break;
	}

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		text->cy -= text->height / 2;
		text->clip_cy -= text->clip_cheight / 2;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		text->cy -= text->height;
		text->clip_cy -= text->clip_cheight;
		break;
	}

	/* Bounds */

	if (text->clip) {
		*px1 = text->clip_cx;
		*py1 = text->clip_cy;
		*px2 = text->clip_cx + text->clip_cwidth;
		*py2 = text->clip_cy + text->clip_cheight;
	} else {
		*px1 = text->cx;
		*py1 = text->cy;
		*px2 = text->cx + text->max_width;
		*py2 = text->cy + text->height;
	}
}

/* Recalculates the bounding box of the text item.  The bounding box is defined by the text's
 * extents if the clip rectangle is disabled.  If it is enabled, the bounding box is defined by the
 * clip rectangle itself.
 */
static void
recalc_bounds (GnomeCanvasText *text)
{
	GnomeCanvasItem *item;

	item = GNOME_CANVAS_ITEM (text);

	get_bounds (text, &item->x1, &item->y1, &item->x2, &item->y2);

	gnome_canvas_group_child_bounds (GNOME_CANVAS_GROUP (item->parent), item);
}

/* Calculates the line widths (in pixels) of the text's splitted lines */
static void
calc_line_widths (GnomeCanvasText *text)
{
	struct line *lines;
	int i;

	lines = text->lines;
	text->max_width = 0;

	if (!lines)
		return;

	for (i = 0; i < text->num_lines; i++) {
		if (lines->length != 0) {
			lines->width = gdk_text_width (text->font, lines->text, lines->length);

			if (lines->width > text->max_width)
				text->max_width = lines->width;
		}

		lines++;
	}
}

/* Splits the text of the text item into lines */
static void
split_into_lines (GnomeCanvasText *text)
{
	char *p;
	struct line *lines;
	int len;

	/* Free old array of lines */

	if (text->lines)
		g_free (text->lines);

	text->lines = NULL;
	text->num_lines = 0;

	if (!text->text)
		return;

	/* First, count the number of lines */

	for (p = text->text; *p; p++)
		if (*p == '\n')
			text->num_lines++;

	text->num_lines++;

	/* Allocate array of lines and calculate split positions */

	text->lines = lines = g_new0 (struct line, text->num_lines);
	len = 0;

	for (p = text->text; *p; p++) {
		if (*p == '\n') {
			lines->length = len;
			lines++;
			len = 0;
		} else if (len == 0) {
			len++;
			lines->text = p;
		} else
			len++;
	}

	lines->length = len;

	calc_line_widths (text);
}

/* Convenience function to set the text's GC's foreground color */
static void
set_text_gc_foreground (GnomeCanvasText *text)
{
	GdkColor c;

	if (!text->gc)
		return;

	c.pixel = text->pixel;
	gdk_gc_set_foreground (text->gc, &c);
}

/* Sets the stipple pattern for the text */
static void
set_stipple (GnomeCanvasText *text, GdkBitmap *stipple, int reconfigure)
{
	if (text->stipple && !reconfigure)
		gdk_bitmap_unref (text->stipple);

	text->stipple = stipple;
	if (stipple && !reconfigure)
		gdk_bitmap_ref (stipple);

	if (text->gc)
		if (stipple) {
			gdk_gc_set_stipple (text->gc, stipple);
			gdk_gc_set_fill (text->gc, GDK_STIPPLED);
		} else
			gdk_gc_set_fill (text->gc, GDK_SOLID);
}

static void
gnome_canvas_text_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasItem *item;
	GnomeCanvasText *text;
	GdkColor color;

	item = GNOME_CANVAS_ITEM (object);
	text = GNOME_CANVAS_TEXT (object);

	switch (arg_id) {
	case ARG_TEXT:
		if (text->text)
			g_free (text->text);

		text->text = g_strdup (GTK_VALUE_STRING (*arg));
		split_into_lines (text);
		recalc_bounds (text);
		break;

	case ARG_X:
		text->x = GTK_VALUE_DOUBLE (*arg);
		recalc_bounds (text);
		break;

	case ARG_Y:
		text->y = GTK_VALUE_DOUBLE (*arg);
		recalc_bounds (text);
		break;

	case ARG_FONT:
		if (text->font)
			gdk_font_unref (text->font);

		text->font = gdk_font_load (GTK_VALUE_STRING (*arg));
		if (!text->font) {
			text->font = gdk_font_load ("fixed");
			g_assert (text->font != NULL);
		}
		if (item->canvas->aa) {
			if (text->suckfont)
				gnome_canvas_suck_font_free (text->suckfont);
			text->suckfont = gnome_canvas_suck_font (text->font);
		}

		calc_line_widths (text);
		recalc_bounds (text);
		break;

	case ARG_FONT_GDK:
		if (text->font)
			gdk_font_unref (text->font);

		text->font = GTK_VALUE_BOXED (*arg);
		gdk_font_ref (text->font);
		if (item->canvas->aa) {
			if (text->suckfont)
				gnome_canvas_suck_font_free (text->suckfont);
			text->suckfont = gnome_canvas_suck_font (text->font);
		}
		calc_line_widths (text);
		recalc_bounds (text);
		break;

	case ARG_ANCHOR:
		text->anchor = GTK_VALUE_ENUM (*arg);
		recalc_bounds (text);
		break;

	case ARG_JUSTIFICATION:
		text->justification = GTK_VALUE_ENUM (*arg);
		break;

	case ARG_CLIP_WIDTH:
		text->clip_width = fabs (GTK_VALUE_DOUBLE (*arg));
		recalc_bounds (text);
		break;

	case ARG_CLIP_HEIGHT:
		text->clip_height = fabs (GTK_VALUE_DOUBLE (*arg));
		recalc_bounds (text);
		break;

	case ARG_CLIP:
		text->clip = GTK_VALUE_BOOL (*arg);
		recalc_bounds (text);
		break;

	case ARG_X_OFFSET:
		text->xofs = GTK_VALUE_DOUBLE (*arg);
		recalc_bounds (text);
		break;

	case ARG_Y_OFFSET:
		text->yofs = GTK_VALUE_DOUBLE (*arg);
		recalc_bounds (text);
		break;

	case ARG_FILL_COLOR:
		if (gnome_canvas_get_color (item->canvas, GTK_VALUE_STRING (*arg), &color)) {
			text->pixel = color.pixel;
			if (item->canvas->aa)
				text->rgba =
					((color.red & 0xff00) << 16) |
					((color.green & 0xff00) << 8) |
					(color.blue & 0xff00) |
					0xff;
			else
				set_text_gc_foreground (text);
		} else {
			text->pixel = 0;
			if (item->canvas->aa)
				text->rgba = 0;
			else
				set_text_gc_foreground (text);
		}

		break;

	case ARG_FILL_COLOR_GDK:
		text->pixel = ((GdkColor *) GTK_VALUE_BOXED (*arg))->pixel;
		if (item->canvas->aa)
			text->rgba =
				((color.red & 0xff00) << 16) |
				((color.green & 0xff00) << 8) |
				(color.blue & 0xff00) |
				0xff;
		else
			set_text_gc_foreground (text);
		break;

	case ARG_FILL_COLOR_RGBA:
		text->rgba = GTK_VALUE_UINT (*arg);

		/* should probably request repaint on the fill_svp */
		gnome_canvas_item_request_update (item);

		break;

	case ARG_FILL_STIPPLE:
		set_stipple (text, GTK_VALUE_BOXED (*arg), FALSE);
		break;

	default:
		break;
	}
}

static void
gnome_canvas_text_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeCanvasText *text;
	GdkColor *color;

	text = GNOME_CANVAS_TEXT (object);

	switch (arg_id) {
	case ARG_TEXT:
		GTK_VALUE_STRING (*arg) = g_strdup (text->text);
		break;

	case ARG_X:
		GTK_VALUE_DOUBLE (*arg) = text->x;
		break;

	case ARG_Y:
		GTK_VALUE_DOUBLE (*arg) = text->y;
		break;

	case ARG_FONT_GDK:
		GTK_VALUE_BOXED (*arg) = text->font;
		break;

	case ARG_ANCHOR:
		GTK_VALUE_ENUM (*arg) = text->anchor;
		break;

	case ARG_JUSTIFICATION:
		GTK_VALUE_ENUM (*arg) = text->justification;
		break;

	case ARG_CLIP_WIDTH:
		GTK_VALUE_DOUBLE (*arg) = text->clip_width;
		break;

	case ARG_CLIP_HEIGHT:
		GTK_VALUE_DOUBLE (*arg) = text->clip_height;
		break;

	case ARG_CLIP:
		GTK_VALUE_BOOL (*arg) = text->clip;
		break;

	case ARG_X_OFFSET:
		GTK_VALUE_DOUBLE (*arg) = text->xofs;
		break;

	case ARG_Y_OFFSET:
		GTK_VALUE_DOUBLE (*arg) = text->yofs;
		break;

	case ARG_FILL_COLOR_GDK:
		color = g_new (GdkColor, 1);
		color->pixel = text->pixel;
		gdk_color_context_query_color (text->item.canvas->cc, color);
		GTK_VALUE_BOXED (*arg) = color;
		break;

	case ARG_FILL_COLOR_RGBA:
		GTK_VALUE_UINT (*arg) = text->rgba;
		break;

	case ARG_FILL_STIPPLE:
		GTK_VALUE_BOXED (*arg) = text->stipple;
		break;

	case ARG_TEXT_WIDTH:
		GTK_VALUE_DOUBLE (*arg) = text->max_width / text->item.canvas->pixels_per_unit;
		break;

	case ARG_TEXT_HEIGHT:
		GTK_VALUE_DOUBLE (*arg) = text->height / text->item.canvas->pixels_per_unit;
		break;

	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
gnome_canvas_text_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	GnomeCanvasText *text;
	double x1, y1, x2, y2;
	ArtDRect i_bbox, c_bbox;
	int i;

	text = GNOME_CANVAS_TEXT (item);

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	if (!item->canvas->aa) {
		set_text_gc_foreground (text);
		set_stipple (text, text->stipple, TRUE);
		get_bounds (text, &x1, &y1, &x2, &y2);

		gnome_canvas_update_bbox (item, x1, y1, x2, y2);
	} else {
		/* aa rendering */
		for (i = 0; i < 6; i++)
			text->affine[i] = affine[i];
		get_bounds_item_relative (text, &i_bbox.x0, &i_bbox.y0, &i_bbox.x1, &i_bbox.y1);
		art_drect_affine_transform (&c_bbox, &i_bbox, affine);
#ifdef VERBOSE
		g_print ("gnome_canvas_text_update (%g, %g) - (%g, %g) -> (%g, %g) - (%g, %g)\n",
			 i_bbox.x0, i_bbox.y0, i_bbox.x1, i_bbox.y1,
			 c_bbox.x0, c_bbox.y0, c_bbox.x1, c_bbox.y1);
#endif
		gnome_canvas_update_bbox (item, c_bbox.x0, c_bbox.y0, c_bbox.x1, c_bbox.y1);

	}
}

static void
gnome_canvas_text_realize (GnomeCanvasItem *item)
{
	GnomeCanvasText *text;

	text = GNOME_CANVAS_TEXT (item);

	if (parent_class->realize)
		(* parent_class->realize) (item);

	text->gc = gdk_gc_new (item->canvas->layout.bin_window);

#if 0
	(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->update) (item, NULL, NULL, 0);
#endif
}

static void
gnome_canvas_text_unrealize (GnomeCanvasItem *item)
{
	GnomeCanvasText *text;

	text = GNOME_CANVAS_TEXT (item);

	gdk_gc_unref (text->gc);

	if (parent_class->unrealize)
		(* parent_class->unrealize) (item);
}

/* Calculates the x position of the specified line of text, based on the text's justification */
static double
get_line_xpos_item_relative (GnomeCanvasText *text, struct line *line)
{
	double x;

	x = text->x;

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		x -= text->max_width / 2;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		x -= text->max_width;
		break;
	}

	switch (text->justification) {
	case GTK_JUSTIFY_RIGHT:
		x += text->max_width - line->width;
		break;

	case GTK_JUSTIFY_CENTER:
		x += (text->max_width - line->width) * 0.5;
		break;

	default:
		/* For GTK_JUSTIFY_LEFT, we don't have to do anything.  We do not support
		 * GTK_JUSTIFY_FILL, yet.
		 */
		break;
	}

	return x;
}

/* Calculates the y position of the first line of text. */
static double
get_line_ypos_item_relative (GnomeCanvasText *text)
{
	double x, y;

	y = text->y;

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		y -= text->height / 2;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		y -= text->height;
		break;
	}

	return y;
}

/* Calculates the x position of the specified line of text, based on the text's justification */
static int
get_line_xpos (GnomeCanvasText *text, struct line *line)
{
	int x;

	x = text->cx;

	switch (text->justification) {
	case GTK_JUSTIFY_RIGHT:
		x += text->max_width - line->width;
		break;

	case GTK_JUSTIFY_CENTER:
		x += (text->max_width - line->width) / 2;
		break;

	default:
		/* For GTK_JUSTIFY_LEFT, we don't have to do anything.  We do not support
		 * GTK_JUSTIFY_FILL, yet.
		 */
		break;
	}

	return x;
}

static void
gnome_canvas_text_draw (GnomeCanvasItem *item, GdkDrawable *drawable,
			int x, int y, int width, int height)
{
	GnomeCanvasText *text;
	GdkRectangle rect;
	struct line *lines;
	int i;
	int xpos, ypos;

	text = GNOME_CANVAS_TEXT (item);

	if (!text->text)
		return;

	if (text->clip) {
		rect.x = text->clip_cx - x;
		rect.y = text->clip_cy - y;
		rect.width = text->clip_cwidth;
		rect.height = text->clip_cheight;

		gdk_gc_set_clip_rectangle (text->gc, &rect);
	}

	lines = text->lines;
	ypos = text->cy + text->font->ascent;

	if (text->stipple)
		gnome_canvas_set_stipple_origin (item->canvas, text->gc);

	for (i = 0; i < text->num_lines; i++) {
		if (lines->length != 0) {
			xpos = get_line_xpos (text, lines);

			gdk_draw_text (drawable,
				       text->font,
				       text->gc,
				       xpos - x,
				       ypos - y,
				       lines->text,
				       lines->length);
		}

		ypos += text->font->ascent + text->font->descent;
		lines++;
	}

	if (text->clip)
		gdk_gc_set_clip_rectangle (text->gc, NULL);
}

#ifdef OLD_XFORM
static double
gnome_canvas_text_point (GnomeCanvasItem *item, double x, double y,
			 int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasText *text;
	int i;
	struct line *lines;
	int x1, y1, x2, y2;
	int font_height;
	int dx, dy;
	double dist, best;

	text = GNOME_CANVAS_TEXT (item);

	*actual_item = item;

	/* The idea is to build bounding rectangles for each of the lines of text (clipped by the
	 * clipping rectangle, if it is activated) and see whether the point is inside any of these.
	 * If it is, we are done.  Otherwise, calculate the distance to the nearest rectangle.
	 */

	font_height = text->font->ascent + text->font->descent;

	best = 1.0e36;

	lines = text->lines;

	for (i = 0; i < text->num_lines; i++) {
		/* Compute the coordinates of rectangle for the current line, clipping if appropriate */

		x1 = get_line_xpos (text, lines);
		y1 = text->cy + i * font_height;
		x2 = x1 + lines->width;
		y2 = y1 + font_height;

		if (text->clip) {
			if (x1 < text->clip_cx)
				x1 = text->clip_cx;

			if (y1 < text->clip_cy)
				y1 = text->clip_cy;

			if (x2 > (text->clip_cx + text->clip_width))
				x2 = text->clip_cx + text->clip_width;

			if (y2 > (text->clip_cy + text->clip_height))
				y2 = text->clip_cy + text->clip_height;

			if ((x1 >= x2) || (y1 >= y2))
				continue;
		}

		/* Calculate distance from point to rectangle */

		if (cx < x1)
			dx = x1 - cx;
		else if (cx >= x2)
			dx = cx - x2 + 1;
		else
			dx = 0;

		if (cy < y1)
			dy = y1 - cy;
		else if (cy >= y2)
			dy = cy - y2 + 1;
		else
			dy = 0;

		if ((dx == 0) && (dy == 0))
			return 0.0;

		dist = sqrt (dx * dx + dy * dy);
		if (dist < best)
			best = dist;

		/* Next! */

		lines++;
	}

	return best / item->canvas->pixels_per_unit;
}
#else
static double
gnome_canvas_text_point (GnomeCanvasItem *item, double x, double y,
			 int cx, int cy, GnomeCanvasItem **actual_item)
{
	GnomeCanvasText *text;
	int i;
	struct line *lines;
	int x1, y1, x2, y2;
	int font_height;
	int dx, dy;
	double dist, best;

	text = GNOME_CANVAS_TEXT (item);

	*actual_item = item;

	/* The idea is to build bounding rectangles for each of the lines of text (clipped by the
	 * clipping rectangle, if it is activated) and see whether the point is inside any of these.
	 * If it is, we are done.  Otherwise, calculate the distance to the nearest rectangle.
	 */

	font_height = text->font->ascent + text->font->descent;

	best = 1.0e36;

	lines = text->lines;

	for (i = 0; i < text->num_lines; i++) {
		/* Compute the coordinates of rectangle for the current line, clipping if appropriate */

		x1 = get_line_xpos (text, lines);
		y1 = text->cy + i * font_height;
		x2 = x1 + lines->width;
		y2 = y1 + font_height;

		if (text->clip) {
			if (x1 < text->clip_cx)
				x1 = text->clip_cx;

			if (y1 < text->clip_cy)
				y1 = text->clip_cy;

			if (x2 > (text->clip_cx + text->clip_width))
				x2 = text->clip_cx + text->clip_width;

			if (y2 > (text->clip_cy + text->clip_height))
				y2 = text->clip_cy + text->clip_height;

			if ((x1 >= x2) || (y1 >= y2))
				continue;
		}

		/* Calculate distance from point to rectangle */

		if (cx < x1)
			dx = x1 - cx;
		else if (cx >= x2)
			dx = cx - x2 + 1;
		else
			dx = 0;

		if (cy < y1)
			dy = y1 - cy;
		else if (cy >= y2)
			dy = cy - y2 + 1;
		else
			dy = 0;

		if ((dx == 0) && (dy == 0))
			return 0.0;

		dist = sqrt (dx * dx + dy * dy);
		if (dist < best)
			best = dist;

		/* Next! */

		lines++;
	}

	return best / item->canvas->pixels_per_unit;
}
#endif

static void
gnome_canvas_text_translate (GnomeCanvasItem *item, double dx, double dy)
{
	GnomeCanvasText *text;

	text = GNOME_CANVAS_TEXT (item);

	text->x += dx;
	text->y += dy;

	recalc_bounds (text);
}

static void
gnome_canvas_text_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	GnomeCanvasText *text;
	double width, height;

	text = GNOME_CANVAS_TEXT (item);

	*x1 = text->x;
	*y1 = text->y;

	if (text->clip) {
		width = text->clip_width;
		height = text->clip_height;
	} else {
		width = text->max_width / item->canvas->pixels_per_unit;
		height = text->height / item->canvas->pixels_per_unit;
	}

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_W:
	case GTK_ANCHOR_SW:
		break;

	case GTK_ANCHOR_N:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_S:
		*x1 -= width / 2.0;
		break;

	case GTK_ANCHOR_NE:
	case GTK_ANCHOR_E:
	case GTK_ANCHOR_SE:
		*x1 -= width;
		break;
	}

	switch (text->anchor) {
	case GTK_ANCHOR_NW:
	case GTK_ANCHOR_N:
	case GTK_ANCHOR_NE:
		break;

	case GTK_ANCHOR_W:
	case GTK_ANCHOR_CENTER:
	case GTK_ANCHOR_E:
		*y1 -= height / 2.0;
		break;

	case GTK_ANCHOR_SW:
	case GTK_ANCHOR_S:
	case GTK_ANCHOR_SE:
		*y1 -= height;
		break;
	}

	*x2 = *x1 + width;
	*y2 = *y1 + height;
}

/* Routines for sucking fonts from the X server */

static GnomeCanvasTextSuckFont *
gnome_canvas_suck_font (GdkFont *font)
{
	GnomeCanvasTextSuckFont *suckfont;
	int i;
	int x, y;
	char text[1];
	int lbearing, rbearing, ch_width, ascent, descent;
	GdkPixmap *pixmap;
	GdkColor black, white;
	GdkImage *image;
	GdkGC *gc;
	guchar *bitmap, *line;
	int width, height;
	int black_pixel, pixel;

	suckfont = g_new (GnomeCanvasTextSuckFont, 1);

	height = font->ascent + font->descent;
	x = 0;
	for (i = 0; i < 256; i++) {
		text[0] = i;
		gdk_text_extents (font, text, 1,
				  &lbearing, &rbearing, &ch_width, &ascent, &descent);
		suckfont->chars[i].left_sb = lbearing;
		suckfont->chars[i].right_sb = ch_width - rbearing;
		suckfont->chars[i].width = rbearing - lbearing;
		suckfont->chars[i].ascent = ascent;
		suckfont->chars[i].descent = descent;
		suckfont->chars[i].bitmap_offset = x;
		x += (ch_width + 31) & -32;
	}

	width = x;

	suckfont->bitmap_width = width;
	suckfont->bitmap_height = height;
	suckfont->ascent = font->ascent;

	pixmap = gdk_pixmap_new (NULL, suckfont->bitmap_width,
				 suckfont->bitmap_height, 1);
	gc = gdk_gc_new (pixmap);
	gdk_gc_set_font (gc, font);

	black_pixel = BlackPixel (gdk_display, DefaultScreen (gdk_display));
	black.pixel = black_pixel;
	white.pixel = WhitePixel (gdk_display, DefaultScreen (gdk_display));
	gdk_gc_set_foreground (gc, &white);
	gdk_draw_rectangle (pixmap, gc, 1, 0, 0, width, height);

	gdk_gc_set_foreground (gc, &black);
	for (i = 0; i < 256; i++) {
		text[0] = i;
		gdk_draw_text (pixmap, font, gc,
			       suckfont->chars[i].bitmap_offset - suckfont->chars[i].left_sb,
			       font->ascent,
			       text, 1);
	}

	/* The handling of the image leaves me with distinct unease.
	   But this is more or less copied out of
	   gimp/app/text_tool.c, so it _ought_ to work. -RLL */

	image = gdk_image_get (pixmap, 0, 0, width, height);
	suckfont->bitmap = g_malloc0 ((width >> 3) * height);

	line = suckfont->bitmap;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++) {
			pixel = gdk_image_get_pixel (image, x, y);
			if (pixel == black_pixel)
				line[x >> 3] |= 128 >> (x & 7);
		}
		line += width >> 3;
	}

	gdk_image_destroy (image);

	/* free the pixmap */
	gdk_pixmap_unref (pixmap);

	/* free the gc */
	gdk_gc_destroy (gc);

	return suckfont;
}

static void
gnome_canvas_suck_font_free (GnomeCanvasTextSuckFont *suckfont) {
	g_free (suckfont->bitmap);
	g_free (suckfont);
}

static void
gnome_canvas_text_render      (GnomeCanvasItem *item, GnomeCanvasBuf *buf)
{
	GnomeCanvasText *text;
	guint32 fg_color;
	double x_start, y_start;
	double xpos, ypos;
	struct line *lines;
	int i, j;
	double affine[6];
	GnomeCanvasTextSuckFont *suckfont;
	int dx, dy;
	ArtPoint start_i, start_c;

	text = GNOME_CANVAS_TEXT (item);
	if (!text->text)
		return;

	suckfont = text->suckfont;

	fg_color = text->rgba;

        gnome_canvas_buf_ensure_buf (buf);

	lines = text->lines;
	start_i.y = get_line_ypos_item_relative (text);

	art_affine_scale (affine, item->canvas->pixels_per_unit, item->canvas->pixels_per_unit);
	for (i = 0; i < 6; i++)
		affine[i] = text->affine[i];

#ifdef VERBOSE
	{
		char str[128];
		art_affine_to_string (str, affine);
		g_print ("gnome_canvas_text_render %s\n", str);
	}
#endif
	for (i = 0; i < text->num_lines; i++) {
		if (lines->length != 0) {
			start_i.x = get_line_xpos_item_relative (text, lines);
			art_affine_point (&start_c, &start_i, text->affine);
			xpos = start_c.x;
			ypos = start_c.y;

			for (j = 0; j < lines->length; j++) {
				GnomeCanvasTextSuckChar *ch;

				ch = &suckfont->chars[(unsigned char)((lines->text)[j])];
				affine[4] = xpos;
				affine[5] = ypos;
				art_rgb_bitmap_affine (buf->buf,
						       buf->rect.x0, buf->rect.y0, buf->rect.x1, buf->rect.y1,
						       buf->buf_rowstride,
						       suckfont->bitmap + (ch->bitmap_offset >> 3),
						       ch->width,
						       suckfont->bitmap_height,
						       suckfont->bitmap_width >> 3,
						       fg_color,
						       affine,
						       ART_FILTER_NEAREST, NULL);
				dx = ch->left_sb + ch->width + ch->right_sb;
				xpos += dx * affine[0];
				ypos += dx * affine[1];
			}
		}

		dy = text->font->ascent + text->font->descent;
		start_i.y += dy;
		lines++;
	}

	buf->is_bg = 0;
}
