/* gnome-icon-text-item:  an editable text block with word wrapping for the
 * GNOME canvas.
 *
 * Copyright (C) 1998, 1999 The Free Software Foundation
 *
 * Authors: Miguel de Icaza <miguel@gnu.org>
 *          Federico Mena <federico@gimp.org>
 *
 * FIXME: Provide a ref-count fontname caching like thing.
 */

#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtkmain.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkwindow.h>
#include "gnome-icon-item.h"
#include "libgnomeui/gnome-window-icon.h"

/* Margins used to display the information */
#define MARGIN_X 2
#define MARGIN_Y 2

/* Default fontset to be used if the user specified fontset is not found */
#define DEFAULT_FONT_NAME "-adobe-helvetica-medium-r-normal--*-100-*-*-*-*-*-*,"	\
			  "-*-*-medium-r-normal--10-*-*-*-*-*-*-*,*"

/* Separators for text layout */
#define DEFAULT_SEPARATORS " \t-.[]#"

/* Aliases to minimize screen use in my laptop */
#define ITI(x)       GNOME_ICON_TEXT_ITEM (x)
#define ITI_CLASS(x) GNOME_ICON_TEXT_ITEM_CLASS (x)
#define IS_ITI(x)    GNOME_IS_ICON_TEXT_ITEM (x)


typedef GnomeIconTextItem Iti;

/* Private part of the GnomeIconTextItem structure */
typedef struct {
	/* Font */
	GdkFont *font;

	/* Hack: create an offscreen window and place an entry inside it */
	GtkEntry *entry;
	GtkWidget *entry_top;

	/* Whether the user pressed the mouse while the item was unselected */
	guint unselected_click : 1;

	/* Whether we need to update the position */
	guint need_pos_update : 1;

	/* Whether we need to update the font */
	guint need_font_update : 1;

	/* Whether we need to update the text */
	guint need_text_update : 1;

	/* Whether we need to update because the editing/selected state changed */
	guint need_state_update : 1;

	/* For compatibility with old clients */
	guint use_broken_event_handling : 1;
} ItiPrivate;


static GnomeCanvasItemClass *parent_class;

enum {
	ARG_0,
	ARG_USE_BROKEN_EVENT_HANDLING
};

enum {
	TEXT_CHANGED,
	HEIGHT_CHANGED,
	WIDTH_CHANGED,
	EDITING_STARTED,
	EDITING_STOPPED,
	SELECTION_STARTED,
	SELECTION_STOPPED,
	LAST_SIGNAL
};

static guint iti_signals [LAST_SIGNAL] = { 0 };

static GdkFont *default_font;


/* Stops the editing state of an icon text item */
static void
iti_stop_editing (Iti *iti)
{
	ItiPrivate *priv;

	priv = iti->priv;

	iti->editing = FALSE;

	gtk_widget_destroy (priv->entry_top);
	priv->entry = NULL;
	priv->entry_top = NULL;

	priv->need_state_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));

	gtk_signal_emit (GTK_OBJECT (iti), iti_signals[EDITING_STOPPED]);
}

/* Lays out the text in an icon item */
static void
layout_text (Iti *iti)
{
	ItiPrivate *priv;
	char *text;
	int old_width, old_height;
	int width, height;

	priv = iti->priv;

	/* Save old size */

	if (iti->ti) {
		old_width = iti->ti->width + 2 * MARGIN_X;
		old_height = iti->ti->height + 2 * MARGIN_Y;

		gnome_icon_text_info_free (iti->ti);
	} else {
		old_width = 2 * MARGIN_X;
		old_height = 2 * MARGIN_Y;
	}

	/* Change the text layout */

	if (iti->editing)
		text = gtk_entry_get_text (priv->entry);
	else
		text = iti->text;

	iti->ti = gnome_icon_layout_text (priv->font,
					  text,
					  DEFAULT_SEPARATORS,
					  iti->width - 2 * MARGIN_X,
					  TRUE);

	/* Check the sizes and see if we need to emit any signals */

	width = iti->ti->width + 2 * MARGIN_X;
	height = iti->ti->height + 2 * MARGIN_Y;

	if (width != old_width)
		gtk_signal_emit (GTK_OBJECT (iti), iti_signals[WIDTH_CHANGED]);

	if (height != old_height)
		gtk_signal_emit (GTK_OBJECT (iti), iti_signals[HEIGHT_CHANGED]);
}

