/*
 * Copyright (C) 1998, 1999, 2000 Free Software Foundation
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

/* gnome-icon-text-item:  an editable text block with word wrapping for the
 * GNOME canvas.
 *
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
#include "gnome-cursors.h"

/* Margins used to display the information */
#define MARGIN_X 2
#define MARGIN_Y 2

/* Separators for text layout */
#define DEFAULT_SEPARATORS " \t-.[]#"

/* Aliases to minimize screen use in my laptop */
#define ITI(x)       GNOME_ICON_TEXT_ITEM (x)
#define ITI_CLASS(x) GNOME_ICON_TEXT_ITEM_CLASS (x)
#define IS_ITI(x)    GNOME_IS_ICON_TEXT_ITEM (x)


typedef GnomeIconTextItem Iti;

/* Private part of the GnomeIconTextItem structure */
typedef struct {
	/* Hack: create an offscreen window and place an entry inside it */
	GtkEntry *entry;
	GtkWidget *entry_top;

	/* Maximum allowed width */
	int width;

	/* Whether the user pressed the mouse while the item was unselected */
	guint unselected_click : 1;

	/* Whether we need to update the position */
	guint need_pos_update : 1;

	/* Whether we need to update the text */
	guint need_text_update : 1;

	/* Whether we need to update because the editing/selected state changed */
	guint need_state_update : 1;
} ItiPrivate;


static GnomeCanvasItemClass *parent_class;

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

enum {
	PROP_0,
	PROP_WIDTH,
	PROP_EDITABLE
};

static guint iti_signals [LAST_SIGNAL] = { 0 };


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

/* Update handler for the text item */
static void G_GNUC_UNUSED
iti_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
	Iti *iti;
	ItiPrivate *priv;

	iti = ITI (item);
	priv = iti->priv;

	if (priv->need_state_update) {
	    GtkStyle *style;
	    GdkColor *color;

	    gtk_widget_ensure_style (GTK_WIDGET (item->canvas));
	    style = gtk_widget_get_style (GTK_WIDGET (item->canvas));
	    if (iti->selected && !iti->editing)
		color = &style->fg [GTK_STATE_SELECTED];
	    else
		color = &style->fg [GTK_STATE_NORMAL];

	    gnome_canvas_item_set (item, "fill_color_gdk", color,
				   "text", GNOME_CANVAS_TEXT (item)->text, NULL);
	}
	priv->need_state_update = FALSE;

	if (parent_class->update)
		(* parent_class->update) (item, affine, clip_path, flags);
}

/* Draw method handler for the icon text item */
static void
iti_draw (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y, int update_width, int update_height)
{
	Iti *iti;
	GtkStyle *style;
	int width, height;
	int xofs, yofs;

	iti = ITI (item);

	width  = item->x2 - item->x1;
	height = item->y2 - item->y1;
	
	xofs = item->x1 - x;
	yofs = item->y1 - y;

	style = GTK_WIDGET (item->canvas)->style;

	if (iti->editing) {
		/* Draw outline around text */
		gdk_draw_rectangle (drawable,
				    style->bg_gc[GTK_STATE_NORMAL],
				    TRUE,
				    xofs, yofs,
				    width, height);

		gdk_draw_rectangle (drawable,
				    style->fg_gc[GTK_STATE_NORMAL],
				    FALSE,
				    xofs, yofs,
				    width - 1, height - 1);
	} else {
		gdk_draw_rectangle (drawable,
				    style->bg_gc[iti->selected ? GTK_STATE_SELECTED : GTK_STATE_NORMAL],
				    TRUE,
				    xofs, yofs,
				    width, height);
	}

	if (parent_class->draw)
	    parent_class->draw (item, drawable, x, y, update_width, update_height);
}

