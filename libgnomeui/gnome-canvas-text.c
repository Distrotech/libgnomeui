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
	ARG_FILL_COLOR_GDK
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

static void   gnome_canvas_text_reconfigure (GnomeCanvasItem *item);
static void   gnome_canvas_text_realize     (GnomeCanvasItem *item);
static void   gnome_canvas_text_unrealize   (GnomeCanvasItem *item);
static void   gnome_canvas_text_draw        (GnomeCanvasItem *item, GdkDrawable *drawable,
					     int x, int y, int width, int height);
static double gnome_canvas_text_point       (GnomeCanvasItem *item, double x, double y,
					     int cx, int cy, GnomeCanvasItem **actual_item);
static void   gnome_canvas_text_translate   (GnomeCanvasItem *item, double dx, double dy);


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

	object_class->destroy = gnome_canvas_text_destroy;
	object_class->set_arg = gnome_canvas_text_set_arg;
	object_class->get_arg = gnome_canvas_text_get_arg;

	item_class->reconfigure = gnome_canvas_text_reconfigure;
	item_class->realize = gnome_canvas_text_realize;
	item_class->unrealize = gnome_canvas_text_unrealize;
	item_class->draw = gnome_canvas_text_draw;
	item_class->point = gnome_canvas_text_point;
	item_class->translate = gnome_canvas_text_translate;
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

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/* Recalculates the bounding box of the text item.  The bounding box is defined by the text's
 * extents if the clip rectangle is disabled.  If it is enabled, the bounding box is defined by the
 * clip rectangle itself.
 */
static void
recalc_bounds (GnomeCanvasText *text)
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
		item->x1 = text->clip_cx;
		item->y1 = text->clip_cy;
		item->x2 = text->clip_cx + text->clip_cwidth;
		item->y2 = text->clip_cy + text->clip_cheight;
	} else {
		item->x1 = text->cx;
		item->y1 = text->cy;
		item->x2 = text->cx + text->max_width;
		item->y2 = text->cy + text->height;
	}

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

		calc_line_widths (text);
		recalc_bounds (text);
		break;

	case ARG_FONT_GDK:
		if (text->font)
			gdk_font_unref (text->font);

		text->font = GTK_VALUE_BOXED (*arg);
		gdk_font_ref (text->font);
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
		if (gnome_canvas_get_color (item->canvas, GTK_VALUE_STRING (*arg), &color))
			text->pixel = color.pixel;
		else
			text->pixel = 0;

		set_text_gc_foreground (text);
		break;

	case ARG_FILL_COLOR_GDK:
		text->pixel = ((GdkColor *) GTK_VALUE_BOXED (*arg))->pixel;
		set_text_gc_foreground (text);
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

	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
gnome_canvas_text_reconfigure (GnomeCanvasItem *item)
{
	GnomeCanvasText *text;
	GdkColor color;

	text = GNOME_CANVAS_TEXT (item);

	if (parent_class->reconfigure)
		(* parent_class->reconfigure) (item);

	set_text_gc_foreground (text);

	recalc_bounds (text);
}

static void
gnome_canvas_text_realize (GnomeCanvasItem *item)
{
	GnomeCanvasText *text;

	text = GNOME_CANVAS_TEXT (item);

	if (parent_class->realize)
		(* parent_class->realize) (item);

	text->gc = gdk_gc_new (item->canvas->layout.bin_window);

	(* GNOME_CANVAS_ITEM_CLASS (item->object.klass)->reconfigure) (item);
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

	for (i = 0; i < text->num_lines; i++) {
		xpos = get_line_xpos (text, lines);

		gdk_draw_text (drawable,
			       text->font,
			       text->gc,
			       xpos - x,
			       ypos - y,
			       lines->text,
			       lines->length);

		ypos += text->font->ascent + text->font->descent;
		lines++;
	}

	if (text->clip)
		gdk_gc_set_clip_rectangle (text->gc, NULL);
}

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

static void
gnome_canvas_text_translate (GnomeCanvasItem *item, double dx, double dy)
{
	GnomeCanvasText *text;

	text = GNOME_CANVAS_TEXT (item);

	text->x += dx;
	text->y += dy;

	recalc_bounds (text);
}