/* Accepts the text in the off-screen entry of an icon text item */
static void
iti_edition_accept (Iti *iti)
{
	ItiPrivate *priv;
	gboolean accept;

	priv = iti->priv;
	accept = TRUE;

	gtk_signal_emit (GTK_OBJECT (iti), iti_signals [TEXT_CHANGED], &accept);

	if (iti->editing){
		if (accept) {
			if (iti->is_text_allocated)
				g_free (iti->text);

			iti->text = g_strdup (gtk_entry_get_text (priv->entry));
			iti->is_text_allocated = 1;
		}

		iti_stop_editing (iti);
	}
	layout_text (iti);

	priv->need_text_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}

/* Callback used when the off-screen entry of an icon text item is activated.
 * When this happens, we have to accept edition.
 */
static void
iti_entry_activate (GtkWidget *entry, Iti *iti)
{
	iti_edition_accept (iti);
}

/* Starts the editing state of an icon text item */
static void
iti_start_editing (Iti *iti)
{
	ItiPrivate *priv;

	priv = iti->priv;

	if (iti->editing)
		return;

	/* Trick: The actual edition of the entry takes place in a GtkEntry
	 * which is placed offscreen.  That way we get all of the advantages
	 * from GtkEntry without duplicating code.  Yes, this is a hack.
	 */
	priv->entry = (GtkEntry *) gtk_entry_new ();
	gtk_entry_set_text (priv->entry, iti->text);
	gtk_signal_connect (GTK_OBJECT (priv->entry), "activate",
			    GTK_SIGNAL_FUNC (iti_entry_activate), iti);

	priv->entry_top = gtk_window_new (GTK_WINDOW_POPUP);
	gnome_window_icon_set_from_default (GTK_WINDOW (priv->entry_top));
	gtk_container_add (GTK_CONTAINER (priv->entry_top), GTK_WIDGET (priv->entry));
	gtk_widget_set_uposition (priv->entry_top, 20000, 20000);
	gtk_widget_show_all (priv->entry_top);

	gtk_editable_select_region (GTK_EDITABLE (priv->entry), 0, -1);

	iti->editing = TRUE;

	priv->need_state_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));

	gtk_signal_emit (GTK_OBJECT (iti), iti_signals[EDITING_STARTED]);
}

/* Destroy method handler for the icon text item */
static void
iti_destroy (GtkObject *object)
{
	Iti *iti;
	ItiPrivate *priv;
	GnomeCanvasItem *item;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_ITI (object));

	iti = ITI (object);
	priv = iti->priv;
	item = GNOME_CANVAS_ITEM (object);

	/* FIXME: stop selection and editing */

	/* Queue redraw of bounding box */

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	/* Free everything */

	if (iti->fontname)
		g_free (iti->fontname);

	if (iti->text && iti->is_text_allocated)
		g_free (iti->text);

	if (iti->ti)
		gnome_icon_text_info_free (iti->ti);

	if (priv->font)
		gdk_font_unref (priv->font);

	if (priv->entry_top)
		gtk_widget_destroy (priv->entry_top);

	g_free (priv);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/* set_arg handler for the icon text item */
static void
iti_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	Iti *iti;
	ItiPrivate *priv;

	iti = ITI (object);
	priv = iti->priv;

	switch (arg_id) {
	case ARG_USE_BROKEN_EVENT_HANDLING:
		priv->use_broken_event_handling = GTK_VALUE_BOOL (*arg) ? TRUE : FALSE;
		break;

	default:
		break;
	}
}