/* Lays out the text in an icon item */
static void
layout_text (Iti *iti)
{
	ItiPrivate *priv;
	GnomeCanvasItem *item;
	int old_width, old_height;
	int width, height;

	item = GNOME_CANVAS_ITEM (iti);
	priv = iti->priv;

	old_width = item->x2 - item->x1;
	old_height = item->y2 - item->y1;

	/* Change the text layout */

	if (iti->editing) {
		const char *text = gtk_entry_get_text (priv->entry);
		gnome_canvas_item_set (GNOME_CANVAS_ITEM (iti), "text", text, NULL);
	}

	width = item->x2 - item->x1;
	height = item->y2 - item->y1;

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

	if (iti->editing)
		iti_stop_editing (iti);
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
	gtk_entry_set_text (priv->entry, GNOME_CANVAS_TEXT (iti)->text);
	gtk_signal_connect (GTK_OBJECT (priv->entry), "activate",
			    GTK_SIGNAL_FUNC (iti_entry_activate), iti);

	priv->entry_top = gtk_window_new (GTK_WINDOW_POPUP);
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
iti_finalize (GObject *object)
{
	Iti *iti;
	ItiPrivate *priv;
	GnomeCanvasItem *item;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_ITI (object));

	iti = ITI (object);
	priv = iti->priv;
	item = GNOME_CANVAS_ITEM (object);

	/* FIXME: stop selection and editing */

	/* Queue redraw of bounding box */

	if (priv)
	    gnome_canvas_request_redraw (item->canvas, item->x1, item->y1, item->x2, item->y2);

	/* Free everything */

	if (priv) {
	    if (priv->entry_top)
		gtk_widget_destroy (priv->entry_top);

	    g_free (priv);
	    iti->priv = NULL;
	}

	if (G_OBJECT_CLASS (parent_class)->finalize)
		(* G_OBJECT_CLASS (parent_class)->finalize) (object);
}

/* Point method handler for the icon text item */
static double G_GNUC_UNUSED
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
	int idx = 0;

	pango_layout_xy_to_index (GNOME_CANVAS_TEXT (iti)->layout, x, y, &idx, NULL);
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
	ibeam = gnome_stock_cursor_new (GNOME_STOCK_CURSOR_XTERM);
	gnome_canvas_item_grab (GNOME_CANVAS_ITEM (iti),
				GDK_BUTTON_RELEASE_MASK |
				GDK_POINTER_MOTION_MASK,
				ibeam, event_time);
	gdk_cursor_destroy (ibeam);

#if 0 /* FIXME */
	gtk_editable_select_region (e, idx, idx);
	e->current_pos = e->selection_start_pos;
	e->has_selection = TRUE;
	iti->selecting = TRUE;
#endif

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
#if 0 /* FIXME */
	e->has_selection = FALSE;
#endif
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

#if 0 /* FIXME */
	if (idx < e->current_pos) {
		e->selection_start_pos = idx;
		e->selection_end_pos   = e->current_pos;
	} else {
		e->selection_start_pos = e->current_pos;
		e->selection_end_pos  = idx;
	}
#endif

	priv->need_state_update = TRUE;
	gnome_canvas_item_request_update (GNOME_CANVAS_ITEM (iti));
}

/* Event handler for icon text items */
static gint G_GNUC_UNUSED
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
		if (!iti->editing)
			break;

		if (iti->editing && event->button.button == 1) {
			x = event->button.x - (item->x1 + MARGIN_X);
			y = event->button.y - (item->y1 + MARGIN_Y);
			idx = iti_idx_from_x_y (iti, x, y);

			iti_start_selecting (iti, idx, event->button.time);
		} else
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
		if (iti->selecting && event->button.button == 1)
			iti_stop_selecting (iti, event->button.time);
		else
			break;

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