/* Loads the default font for icon text items if necessary */
static GdkFont *
get_default_font (void)
{
	if (!default_font) {
		/* FIXME: this is never unref-ed */
		default_font = gdk_fontset_load (DEFAULT_FONT_NAME);
		g_assert (default_font != NULL);
	}

	return gdk_font_ref (default_font);
}

/* Recomputes the bounding box of an icon text item */
static void
recompute_bounding_box (Iti *iti)
{
	GnomeCanvasItem *item;
	double affine[6];
	ArtPoint p, q;
	int x1, y1, x2, y2;
	int width, height;

	item = GNOME_CANVAS_ITEM (iti);

	/* Compute width, height, position */

	width = iti->ti->width + 2 * MARGIN_X;
	height = iti->ti->height + 2 * MARGIN_Y;

	x1 = iti->x + (iti->width - width) / 2;
	y1 = iti->y;
	x2 = x1 + width;
	y2 = y1 + height;

	/* Translate to world coordinates */

	gnome_canvas_item_i2w_affine (item, affine);

	p.x = x1;
	p.y = y1;
	art_affine_point (&q, &p, affine);
	item->x1 = q.x;
	item->y1 = q.y;

	p.x = x2;
	p.y = y2;
	art_affine_point (&q, &p, affine);
	item->x2 = q.x;
	item->y2 = q.y;
}

/* Update method for the icon text item */
static void
iti_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	Iti *iti;
	ItiPrivate *priv;

	iti = ITI (item);
	priv = iti->priv;

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);

	/* If necessary, queue a redraw of the old bounding box */

	if ((flags & GNOME_CANVAS_UPDATE_VISIBILITY)
	    || (flags & GNOME_CANVAS_UPDATE_AFFINE)
	    || priv->need_pos_update
	    || priv->need_font_update
	    || priv->need_text_update)
		gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	/* Compute new bounds */

	if (priv->need_pos_update
	    || priv->need_font_update
	    || priv->need_text_update)
		recompute_bounding_box (iti);

	/* Queue redraw */

	gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	priv->need_pos_update = FALSE;
	priv->need_font_update = FALSE;
	priv->need_text_update = FALSE;
	priv->need_state_update = FALSE;
}

/* Draw the icon text item's text when it is being edited */
static void
iti_paint_text (Iti *iti, GdkDrawable *drawable, int x, int y)
{
	ItiPrivate *priv;
        GnomeIconTextInfoRow *row;
	GnomeIconTextInfo *ti;
	GtkStyle *style;
	GdkGC *fg_gc, *bg_gc;
	GdkGC *gc, *bgc, *sgc, *bsgc;
        GList *item;
        int xpos, len;

	priv = iti->priv;
	style = GTK_WIDGET (GNOME_CANVAS_ITEM (iti)->canvas)->style;

	ti = iti->ti;
	len = 0;
        y += ti->font->ascent;

	/*
	 * Pointers to all of the GCs we use
	 */
	gc = style->fg_gc [GTK_STATE_NORMAL];
	bgc = style->bg_gc [GTK_STATE_NORMAL];
	sgc = style->fg_gc [GTK_STATE_SELECTED];
	bsgc = style->bg_gc [GTK_STATE_SELECTED];

        for (item = ti->rows; item; item = item->next, len += (row ? row->text_length : 0)) {
		GdkWChar *text_wc;
		int text_length;
		int cursor, offset, i;
		int sel_start, sel_end;

		row = item->data;

                if (!row) {
			y += ti->baseline_skip / 2;
			continue;
		}

		text_wc = row->text_wc;
		text_length = row->text_length;

		xpos = (ti->width - row->width) / 2;

		sel_start = GTK_EDITABLE (priv->entry)->selection_start_pos - len;
		sel_end = GTK_EDITABLE (priv->entry)->selection_end_pos - len;
		offset = 0;
		cursor = GTK_EDITABLE (priv->entry)->current_pos - len;

		for (i = 0; *text_wc; text_wc++, i++) {
			int size, px;

			size = gdk_text_width_wc (ti->font, text_wc, 1);

			if (i >= sel_start && i < sel_end) {
				fg_gc = sgc;
				bg_gc = bsgc;
			} else {
				fg_gc = gc;
				bg_gc = bgc;
			}

			px = x + xpos + offset;
			gdk_draw_rectangle (drawable,
					    bg_gc,
					    TRUE,
					    px,
					    y - ti->font->ascent,
					    size, ti->baseline_skip);

			gdk_draw_text_wc (drawable,
					  ti->font,
					  fg_gc,
					  px, y,
					  text_wc, 1);

			if (cursor == i)
				gdk_draw_line (drawable,
					       gc,
					       px - 1,
					       y - ti->font->ascent,
					       px - 1,
					       y + ti->font->descent - 1);

			offset += size;
		}

		if (cursor == i) {
			int px = x + xpos + offset;

			gdk_draw_line (drawable,
				       gc,
				       px - 1,
				       y - ti->font->ascent,
				       px - 1,
				       y + ti->font->descent - 1);
		}

		y += ti->baseline_skip;
        }
}

/* Draw method handler for the icon text item */
static void
iti_draw (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y, int width, int height)
{
	Iti *iti;
	GtkStyle *style;
	int w, h;
	int xofs, yofs;

	iti = ITI (item);

	if (iti->ti) {
		w = iti->ti->width + 2 * MARGIN_X;
		h = iti->ti->height + 2 * MARGIN_Y;
	} else {
		w = 2 * MARGIN_X;
		h = 2 * MARGIN_Y;
	}

	xofs = item->x1 - x;
	yofs = item->y1 - y;

	style = GTK_WIDGET (item->canvas)->style;

	if (iti->selected && !iti->editing)
		gdk_draw_rectangle (drawable,
				    style->bg_gc[GTK_STATE_SELECTED],
				    TRUE,
				    xofs, yofs,
				    w, h);

	if (iti->editing) {
		gdk_draw_rectangle (drawable,
				    style->fg_gc[GTK_STATE_NORMAL],
				    FALSE,
				    xofs, yofs,
				    w - 1, h - 1);

		iti_paint_text (iti, drawable, xofs + MARGIN_X, yofs + MARGIN_Y);
	} else
		gnome_icon_paint_text (iti->ti,
				       drawable,
				       style->fg_gc[(iti->selected
						     ? GTK_STATE_SELECTED
						     : GTK_STATE_NORMAL)],
				       xofs + MARGIN_X,
				       yofs + MARGIN_Y,
				       GTK_JUSTIFY_CENTER);
}

/* Point method handler for the icon text item */
static double
iti_point (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item)
{
	double dx, dy;

	*actual_item = item;

	if (cx < item->x1)
		dx = item->x1 - cx;
	else if (cx > item->x2)
		dx = cx - item->x2;
	else
		dx = 0.0;

	if (cy < item->y1)
		dy = item->y1 - cy;
	else if (cy > item->y2)
		dy = cy - item->y2;
	else
		dy = 0.0;

	return sqrt (dx * dx + dy * dy);
}