/* Set_arg handler for the text item */
static void
iti_set_property (GObject            *object,
		  guint               param_id,
		  const GValue       *value,
		  GParamSpec         *pspec)
{
	ItiPrivate *priv;
	Iti *iti;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_ITI (object));

	iti = ITI (object);
	priv = iti->priv;

	switch (param_id) {
	case PROP_WIDTH:
		priv->width = g_value_get_double (value);
		pango_layout_set_width (GNOME_CANVAS_TEXT (iti)->layout,
					priv->width * 1000.0 * /* FIXME: Why 1000.0 ? */
					GNOME_CANVAS_ITEM (iti)->canvas->pixels_per_unit);
		pango_layout_set_wrap (GNOME_CANVAS_TEXT (iti)->layout, PANGO_WRAP_CHAR);
		break;

	case PROP_EDITABLE:
		iti->is_editable = g_value_get_boolean (value);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

/* Get_arg handler for the text item */
static void
iti_get_property (GObject            *object,
		  guint               param_id,
		  GValue             *value,
		  GParamSpec         *pspec)
{
	ItiPrivate *priv;
	Iti *iti;

	g_return_if_fail (object != NULL);
	g_return_if_fail (IS_ITI (object));

	iti = ITI (object);
	priv = iti->priv;

	switch (param_id) {
	case PROP_WIDTH:
		g_value_set_double (value, priv->width);
		break;

	case PROP_EDITABLE:
		g_value_set_boolean (value, iti->is_editable);
		break;

	default:
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
		break;
	}
}

/* Class initialization function for the icon text item */
static void
iti_class_init (GnomeIconTextItemClass *text_item_class)
{
	GObjectClass *object_class;
	GnomeCanvasItemClass *item_class;

	object_class = (GObjectClass *) text_item_class;
	item_class   = (GnomeCanvasItemClass *) text_item_class;

	parent_class = gtk_type_class (gnome_canvas_text_get_type ());

	iti_signals [TEXT_CHANGED] =
		gtk_signal_new (
			"text_changed",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, text_changed),
			gtk_marshal_BOOLEAN__VOID,
			GTK_TYPE_BOOL, 0);

	iti_signals [HEIGHT_CHANGED] =
		gtk_signal_new (
			"height_changed",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, height_changed),
			gtk_marshal_VOID__VOID,
			GTK_TYPE_NONE, 0);

	iti_signals [WIDTH_CHANGED] =
		gtk_signal_new (
			"width_changed",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, width_changed),
			gtk_marshal_VOID__VOID,
			GTK_TYPE_NONE, 0);

	iti_signals[EDITING_STARTED] =
		gtk_signal_new (
			"editing_started",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, editing_started),
			gtk_marshal_VOID__VOID,
			GTK_TYPE_NONE, 0);

	iti_signals[EDITING_STOPPED] =
		gtk_signal_new (
			"editing_stopped",
			GTK_RUN_LAST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, editing_stopped),
			gtk_marshal_VOID__VOID,
			GTK_TYPE_NONE, 0);

	iti_signals[SELECTION_STARTED] =
		gtk_signal_new (
			"selection_started",
			GTK_RUN_FIRST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, selection_started),
			gtk_marshal_VOID__VOID,
			GTK_TYPE_NONE, 0);

	iti_signals[SELECTION_STOPPED] =
		gtk_signal_new (
			"selection_stopped",
			GTK_RUN_FIRST,
			GTK_CLASS_TYPE (object_class),
			GTK_SIGNAL_OFFSET (GnomeIconTextItemClass, selection_stopped),
			gtk_marshal_VOID__VOID,
			GTK_TYPE_NONE, 0);

	object_class->set_property = iti_set_property;
	object_class->get_property = iti_get_property;

        g_object_class_install_property
                (object_class,
                 PROP_WIDTH,
                 g_param_spec_double ("width", NULL, NULL,
				      -G_MAXDOUBLE, G_MAXDOUBLE, 0.0,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE)));
        g_object_class_install_property
                (object_class,
                 PROP_EDITABLE,
                 g_param_spec_boolean ("editable", NULL, NULL,
				       FALSE,
				       (G_PARAM_READABLE | G_PARAM_WRITABLE)));

	object_class->finalize = iti_finalize;

	item_class->update = iti_update;
	item_class->draw = iti_draw;
	item_class->point = iti_point;
	item_class->event = iti_event;
}

/* Object initialization function for the icon text item */
static void
iti_init (GnomeIconTextItem *iti)
{
	ItiPrivate *priv;

	priv = g_new0 (ItiPrivate, 1);
	iti->priv = priv;
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

	iti->is_editable = is_editable != FALSE;

	pango_layout_set_width (GNOME_CANVAS_TEXT (iti)->layout, width);

	layout_text (iti);

	/* Request update */

	priv->need_pos_update = TRUE;
	priv->need_text_update = TRUE;
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
const char *
gnome_icon_text_item_get_text (GnomeIconTextItem *iti)
{
	ItiPrivate *priv;

	g_return_val_if_fail (iti != NULL, NULL);
	g_return_val_if_fail (IS_ITI (iti), NULL);

	priv = iti->priv;

	if (iti->editing)
		return gtk_entry_get_text (priv->entry);
	else
		return GNOME_CANVAS_TEXT (iti)->text;
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

		iti_type = gtk_type_unique (gnome_canvas_text_get_type (), &iti_info);
	}

	return iti_type;
}