/* Given X, Y, a mouse position, return a valid index inside the edited text */
static int
iti_idx_from_x_y (Iti *iti, int x, int y)
{
	ItiPrivate *priv;
        GnomeIconTextInfoRow *row;
	int lines;
	int line, col, i, idx;
	GList *l;

	priv = iti->priv;

	if (iti->ti->rows == NULL)
		return 0;

	lines = g_list_length (iti->ti->rows);
	line = y / iti->ti->baseline_skip;

	if (line < 0)
		line = 0;
	else if (lines < line + 1)
		line = lines - 1;

	/* Compute the base index for this line */
	for (l = iti->ti->rows, idx = i = 0; i < line; l = l->next, i++) {
		row = l->data;
		idx += row->text_length;
	}

	row = g_list_nth (iti->ti->rows, line)->data;
	col = 0;
	if (row != NULL) {
		int first_char;
		int last_char;

		first_char = (iti->ti->width - row->width) / 2;
		last_char = first_char + row->width;

		if (x < first_char) {
			/* nothing */
		} else if (x > last_char) {
			col = row->text_length;
		} else {
			GdkWChar *s = row->text_wc;
			int pos = first_char;

			while (pos < last_char) {
				pos += gdk_text_width_wc (iti->ti->font, s, 1);
				if (pos > x)
					break;
				col++;
				s++;
			}
		}
	}

	idx += col;

	g_assert (idx <= priv->entry->text_size);

	return idx;
}

/* Starts the selection state in the icon text item */
static void
iti_start_selecting (Iti *iti, int idx, guint32 event_time)
{
	ItiPrivate *priv;
	GtkEditable *e;
	GdkCursor *ibeam;

	priv = iti->priv;
	e = GTK_EDITABLE (priv->entry);

	gtk_editable_select_region (e, idx, idx);
	gtk_editable_set_position (e, idx);
	ibeam = gdk_cursor_new (GDK_XTERM);
	gnome_canvas_item_grab (GNOME_CANVAS_ITEM (iti),
				GDK_BUTTON_RELEASE_MASK |
				GDK_POINTER_MOTION_MASK,
				ibeam, event_time);
	gdk_cursor_destroy (ibeam);

	gtk_editable_select_region (e, idx, idx);
	e->current_pos = e->selection_start_pos;
	e->has_selection = TRUE;
	iti->selecting = TRUE;

	priv->need_state_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));

	gtk_signal_emit (GTK_OBJECT (iti), iti_signals[SELECTION_STARTED]);
}

/* Stops the selection state in the icon text item */
static void
iti_stop_selecting (Iti *iti, guint32 event_time)
{
	ItiPrivate *priv;
	GnomeCanvasItem *item;
	GtkEditable *e;

	priv = iti->priv;
	item = GNOME_CANVAS_ITEM (iti);
	e = GTK_EDITABLE (priv->entry);

	gnome_canvas_item_ungrab (item, event_time);
	e->has_selection = FALSE;
	iti->selecting = FALSE;

	priv->need_state_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
	gtk_signal_emit (GTK_OBJECT (iti), iti_signals[SELECTION_STOPPED]);
}

/* Handles selection range changes on the icon text item */
static void
iti_selection_motion (Iti *iti, int idx)
{
	ItiPrivate *priv;
	GtkEditable *e;

	priv = iti->priv;
	e = GTK_EDITABLE (priv->entry);

	if (idx < e->current_pos) {
		e->selection_start_pos = idx;
		e->selection_end_pos   = e->current_pos;
	} else {
		e->selection_start_pos = e->current_pos;
		e->selection_end_pos  = idx;
	}

	priv->need_state_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}

/* Event handler for icon text items */
static gint
iti_event (GnomeCanvasItem *item, GdkEvent *event)
{
	Iti *iti;
	ItiPrivate *priv;
	int idx;
	double x, y;

	iti = ITI (item);
	priv = iti->priv;

	switch (event->type) {
	case GDK_KEY_PRESS:
		if (!iti->editing)
			break;

		if (event->key.keyval == GDK_Escape)
			iti_stop_editing (iti);
		else
			gtk_widget_event (GTK_WIDGET (priv->entry), event);

		layout_text (iti);
		priv->need_text_update = TRUE;
		gnome_canvas_item_request_update (item);
		return TRUE;

	case GDK_BUTTON_PRESS:
		if (priv->use_broken_event_handling) {
			if (!iti->is_editable)
				break;

			if (event->button.button != 1)
				break;

			if (!iti->selected) {
				priv->unselected_click = TRUE;
				break;
			} else
				priv->unselected_click = FALSE;
		} else if (!iti->editing)
			break;

		if (iti->editing && event->button.button == 1) {
			x = event->button.x - (item->x1 + MARGIN_X);
			y = event->button.y - (item->y1 + MARGIN_Y);
			idx = iti_idx_from_x_y (iti, x, y);

			iti_start_selecting (iti, idx, event->button.time);
		} else if (!priv->use_broken_event_handling)
			break;

		return TRUE;

	case GDK_MOTION_NOTIFY:
		if (!iti->selecting)
			break;

		x = event->motion.x - (item->x1 + MARGIN_X);
		y = event->motion.y - (item->y1 + MARGIN_Y);
		idx = iti_idx_from_x_y (iti, x, y);
		iti_selection_motion (iti, idx);
		return TRUE;

	case GDK_BUTTON_RELEASE:
		if (priv->use_broken_event_handling) {
			if (!iti->is_editable)
				break;

			if (!priv->entry) {
				if (priv->unselected_click || iti->editing)
					break;

				if (event->button.button != 1)
					break;

				gnome_canvas_item_grab_focus (item);
				iti_start_editing (iti);
			} else
				iti_stop_selecting (iti, event->button.time);
		} else {
			if (iti->selecting && event->button.button == 1)
				iti_stop_selecting (iti, event->button.time);
			else
				break;
		}

		return TRUE;

	case GDK_FOCUS_CHANGE:
		if (iti->editing && event->focus_change.in == FALSE)
			iti_edition_accept (iti);

		return TRUE;

	default:
		break;
	}

	return FALSE;
}

/* Bounds method handler for the icon text item */
static void
iti_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	Iti *iti;
	int width, height;

	iti = ITI (item);

	if (iti->ti) {
		width = iti->ti->width + 2 * MARGIN_X;
		height = iti->ti->height + 2 * MARGIN_Y;
	} else {
		width = 2 * MARGIN_X;
		height = 2 * MARGIN_Y;
	}

	*x1 = iti->x + (iti->width - width) / 2;
	*y1 = iti->y;
	*x2 = *x1 + width;
	*y2 = *y1 + height;
}

/* Class initialization function for the icon text item */
static void
iti_class_init (GnomeIconTextItemClass *text_item_class)
{
	GtkObjectClass  *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GtkObjectClass *) text_item_class;
	item_class   = (GnomeCanvasItemClass *) text_item_class;

	parent_class = gtk_type_class (gnome_canvas_item_get_type ());

	gtk_object_add_arg_type ("GnomeIconTextItem::use_broken_event_handling",
				 GTK_TYPE_BOOL, GTK_ARG_WRITABLE, ARG_USE_BROKEN_EVENT_HANDLING);

	iti_signals [TEXT_CHANGED] =
		gtk_signal_new (
			"text_changed",
			GTK_RUN_LAST,
			object_class->type,
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, text_changed),
			gtk_marshal_BOOL__NONE,
			GTK_TYPE_BOOL, 0);

	iti_signals [HEIGHT_CHANGED] =
		gtk_signal_new (
			"height_changed",
			GTK_RUN_LAST,
			object_class->type,
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, height_changed),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE, 0);

	iti_signals [WIDTH_CHANGED] =
		gtk_signal_new (
			"width_changed",
			GTK_RUN_LAST,
			object_class->type,
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, width_changed),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE, 0);

	iti_signals[EDITING_STARTED] =
		gtk_signal_new (
			"editing_started",
			GTK_RUN_LAST,
			object_class->type,
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, editing_started),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE, 0);

	iti_signals[EDITING_STOPPED] =
		gtk_signal_new (
			"editing_stopped",
			GTK_RUN_LAST,
			object_class->type,
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, editing_stopped),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE, 0);

	iti_signals[SELECTION_STARTED] =
		gtk_signal_new (
			"selection_started",
			GTK_RUN_FIRST,
			object_class->type,
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, selection_started),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE, 0);

	iti_signals[SELECTION_STOPPED] =
		gtk_signal_new (
			"selection_stopped",
			GTK_RUN_FIRST,
			object_class->type,
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, selection_stopped),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE, 0);

	gtk_object_class_add_signals (object_class, iti_signals, LAST_SIGNAL);

	object_class->destroy = iti_destroy;
	object_class->set_arg = iti_set_arg;

	item_class->update = iti_update;
	item_class->draw = iti_draw;
	item_class->point = iti_point;
	item_class->bounds = iti_bounds;
	item_class->event = iti_event;
}

/* Object initialization function for the icon text item */
static void
iti_init (GnomeIconTextItem *iti)
{
	ItiPrivate *priv;

	priv = g_new0 (ItiPrivate, 1);
	iti->priv = priv;

	/* For compatibility with old clients */
	priv->use_broken_event_handling = TRUE;
}

/**
 * gnome_icon_text_item_configure:
 * @iti: An icon text item.
 * @x: X position in which to place the item.
 * @y: Y position in which to place the item.
 * @width: Maximum width allowed for this item, to be used for word wrapping.
 * @fontname: Name of the fontset that should be used to display the text.
 * @text: Text that is going to be displayed.
 * @is_editable: Deprecated.
 * @is_static: Whether @text points to a static string or not.
 *
 * This routine is used to configure a &GnomeIconTextItem.
 *
 * @x and @y specify the cordinates where the item is placed inside the canvas.
 * The @x coordinate should be the leftmost position that the icon text item can
 * assume at any one time, that is, the left margin of the column in which the
 * icon is to be placed.  The @y coordinate specifies the top of the icon text
 * item.
 *
 * @width is the maximum width allowed for this icon text item.  The coordinates
 * define the upper-left corner of an icon text item with maximum width; this may
 * actually be outside the bounding box of the item if the text is narrower than
 * the maximum width.
 *
 * If @is_static is true, it means that there is no need for the item to
 * allocate memory for the string (it is a guarantee that the text is allocated
 * by the caller and it will not be deallocated during the lifetime of this
 * item).  This is an optimization to reduce memory usage for large icon sets.
 */
void
gnome_icon_text_item_configure (GnomeIconTextItem *iti, int x, int y,
				int width, const char *fontname,
				const char *text,
				gboolean is_editable, gboolean is_static)
{
	ItiPrivate *priv;

	g_return_if_fail (iti != NULL);
	g_return_if_fail (IS_ITI (iti));
	g_return_if_fail (width > 2 * MARGIN_X);
	g_return_if_fail (text != NULL);

	priv = iti->priv;

	iti->x = x;
	iti->y = y;
	iti->width = width;
	iti->is_editable = is_editable != FALSE;

	if (iti->text && iti->is_text_allocated)
		g_free (iti->text);

	iti->is_text_allocated = !is_static;

	/* This cast is to shut up the compiler */
	if (is_static)
		iti->text = (char *) text;
	else
		iti->text = g_strdup (text);

	if (iti->fontname)
		g_free (iti->fontname);

	iti->fontname = g_strdup (fontname ? fontname : DEFAULT_FONT_NAME);

	/* FIXME: We update the font and layout here instead of in the
	 * ::update() method because the stupid icon list makes use of iti->ti
	 * and expects it to be valid at all times.  It should request the
	 * item's bounds instead.
	 */

	if (priv->font)
		gdk_font_unref (priv->font);

	priv->font = NULL;
	if (fontname)
		priv->font = gdk_fontset_load (iti->fontname);
	if (!priv->font)
		priv->font = get_default_font ();

	layout_text (iti);

	/* Request update */

	priv->need_pos_update = TRUE;
	priv->need_font_update = TRUE;
	priv->need_text_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}

/**
 * gnome_icon_text_item_setxy:
 * @iti:  An icon text item.
 * @x: X position.
 * @y: Y position.
 *
 * Sets the coordinates at which the icon text item should be placed.
 *
 * See also: gnome_icon_text_item_configure().
 */
void
gnome_icon_text_item_setxy (GnomeIconTextItem *iti, int x, int y)
{
	ItiPrivate *priv;

	g_return_if_fail (iti != NULL);
	g_return_if_fail (IS_ITI (iti));

	priv = iti->priv;

	iti->x = x;
	iti->y = y;

	priv->need_pos_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}

/**
 * gnome_icon_text_item_select:
 * @iti: An icon text item
 * @sel: Whether the icon text item should be displayed as selected.
 *
 * This function is used to control whether an icon text item is displayed as
 * selected or not.  Mouse events are ignored by the item when it is unselected;
 * when the user clicks on a selected icon text item, it will start the text
 * editing process.
 */
void
gnome_icon_text_item_select (GnomeIconTextItem *iti, int sel)
{
	ItiPrivate *priv;

	g_return_if_fail (iti != NULL);
	g_return_if_fail (IS_ITI (iti));

	priv = iti->priv;

	if (!iti->selected == !sel)
		return;

	iti->selected = sel ? TRUE : FALSE;

	if (!iti->selected && iti->editing)
		iti_edition_accept (iti);

	priv->need_state_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}

/**
 * gnome_icon_text_item_get_text:
 * @iti: An icon text item.
 *
 * Returns the current text string in an icon text item.  The client should not
 * free this string, as it is internal to the icon text item.
 */
char *
gnome_icon_text_item_get_text (GnomeIconTextItem *iti)
{
	ItiPrivate *priv;

	g_return_val_if_fail (iti != NULL, NULL);
	g_return_val_if_fail (IS_ITI (iti), NULL);

	priv = iti->priv;

	if (iti->editing)
		return gtk_entry_get_text (priv->entry);
	else
		return iti->text;
}


/**
 * gnome_icon_text_item_start_editing:
 * @iti: An icon text item.
 *
 * Starts the editing state of an icon text item.
 **/
void
gnome_icon_text_item_start_editing (GnomeIconTextItem *iti)
{
	g_return_if_fail (iti != NULL);
	g_return_if_fail (IS_ITI (iti));

	if (iti->editing)
		return;

	iti->selected = TRUE; /* Ensure that we are selected */
	gnome_canvas_item_grab_focus (GNOME_CANVAS_ITEM (iti));
	iti_start_editing (iti);
}

/**
 * gnome_icon_text_item_stop_editing:
 * @iti: An icon text item.
 * @accept: Whether to accept the current text or to discard it.
 *
 * Terminates the editing state of an icon text item.  The @accept argument
 * controls whether the item's current text should be accepted or discarded.  If
 * it is discarded, then the icon's original text will be restored.
 **/
void
gnome_icon_text_item_stop_editing (GnomeIconTextItem *iti,
				   gboolean accept)
{
	g_return_if_fail (iti != NULL);
	g_return_if_fail (IS_ITI (iti));

	if (!iti->editing)
		return;

	if (accept)
		iti_edition_accept (iti);
	else
		iti_stop_editing (iti);
}


/**
 * gnome_icon_text_item_get_type:
 * @void:
 *
 * Registers the &GnomeIconTextItem class if necessary, and returns the type ID
 * associated to it.
 *
 * Return value: the type ID of the &GnomeIconTextItem class.
 **/
GtkType
gnome_icon_text_item_get_type (void)
{
	static GtkType iti_type = 0;

	if (!iti_type) {
		static const GtkTypeInfo iti_info = {
			"GnomeIconTextItem",
			sizeof (GnomeIconTextItem),
			sizeof (GnomeIconTextItemClass),
			(GtkClassInitFunc) iti_class_init,
			(GtkObjectInitFunc) iti_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		iti_type = gtk_type_unique (gnome_canvas_item_get_type (), &iti_info);
	}

	return iti_type;
}
