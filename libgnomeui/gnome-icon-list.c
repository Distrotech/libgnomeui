/*
 * GnomeIconList widget - scrollable icon list
 *
 * Copyright (C) 1998 the Free Software Foundation
 *
 * Authors:
 *    Federico Mena <federico@nuclecu.unam.mx>
 *    Miguel de Icaza <miguel@nuclecu.unam.mx>
 *
 * Rewrote from scratch from the code written by Federico Mena
 * <federico@nuclecu.unam.mx> to be based on a GnomeCanvas, and
 * to support banding selection and allow inline icon renaming.
 */

#include <config.h>
#include <string.h>
#include <stdio.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include "gnome-icon-list.h"
#include "gnome-icon-item.h"
#include "gnome-canvas-image.h"
#include "gnome-canvas-rect-ellipse.h"


/* Aliases to minimize screen use in my laptop */
#define GIL(x)       GNOME_ICON_LIST(x)
#define GIL_CLASS(x) GNOME_ICON_LIST_CLASS(x)
#define IS_GIL(x)    GNOME_IS_ICON_LIST(x)

typedef GnomeIconList Gil;
typedef GnomeIconListClass GilClass;


/* default spacings */
#define DEFAULT_ROW_SPACING  4
#define DEFAULT_COL_SPACING  2
#define DEFAULT_TEXT_SPACING 2
#define DEFAULT_ICON_BORDER  2

/* Autoscroll timeout in milliseconds */
#define SCROLL_TIMEOUT 30


/* Signals */
enum {
	SELECT_ICON,
	UNSELECT_ICON,
	TEXT_CHANGED,
	LAST_SIGNAL
};

typedef enum {
	SYNC_INSERT,
	SYNC_REMOVE
} SyncType;

enum {
	ARG_0,
	ARG_HADJUSTMENT,
	ARG_VADJUSTMENT
};

static guint gil_signals[LAST_SIGNAL] = { 0 };


static GtkContainerClass *parent_class;


/* Icon structure */
typedef struct {
	/* Icon image and text items */
	GnomeCanvasImage *image;
	GnomeIconTextItem *text;

	/* User data and destroy notify function */
	gpointer data;
	GtkDestroyNotify destroy;

	/* ID for the text item's event signal handler */
	guint text_event_id;

	/* Whether the icon is selected, and temporary storage for rubberband
         * selections.
	 */
	guint selected : 1;
	guint tmp_selected : 1;
} Icon;

/* A row of icons */
typedef struct {
	int y;
	int icon_height, text_height;
	GList *line_icons;
} IconLine;

/* Private data of the GnomeIconList structure */
typedef struct {
	/* List of icons and list of rows of icons */
	GList *icon_list;
	GList *lines;

	/* Max of the height of all the icon rows and window height */
	int total_height;

	/* Selection mode */
	GtkSelectionMode selection_mode;

	/* Freeze count */
	int frozen;

	/* Width allocated for icons */
	int icon_width;

	/* Spacing values */
	int row_spacing;
	int col_spacing;
	int text_spacing;
	int icon_border;

	/* Separators used to wrap the text below icons */
	char *separators;

	/* Index and pointer to last selected icon */
	int last_selected_idx;
	Icon *last_selected_icon;

	/* Timeout ID for autoscrolling */
	guint timer_tag;

	/* Change the adjustment value by this amount when autoscrolling */
	int value_diff;

	/* Mouse position for autoscrolling */
	int event_last_x;
	int event_last_y;

	/* Selection start position */
	int sel_start_x;
	int sel_start_y;

	/* Modifier state when the selection began */
	guint sel_state;

	/* Rubberband rectangle */
	GnomeCanvasItem *sel_rect;

	/* Saved event for a pending selection */
	GdkEvent select_pending_event;

	/* Whether the icon texts are editable */
	guint is_editable : 1;

	/* Whether the icon texts need to be copied */
	guint static_text : 1;

	/* Whether the icons need to be laid out */
	guint dirty : 1;

	/* Whether the user is performing a rubberband selection */
	guint selecting : 1;

	/* Whether editing an icon is pending after a button press */
	guint edit_pending : 1;

	/* Whether selection is pending after a button press */
	guint select_pending : 1;

	/* Whether the icon that is pending selection was selected to begin with */
	guint select_pending_was_selected : 1;
} GilPrivate;


static inline int
icon_line_height (Gil *gil, IconLine *il)
{
	GilPrivate *priv;

	priv = gil->priv;

	return il->icon_height + il->text_height + priv->row_spacing + priv->text_spacing;
}

static void
icon_get_height (Icon *icon, int *icon_height, int *text_height)
{
	*icon_height = icon->image->height;
	*text_height = icon->text->ti->height;
}

static int
gil_get_items_per_line (Gil *gil)
{
	GilPrivate *priv;
	int items_per_line;

	priv = gil->priv;

	items_per_line = GTK_WIDGET (gil)->allocation.width / (priv->icon_width + priv->col_spacing);
	if (items_per_line == 0)
		items_per_line = 1;

	return items_per_line;
}

/**
 * gnome_icon_list_get_items_per_line:
 * @gil: An icon list.
 *
 * Returns the number of icons that fit in a line or row.
 */
int
gnome_icon_list_get_items_per_line (GnomeIconList *gil)
{
	g_return_val_if_fail (gil != NULL, 1);
	g_return_val_if_fail (IS_GIL (gil), 1);

	return gil_get_items_per_line (gil);
}

static void
gil_place_icon (Gil *gil, Icon *icon, int x, int y, int icon_height)
{
	GilPrivate *priv;
	int y_offset;

	priv = gil->priv;

	if (icon_height > icon->image->height)
		y_offset = (icon_height - icon->image->height) / 2;
	else
		y_offset = 0;

	gnome_canvas_item_set (GNOME_CANVAS_ITEM (icon->image),
			       "x",  (double) x + priv->icon_width / 2,
			       "y",  (double) y + y_offset,
			       "anchor", GTK_ANCHOR_N,
			       NULL);
	gnome_icon_text_item_setxy (icon->text,
				    x,
				    y + icon_height + priv->text_spacing);
}

static void
gil_layout_line (Gil *gil, IconLine *il)
{
	GilPrivate *priv;
	GList *l;
	int x;

	priv = gil->priv;

	x = 0;
	for (l = il->line_icons; l; l = l->next) {
		Icon *icon = l->data;

		gil_place_icon (gil, icon, x, il->y, il->icon_height);
		x += priv->icon_width + priv->col_spacing;
	}
}

static void
gil_add_and_layout_line (Gil *gil, GList *line_icons, int y,
			 int icon_height, int text_height)
{
	GilPrivate *priv;
	IconLine *il;

	priv = gil->priv;

	il = g_new (IconLine, 1);
	il->line_icons = line_icons;
	il->y = y;
	il->icon_height = icon_height;
	il->text_height = text_height;

	gil_layout_line (gil, il);
	priv->lines = g_list_append (priv->lines, il);
}

static void
gil_relayout_icons_at (Gil *gil, int pos, int y)
{
	GilPrivate *priv;
	int col, row, text_height, icon_height;
	int items_per_line, n;
	GList *line_icons, *l;

	priv = gil->priv;
	items_per_line = gil_get_items_per_line (gil);

	col = row = text_height = icon_height = 0;
	line_icons = NULL;

	l = g_list_nth (priv->icon_list, pos);

	for (n = pos; l; l = l->next, n++) {
		Icon *icon = l->data;
		int ih, th;

		if (!(n % items_per_line)) {
			if (line_icons) {
				gil_add_and_layout_line (gil, line_icons, y,
							 icon_height, text_height);
				line_icons = NULL;

				y += (icon_height + text_height
				      + priv->row_spacing + priv->text_spacing);
			}

			icon_height = 0;
			text_height = 0;
		}

		icon_get_height (icon, &ih, &th);

		icon_height = MAX (ih, icon_height);
		text_height = MAX (th, text_height);

		line_icons = g_list_append (line_icons, icon);
	}

	if (line_icons)
		gil_add_and_layout_line (gil, line_icons, y, icon_height, text_height);
}

static void
gil_free_line_info (Gil *gil)
{
	GilPrivate *priv;
	GList *l;

	priv = gil->priv;

	for (l = priv->lines; l; l = l->next) {
		IconLine *il = l->data;

		g_list_free (il->line_icons);
		g_free (il);
	}

	g_list_free (priv->lines);
	priv->lines = NULL;
	priv->total_height = 0;
}

static void
gil_free_line_info_from (Gil *gil, int first_line)
{
	GilPrivate *priv;
	GList *l, *ll;

	priv = gil->priv;
	ll = g_list_nth (priv->lines, first_line);

	for (l = ll; l; l = l->next) {
		IconLine *il = l->data;

		g_list_free (il->line_icons);
		g_free (il);
	}

	if (priv->lines) {
		if (ll->prev)
			ll->prev->next = NULL;
		else
			priv->lines = NULL;
	}

	g_list_free (ll);
}

static void
gil_layout_from_line (Gil *gil, int line)
{
	GilPrivate *priv;
	GList *l;
	int height;

	priv = gil->priv;

	gil_free_line_info_from (gil, line);

	height = 0;
	for (l = priv->lines; l; l = l->next) {
		IconLine *il = l->data;

		height += icon_line_height (gil, il);
	}

	gil_relayout_icons_at (gil, line * gil_get_items_per_line (gil), height);
}

static void
gil_layout_all_icons (Gil *gil)
{
	GilPrivate *priv;

	priv = gil->priv;

	if (!GTK_WIDGET_REALIZED (gil))
		return;

	gil_free_line_info (gil);
	gil_relayout_icons_at (gil, 0, 0);
	priv->dirty = FALSE;
}

static void
gil_scrollbar_adjust (Gil *gil)
{
	GilPrivate *priv;
	GList *l;
	double wx, wy;
	int height, step_increment;

	priv = gil->priv;

	if (!GTK_WIDGET_REALIZED (gil))
		return;

	height = 0;
	step_increment = 0;
	for (l = priv->lines; l; l = l->next) {
		IconLine *il = l->data;

		height += icon_line_height (gil, il);

		if (l == priv->lines)
			step_increment = height;
	}

	if (!step_increment)
		step_increment = 10;

	priv->total_height = MAX (height, GTK_WIDGET (gil)->allocation.height);

	wx = wy = 0;
	gnome_canvas_window_to_world (GNOME_CANVAS (gil), 0, 0, &wx, &wy);

	gil->adj->upper = height;
	gil->adj->step_increment = step_increment;
	gil->adj->page_increment = GTK_WIDGET (gil)->allocation.height;
	gil->adj->page_size = GTK_WIDGET (gil)->allocation.height;

	if (wy > gil->adj->upper - gil->adj->page_size)
		wy = gil->adj->upper - gil->adj->page_size;

	gil->adj->value = wy;

	gtk_adjustment_changed (gil->adj);
}

/* Emits the select_icon or unselect_icon signals as appropriate */
static void
emit_select (Gil *gil, int sel, int i, GdkEvent *event)
{
	gtk_signal_emit (GTK_OBJECT (gil),
			 gil_signals[sel ? SELECT_ICON : UNSELECT_ICON],
			 i,
			 event);
}

/**
 * gnome_icon_list_unselect_all:
 * @gil:   An icon list.
 * @event: Unused, must be NULL.
 * @keep:  For internal use only; must be NULL.
 *
 * Unselects all the icons in the icon list.  The @event and @keep parameters
 * must be NULL, since they are used only internally.
 *
 * Returns: the number of icons in the icon list
 */
int
gnome_icon_list_unselect_all (GnomeIconList *gil, GdkEvent *event, gpointer keep)
{
	GilPrivate *priv;
	GList *l;
	Icon *icon;
	int i, idx = 0;

	g_return_val_if_fail (gil != NULL, 0);
	g_return_val_if_fail (IS_GIL (gil), 0);

	priv = gil->priv;

	for (l = priv->icon_list, i = 0; l; l = l->next, i++) {
		icon = l->data;

		if (icon == keep)
			idx = i;
		else if (icon->selected)
			emit_select (gil, FALSE, i, event);
	}

	return idx;
}

static void
sync_selection (Gil *gil, int pos, SyncType type)
{
	GList *list;

	for (list = gil->selection; list; list = list->next) {
		if (GPOINTER_TO_INT (list->data) >= pos) {
			int i = GPOINTER_TO_INT (list->data);

			switch (type) {
			case SYNC_INSERT:
				list->data = GINT_TO_POINTER (i + 1);
				break;

			case SYNC_REMOVE:
				list->data = GINT_TO_POINTER (i - 1);
				break;

			default:
				g_assert_not_reached ();
			}
		}
	}
}

static int
gil_icon_to_index (Gil *gil, Icon *icon)
{
	GilPrivate *priv;
	GList *l;
	int n;

	priv = gil->priv;

	n = 0;
	for (l = priv->icon_list; l; n++, l = l->next)
		if (l->data == icon)
			return n;

	g_assert_not_reached ();
	return -1; /* Shut up the compiler */
}

/* Event handler for icons when we are in SINGLE or BROWSE mode */
static gint
selection_one_icon_event (Gil *gil, Icon *icon, int idx, int on_text, GdkEvent *event)
{
	GilPrivate *priv;
	GnomeIconTextItem *text;
	int retval;

	priv = gil->priv;
	retval = FALSE;

	/* We use a separate variable and ref the object because it may be
	 * destroyed by one of the signal handlers.
	 */
	text = icon->text;
	gtk_object_ref (GTK_OBJECT (text));

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		priv->edit_pending = FALSE;
		priv->select_pending = FALSE;

		/* Ignore wheel mouse clicks for now */
		if (event->button.button > 3)
			break;

		if (!icon->selected) {
			gnome_icon_list_unselect_all (gil, NULL, NULL);
			emit_select (gil, TRUE, idx, event);
		} else {
			if (priv->selection_mode == GTK_SELECTION_SINGLE
			    && (event->button.state & GDK_CONTROL_MASK))
				emit_select (gil, FALSE, idx, event);
			else if (on_text && priv->is_editable && event->button.button == 1)
				priv->edit_pending = TRUE;
			else
				emit_select (gil, TRUE, idx, event);
		}

		retval = TRUE;
		break;

	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
		/* Ignore wheel mouse clicks for now */
		if (event->button.button > 3)
			break;

		emit_select (gil, TRUE, idx, event);
		retval = TRUE;
		break;

	case GDK_BUTTON_RELEASE:
		if (priv->edit_pending) {
			gnome_icon_text_item_start_editing (text);
			priv->edit_pending = FALSE;
		}

		retval = TRUE;
		break;

	default:
		break;
	}

	/* If the click was on the text and we actually did something, stop the
	 * icon text item's own handler from executing.
	 */
	if (on_text && retval)
		gtk_signal_emit_stop_by_name (GTK_OBJECT (text), "event");

	gtk_object_unref (GTK_OBJECT (text));

	return retval;
}

/* Handles range selections when clicking on an icon */
static void
select_range (Gil *gil, Icon *icon, int idx, GdkEvent *event)
{
	GilPrivate *priv;
	int a, b;
	GList *l;
	Icon *i;

	priv = gil->priv;

	if (priv->last_selected_idx == -1) {
		priv->last_selected_idx = idx;
		priv->last_selected_icon = icon;
	}

	if (idx < priv->last_selected_idx) {
		a = idx;
		b = priv->last_selected_idx;
	} else {
		a = priv->last_selected_idx;
		b = idx;
	}

	l = g_list_nth (priv->icon_list, a);

	for (; a <= b; a++, l = l->next) {
		i = l->data;

		if (!i->selected)
			emit_select (gil, TRUE, a, NULL);
	}

	/* Actually notify the client of the event */
	emit_select (gil, TRUE, idx, event);
}

/* Handles icon selection for MULTIPLE or EXTENDED selection modes */
static void
do_select_many (Gil *gil, Icon *icon, int idx, GdkEvent *event, int use_event)
{
	GilPrivate *priv;
	int range, additive;

	priv = gil->priv;

	range = (event->button.state & GDK_SHIFT_MASK) != 0;
	additive = (event->button.state & GDK_CONTROL_MASK) != 0;

	if (!additive) {
		if (icon->selected)
			gnome_icon_list_unselect_all (gil, NULL, icon);
		else
			gnome_icon_list_unselect_all (gil, NULL, NULL);
	}

	if (!range) {
		if (additive)
			emit_select (gil, !icon->selected, idx, use_event ? event : NULL);
		else
			emit_select (gil, TRUE, idx, use_event ? event : NULL);

		priv->last_selected_idx = idx;
		priv->last_selected_icon = icon;
	} else
		select_range (gil, icon, idx, use_event ? event : NULL);
}

/* Event handler for icons when we are in MULTIPLE or EXTENDED mode */
static gint
selection_many_icon_event (Gil *gil, Icon *icon, int idx, int on_text, GdkEvent *event)
{
	GilPrivate *priv;
	GnomeIconTextItem *text;
	int retval;
	int additive, range;
	int do_select;

	priv = gil->priv;
	retval = FALSE;

	/* We use a separate variable and ref the object because it may be
	 * destroyed by one of the signal handlers.
	 */
	text = icon->text;
	gtk_object_ref (GTK_OBJECT (text));

	range = (event->button.state & GDK_SHIFT_MASK) != 0;
	additive = (event->button.state & GDK_CONTROL_MASK) != 0;

	switch (event->type) {
	case GDK_BUTTON_PRESS:
		priv->edit_pending = FALSE;
		priv->select_pending = FALSE;

		/* Ignore wheel mouse clicks for now */
		if (event->button.button > 3)
			break;

		do_select = TRUE;

		if (additive || range) {
			if (additive && !range) {
				priv->select_pending = TRUE;
				priv->select_pending_event = *event;
				priv->select_pending_was_selected = icon->selected;

				/* We have to emit this so that the client will
				 * know about the click.
				 */
				emit_select (gil, TRUE, idx, event);
				do_select = FALSE;
			}
		} else if (icon->selected) {
			priv->select_pending = TRUE;
			priv->select_pending_event = *event;
			priv->select_pending_was_selected = icon->selected;

			if (on_text && priv->is_editable && event->button.button == 1)
				priv->edit_pending = TRUE;

			emit_select (gil, TRUE, idx, event);
			do_select = FALSE;
		}
#if 0
		} else if (icon->selected && on_text && priv->is_editable
			   && event->button.button == 1) {
			priv->edit_pending = TRUE;
			do_select = FALSE;
		}
#endif

		if (do_select)
			do_select_many (gil, icon, idx, event, TRUE);

		retval = TRUE;
		break;

	case GDK_2BUTTON_PRESS:
	case GDK_3BUTTON_PRESS:
		/* Ignore wheel mouse clicks for now */
		if (event->button.button > 3)
			break;

		emit_select (gil, TRUE, idx, event);
		retval = TRUE;
		break;

	case GDK_BUTTON_RELEASE:
		if (priv->select_pending) {
			icon->selected = priv->select_pending_was_selected;
			do_select_many (gil, icon, idx, &priv->select_pending_event, FALSE);
			priv->select_pending = FALSE;
			retval = TRUE;
		}

		if (priv->edit_pending) {
			gnome_icon_text_item_start_editing (text);
			priv->edit_pending = FALSE;
			retval = TRUE;
		}
#if 0
		if (priv->select_pending) {
			icon->selected = priv->select_pending_was_selected;
			do_select_many (gil, icon, idx, &priv->select_pending_event);
			priv->select_pending = FALSE;
			retval = TRUE;
		} else if (priv->edit_pending) {
			gnome_icon_text_item_start_editing (text);
			priv->edit_pending = FALSE;
			retval = TRUE;
		}
#endif

		break;

	default:
		break;
	}

	/* If the click was on the text and we actually did something, stop the
	 * icon text item's own handler from executing.
	 */
	if (on_text && retval)
		gtk_signal_emit_stop_by_name (GTK_OBJECT (text), "event");

	gtk_object_unref (GTK_OBJECT (text));

	return retval;
}

/* Event handler for icons in the icon list */
static gint
icon_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data)
{
	Icon *icon;
	Gil *gil;
	GilPrivate *priv;
	int idx;
	int on_text;

	icon = data;
	gil = GIL (item->canvas);
	priv = gil->priv;
	idx = gil_icon_to_index (gil, icon);
	on_text = item == GNOME_CANVAS_ITEM (icon->text);

	switch (priv->selection_mode) {
	case GTK_SELECTION_SINGLE:
	case GTK_SELECTION_BROWSE:
		return selection_one_icon_event (gil, icon, idx, on_text, event);

	case GTK_SELECTION_MULTIPLE:
	case GTK_SELECTION_EXTENDED:
		return selection_many_icon_event (gil, icon, idx, on_text, event);

	default:
		g_assert_not_reached ();
		return FALSE; /* Shut up the compiler */
	}
}

/* Handler for the editing_started signal of an icon text item.  We block the
 * event handler so that it will not be called while the text is being edited.
 */
static void
editing_started (GnomeIconTextItem *iti, gpointer data)
{
	Icon *icon;

	icon = data;
	gtk_signal_handler_block (GTK_OBJECT (iti), icon->text_event_id);
	gnome_icon_list_unselect_all (GIL (GNOME_CANVAS_ITEM (iti)->canvas), NULL, icon);
}

/* Handler for the editing_stopped signal of an icon text item.  We unblock the
 * event handler so that we can get events from it again.
 */
static void
editing_stopped (GnomeIconTextItem *iti, gpointer data)
{
	Icon *icon;

	icon = data;
	gtk_signal_handler_unblock (GTK_OBJECT (iti), icon->text_event_id);
}

static gboolean
text_changed (GnomeCanvasItem *item, Icon *icon)
{
	Gil *gil;
	gboolean accept;
	int idx;

	gil = GIL (item->canvas);
	accept = TRUE;

	idx = gil_icon_to_index (gil, icon);
	gtk_signal_emit (GTK_OBJECT (gil),
			 gil_signals[TEXT_CHANGED],
			 idx, gnome_icon_text_item_get_text (icon->text),
			 &accept);

	return accept;
}

static void
height_changed (GnomeCanvasItem *item, Icon *icon)
{
	Gil *gil;
	GilPrivate *priv;
	int n;

	gil = GIL (item->canvas);
	priv = gil->priv;

	if (!GTK_WIDGET_REALIZED (gil))
		return;

	if (priv->frozen) {
		priv->dirty = TRUE;
		return;
	}

	n = gil_icon_to_index (gil, icon);
	gil_layout_from_line (gil, n / gil_get_items_per_line (gil));
	gil_scrollbar_adjust (gil);
}

static Icon *
icon_new_from_imlib (GnomeIconList *gil, GdkImlibImage *im, const char *text)
{
	GilPrivate *priv;
	GnomeCanvas *canvas;
	GnomeCanvasGroup *group;
	Icon *icon;

	priv = gil->priv;
	canvas = GNOME_CANVAS (gil);
	group = GNOME_CANVAS_GROUP (canvas->root);

	icon = g_new0 (Icon, 1);

	if (im)
		icon->image = GNOME_CANVAS_IMAGE (gnome_canvas_item_new (
			group,
			gnome_canvas_image_get_type (),
			"x", 0.0,
			"y", 0.0,
			"width", (double) im->rgb_width,
			"height", (double) im->rgb_height,
			"image", im,
			NULL));
	else
		icon->image = GNOME_CANVAS_IMAGE (gnome_canvas_item_new (
			group,
			gnome_canvas_image_get_type (),
			"x", 0.0,
			"y", 0.0,
			NULL));

	icon->text = GNOME_ICON_TEXT_ITEM (gnome_canvas_item_new (
		group,
		gnome_icon_text_item_get_type (),
		NULL));

	gnome_canvas_item_set (GNOME_CANVAS_ITEM (icon->text),
			       "use_broken_event_handling", FALSE,
			       NULL);

	gnome_icon_text_item_configure (icon->text,
					0, 0, priv->icon_width, NULL,
					text, priv->is_editable, priv->static_text);

	gtk_signal_connect (GTK_OBJECT (icon->image), "event",
			    GTK_SIGNAL_FUNC (icon_event),
			    icon);
	icon->text_event_id = gtk_signal_connect (GTK_OBJECT (icon->text), "event",
						  GTK_SIGNAL_FUNC (icon_event),
						  icon);

	gtk_signal_connect (GTK_OBJECT (icon->text), "editing_started",
			    GTK_SIGNAL_FUNC (editing_started),
			    icon);
	gtk_signal_connect (GTK_OBJECT (icon->text), "editing_stopped",
			    GTK_SIGNAL_FUNC (editing_stopped),
			    icon);

	gtk_signal_connect (GTK_OBJECT (icon->text), "text_changed",
			    GTK_SIGNAL_FUNC (text_changed),
			    icon);
	gtk_signal_connect (GTK_OBJECT (icon->text), "height_changed",
			    GTK_SIGNAL_FUNC (height_changed),
			    icon);
	return icon;
}

static Icon *
icon_new (Gil *gil, const char *icon_filename, const char *text)
{
	GdkImlibImage *im;

	if (icon_filename)
		im = gdk_imlib_load_image ((char *) icon_filename);
	else
		im = NULL;

	return icon_new_from_imlib (gil, im, text);
}

static int
icon_list_append (Gil *gil, Icon *icon)
{
	GilPrivate *priv;
	int pos;

	priv = gil->priv;

	pos = gil->icons++;
	priv->icon_list = g_list_append (priv->icon_list, icon);

	switch (priv->selection_mode) {
	case GTK_SELECTION_BROWSE:
		gnome_icon_list_select_icon (gil, 0);
		break;

	default:
		break;
	}

	if (!priv->frozen) {
		/* FIXME: this should only layout the last line */
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	} else
		priv->dirty = TRUE;

	return gil->icons - 1;
}

static void
icon_list_insert (Gil *gil, int pos, Icon *icon)
{
	GilPrivate *priv;

	priv = gil->priv;

	if (pos == gil->icons) {
		icon_list_append (gil, icon);
		return;
	}

	priv->icon_list = g_list_insert (priv->icon_list, icon, pos);
	gil->icons++;

	switch (priv->selection_mode) {
	case GTK_SELECTION_BROWSE:
		gnome_icon_list_select_icon (gil, 0);
		break;

	default:
		break;
	}

	if (!priv->frozen) {
		/* FIXME: this should only layout the lines from then one
		 * containing the Icon to the end.
		 */
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	} else
		priv->dirty = TRUE;

	sync_selection (gil, pos, SYNC_INSERT);
}

/**
 * gnome_icon_list_insert_imlib:
 * @gil:  An icon list.
 * @pos:  Position at which the new icon should be inserted.
 * @im:   Imlib image with the icon image.
 * @text: Text to be used for the icon's caption.
 *
 * Inserts an icon in the specified icon list.  The icon is created from the
 * specified Imlib image, and it is inserted at the @pos index.
 */
void
gnome_icon_list_insert_imlib (GnomeIconList *gil, int pos, GdkImlibImage *im, const char *text)
{
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (im != NULL);

	icon = icon_new_from_imlib (gil, im, text);
	icon_list_insert (gil, pos, icon);
	return;
}

/**
 * gnome_icon_list_insert:
 * @gil:           An icon list.
 * @pos:           Position at which the new icon should be inserted.
 * @icon_filename: Name of the file that holds the icon's image.
 * @text:          Text to be used for the icon's caption.
 *
 * Inserts an icon in the specified icon list.  The icon's image is loaded
 * from the specified file, and it is inserted at the @pos index.
 */
void
gnome_icon_list_insert (GnomeIconList *gil, int pos, const char *icon_filename, const char *text)
{
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	icon = icon_new (gil, icon_filename, text);
	icon_list_insert (gil, pos, icon);
	return;
}

/**
 * gnome_icon_list_append_imlib:
 * @gil:  An icon list.
 * @im:   Imlib image with the icon image.
 * @text: Text to be used for the icon's caption.
 *
 * Appends an icon to the specified icon list.  The icon is created from
 * the specified Imlib image.
 */
int
gnome_icon_list_append_imlib (GnomeIconList *gil, GdkImlibImage *im, const char *text)
{
	Icon *icon;

	g_return_val_if_fail (gil != NULL, -1);
	g_return_val_if_fail (IS_GIL (gil), -1);
	g_return_val_if_fail (im != NULL, -1);

	icon = icon_new_from_imlib (gil, im, text);
	return icon_list_append (gil, icon);
}

/**
 * gnome_icon_list_append:
 * @gil:           An icon list.
 * @icon_filename: Name of the file that holds the icon's image.
 * @text:          Text to be used for the icon's caption.
 *
 * Appends an icon to the specified icon list.  The icon's image is loaded from
 * the specified file, and it is inserted at the @pos index.
 */
int
gnome_icon_list_append (GnomeIconList *gil, const char *icon_filename,
			 const char *text)
{
	Icon *icon;

	g_return_val_if_fail (gil != NULL, -1);
	g_return_val_if_fail (IS_GIL (gil), -1);

	icon = icon_new (gil, icon_filename, text);
	return icon_list_append (gil, icon);
}

static void
icon_destroy (Icon *icon)
{
	if (icon->destroy)
		(* icon->destroy) (icon->data);

	gtk_object_destroy (GTK_OBJECT (icon->image));
	gtk_object_destroy (GTK_OBJECT (icon->text));
	g_free (icon);
}

/**
 * gnome_icon_list_remove:
 * @gil: An icon list.
 * @pos: Index of the icon that should be removed.
 *
 * Removes the icon at index position @pos.  If a destroy handler was specified
 * for that icon, it will be called with the appropriate data.
 */
void
gnome_icon_list_remove (GnomeIconList *gil, int pos)
{
	GilPrivate *priv;
	int was_selected;
	GList *list;
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (pos >= 0 && pos < gil->icons);

	priv = gil->priv;

	was_selected = FALSE;

	list = g_list_nth (priv->icon_list, pos);
	icon = list->data;

	if (icon->selected) {
		was_selected = TRUE;

		switch (priv->selection_mode) {
		case GTK_SELECTION_SINGLE:
		case GTK_SELECTION_BROWSE:
		case GTK_SELECTION_MULTIPLE:
		case GTK_SELECTION_EXTENDED:
			gnome_icon_list_unselect_icon (gil, pos);
			break;

		default:
			break;
		}
	}

	priv->icon_list = g_list_remove_link (priv->icon_list, list);
	g_list_free_1(list);
	gil->icons--;

	sync_selection (gil, pos, SYNC_REMOVE);

	if (was_selected) {
		switch (priv->selection_mode) {
		case GTK_SELECTION_BROWSE:
			if (pos == gil->icons)
				gnome_icon_list_select_icon (gil, pos - 1);
			else
				gnome_icon_list_select_icon (gil, pos);

			break;

		default:
			break;
		}
	}

	if (gil->icons >= priv->last_selected_idx)
		priv->last_selected_idx = -1;

	if (priv->last_selected_icon == icon)
		priv->last_selected_icon = NULL;

	icon_destroy (icon);

	if (!priv->frozen) {
		/* FIXME: Optimize, only re-layout from pos to end */
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	} else
		priv->dirty = TRUE;
}

/**
 * gnome_icon_list_clear:
 * @gil: An icon list.
 *
 * Clears the contents for the icon list by removing all the icons.  If destroy
 * handlers were specified for any of the icons, they will be called with the
 * appropriate data.
 */
void
gnome_icon_list_clear (GnomeIconList *gil)
{
	GilPrivate *priv;
	GList *l;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->priv;

	for (l = priv->icon_list; l; l = l->next)
		icon_destroy (l->data);

	gil_free_line_info (gil);

	g_list_free (gil->selection);
	gil->selection = NULL;
	priv->icon_list = NULL;
	gil->icons = 0;
	priv->last_selected_idx = -1;
	priv->last_selected_icon = NULL;
}

static void
gil_destroy (GtkObject *object)
{
	Gil *gil;
	GilPrivate *priv;

	gil = GIL (object);
	priv = gil->priv;

	g_free (priv->separators);

	priv->frozen = 1;
	priv->dirty  = TRUE;
	gnome_icon_list_clear (gil);

	if (priv->timer_tag != 0) {
		gtk_timeout_remove (priv->timer_tag);
		priv->timer_tag = 0;
	}

	if (gil->adj)
		gtk_object_unref (GTK_OBJECT (gil->adj));

	if (gil->hadj)
		gtk_object_unref (GTK_OBJECT (gil->hadj));

	g_free (priv);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
select_icon (Gil *gil, int pos, GdkEvent *event)
{
	GilPrivate *priv;
	gint i;
	GList *list;
	Icon *icon;

	priv = gil->priv;

	switch (priv->selection_mode) {
	case GTK_SELECTION_SINGLE:
	case GTK_SELECTION_BROWSE:
		i = 0;

		for (list = priv->icon_list; list; list = list->next) {
			icon = list->data;

			if (i != pos && icon->selected)
				emit_select (gil, FALSE, i, event);

			i++;
		}

		emit_select (gil, TRUE, pos, event);
		break;

	case GTK_SELECTION_MULTIPLE:
	case GTK_SELECTION_EXTENDED:
		emit_select (gil, TRUE, pos, event);
		break;

	default:
		break;
	}
}

/**
 * gnome_icon_list_select_icon:
 * @gil: An icon list.
 * @pos: Index of the icon to be selected.
 *
 * Selects the icon at the index specified by @pos.
 */
void
gnome_icon_list_select_icon (GnomeIconList *gil, int pos)
{
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (pos >= 0 && pos < gil->icons);

	select_icon (gil, pos, NULL);
}

static void
unselect_icon (Gil *gil, int pos, GdkEvent *event)
{
	GilPrivate *priv;

	priv = gil->priv;

	switch (priv->selection_mode) {
	case GTK_SELECTION_SINGLE:
	case GTK_SELECTION_BROWSE:
	case GTK_SELECTION_MULTIPLE:
	case GTK_SELECTION_EXTENDED:
		emit_select (gil, FALSE, pos, event);
		break;

	default:
		break;
	}
}

/**
 * gnome_icon_list_unselect_icon:
 * @gil: An icon list.
 * @pos: Index of the icon to be unselected.
 *
 * Unselects the icon at the index specified by @pos.
 */
void
gnome_icon_list_unselect_icon (GnomeIconList *gil, int pos)
{
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (pos >= 0 && pos < gil->icons);

	unselect_icon (gil, pos, NULL);
}

static void
gil_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
	requisition->width = 1;
	requisition->height = 1;
}

static void
gil_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
	Gil *gil;
	GilPrivate *priv;

	gil = GIL (widget);
	priv = gil->priv;

	if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
		(* GTK_WIDGET_CLASS (parent_class)->size_allocate) (widget, allocation);

	if (priv->frozen)
		return;

	gil_layout_all_icons (gil);
	gil_scrollbar_adjust (gil);
}

static void
gil_realize (GtkWidget *widget)
{
	Gil *gil;
	GilPrivate *priv;
	GtkStyle *style;

	gil = GIL (widget);
	priv = gil->priv;

	priv->frozen++;

	if (GTK_WIDGET_CLASS (parent_class)->realize)
		(* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

	priv->frozen--;

	/* Change the style to use the base color as the background */

	style = gtk_style_copy (gtk_widget_get_style (widget));
	style->bg[GTK_STATE_NORMAL] = style->base[GTK_STATE_NORMAL];
	gtk_widget_set_style (widget, style);

	gdk_window_set_background (GTK_LAYOUT (gil)->bin_window,
				   &widget->style->bg[GTK_STATE_NORMAL]);

	if (priv->frozen)
		return;

	if (priv->dirty) {
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	}
}

static void
real_select_icon (Gil *gil, gint num, GdkEvent *event)
{
	GilPrivate *priv;
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (num >= 0 && num < gil->icons);

	priv = gil->priv;

	icon = g_list_nth (priv->icon_list, num)->data;

	if (icon->selected)
		return;

	icon->selected = TRUE;
	gnome_icon_text_item_select (icon->text, TRUE);
	gil->selection = g_list_append (gil->selection, GINT_TO_POINTER (num));
}

static void
real_unselect_icon (Gil *gil, gint num, GdkEvent *event)
{
	GilPrivate *priv;
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (num >= 0 && num < gil->icons);

	priv = gil->priv;

	icon = g_list_nth (priv->icon_list, num)->data;

	if (!icon->selected)
		return;

	icon->selected = FALSE;
	gnome_icon_text_item_select (icon->text, FALSE);
	gil->selection = g_list_remove (gil->selection, GINT_TO_POINTER (num));
}

/* Saves the selection of the icon list to temporary storage */
static void
store_temp_selection (Gil *gil)
{
	GilPrivate *priv;
	GList *l;
	Icon *icon;

	priv = gil->priv;

	for (l = priv->icon_list; l; l = l->next) {
		icon = l->data;

		icon->tmp_selected = icon->selected;
	}
}

#define gray50_width 2
#define gray50_height 2
static const char gray50_bits[] = {
  0x02, 0x01, };

/* Button press handler for the icon list */
static gint
gil_button_press (GtkWidget *widget, GdkEventButton *event)
{
	Gil *gil;
	GilPrivate *priv;
	int only_one;
	GdkBitmap *stipple;
	double tx, ty;

	gil = GIL (widget);
	priv = gil->priv;

	/* Invoke the canvas event handler and see if an item picks up the event */

	if ((* GTK_WIDGET_CLASS (parent_class)->button_press_event) (widget, event))
		return TRUE;

	if (!(event->type == GDK_BUTTON_PRESS
	      && event->button == 1
	      && priv->selection_mode != GTK_SELECTION_BROWSE))
		return FALSE;

	only_one = priv->selection_mode == GTK_SELECTION_SINGLE;

	if (only_one || (event->state & (GDK_SHIFT_MASK | GDK_CONTROL_MASK)) == 0)
		gnome_icon_list_unselect_all (gil, NULL, NULL);

	if (only_one)
		return TRUE;

	if (priv->selecting)
		return FALSE;

	gnome_canvas_window_to_world (GNOME_CANVAS (gil), event->x, event->y, &tx, &ty);
	priv->sel_start_x = tx;
	priv->sel_start_y = ty;
	priv->sel_state = event->state;
	priv->selecting = TRUE;

	store_temp_selection (gil);

	stipple = gdk_bitmap_create_from_data (NULL, gray50_bits, gray50_width, gray50_height);
	priv->sel_rect = gnome_canvas_item_new (gnome_canvas_root (GNOME_CANVAS (gil)),
						gnome_canvas_rect_get_type (),
						"x1", tx,
						"y1", ty,
						"x2", tx,
						"y2", ty,
						"outline_color", "black",
						"width_pixels", 1,
						"outline_stipple", stipple,
						NULL);
	gdk_bitmap_unref (stipple);

	gnome_canvas_item_grab (priv->sel_rect, GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
				NULL, event->time);

	return TRUE;
}

/* Returns whether the specified icon is at least partially inside the specified
 * rectangle.
 */
static int
icon_is_in_area (Icon *icon, int x1, int y1, int x2, int y2)
{
	double ix1, iy1, ix2, iy2;

	if (x1 == x2 && y1 == y2)
		return FALSE;

	gnome_canvas_item_get_bounds (GNOME_CANVAS_ITEM (icon->image), &ix1, &iy1, &ix2, &iy2);

	if (ix1 <= x2 && iy1 <= y2 && ix2 >= x1 && iy2 >= y1)
		return TRUE;

	gnome_canvas_item_get_bounds (GNOME_CANVAS_ITEM (icon->text), &ix1, &iy1, &ix2, &iy2);

	if (ix1 <= x2 && iy1 <= y2 && ix2 >= x1 && iy2 >= y1)
		return TRUE;

	return FALSE;
}

/* Updates the rubberband selection to the specified point */
static void
update_drag_selection (Gil *gil, int x, int y)
{
	GilPrivate *priv;
	int x1, x2, y1, y2;
	GList *l;
	int i;
	Icon *icon;
	int additive, invert;

	priv = gil->priv;

	/* Update rubberband */

	if (priv->sel_start_x < x) {
		x1 = priv->sel_start_x;
		x2 = x;
	} else {
		x1 = x;
		x2 = priv->sel_start_x;
	}

	if (priv->sel_start_y < y) {
		y1 = priv->sel_start_y;
		y2 = y;
	} else {
		y1 = y;
		y2 = priv->sel_start_y;
	}

	if (x1 < 0)
		x1 = 0;

	if (y1 < 0)
		y1 = 0;

	if (x2 >= GTK_WIDGET (gil)->allocation.width)
		x2 = GTK_WIDGET (gil)->allocation.width - 1;

	if (y2 >= priv->total_height)
		y2 = priv->total_height - 1;

	gnome_canvas_item_set (priv->sel_rect,
			       "x1", (double) x1,
			       "y1", (double) y1,
			       "x2", (double) x2,
			       "y2", (double) y2,
			       NULL);

	/* Select or unselect icons as appropriate */

	additive = priv->sel_state & GDK_SHIFT_MASK;
	invert = priv->sel_state & GDK_CONTROL_MASK;

	for (l = priv->icon_list, i = 0; l; l = l->next, i++) {
		icon = l->data;

		if (icon_is_in_area (icon, x1, y1, x2, y2)) {
			if (invert) {
				if (icon->selected == icon->tmp_selected)
					emit_select (gil, !icon->selected, i, NULL);
			} else if (additive) {
				if (!icon->selected)
					emit_select (gil, TRUE, i, NULL);
			} else {
				if (!icon->selected)
					emit_select (gil, TRUE, i, NULL);
			}
		} else if (icon->selected != icon->tmp_selected)
			emit_select (gil, icon->tmp_selected, i, NULL);
	}
}

/* Button release handler for the icon list */
static gint
gil_button_release (GtkWidget *widget, GdkEventButton *event)
{
	Gil *gil;
	GilPrivate *priv;
	double x, y;

	gil = GIL (widget);
	priv = gil->priv;

	if (!priv->selecting)
		return (* GTK_WIDGET_CLASS (parent_class)->button_release_event) (widget, event);

	if (event->button != 1)
		return FALSE;

	gnome_canvas_window_to_world (GNOME_CANVAS (gil), event->x, event->y, &x, &y);
	update_drag_selection (gil, x, y);
	gnome_canvas_item_ungrab (priv->sel_rect, event->time);

	gtk_object_destroy (GTK_OBJECT (priv->sel_rect));
	priv->sel_rect = NULL;
	priv->selecting = FALSE;

	if (priv->timer_tag != 0) {
		gtk_timeout_remove (priv->timer_tag);
		priv->timer_tag = 0;
	}

	return TRUE;
}

/* Autoscroll timeout handler for the icon list */
static gint
scroll_timeout (gpointer data)
{
	Gil *gil;
	GilPrivate *priv;
	double x, y;
	int value;

	gil = data;
	priv = gil->priv;

	GDK_THREADS_ENTER ();

	value = gil->adj->value + priv->value_diff;
	if (value > gil->adj->upper - gil->adj->page_size)
		value = gil->adj->upper - gil->adj->page_size;

	gtk_adjustment_set_value (gil->adj, value);

	gnome_canvas_window_to_world (GNOME_CANVAS (gil),
				      priv->event_last_x, priv->event_last_y,
				      &x, &y);
	update_drag_selection (gil, x, y);

	GDK_THREADS_LEAVE();

	return TRUE;
}

/* Motion event handler for the icon list */
static gint
gil_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
	Gil *gil;
	GilPrivate *priv;
	double x, y;

	gil = GIL (widget);
	priv = gil->priv;

	if (!priv->selecting)
		return (* GTK_WIDGET_CLASS (parent_class)->motion_notify_event) (widget, event);

	gnome_canvas_window_to_world (GNOME_CANVAS (gil), event->x, event->y, &x, &y);
	update_drag_selection (gil, x, y);

	/* If we are out of bounds, schedule a timeout that will do the scrolling */

	if (event->y < 0 || event->y > widget->allocation.height) {
		if (priv->timer_tag == 0)
			priv->timer_tag = gtk_timeout_add (SCROLL_TIMEOUT, scroll_timeout, gil);

		if (event->y < 0)
			priv->value_diff = event->y;
		else
			priv->value_diff = event->y - widget->allocation.height;

		priv->event_last_x = event->x;
		priv->event_last_y = event->y;

		/* Make the steppings be relative to the mouse distance from the
		 * canvas.  Also notice the timeout above is small to give a
		 * more smooth movement.
		 */
		priv->value_diff /= 5;
	} else if (priv->timer_tag != 0) {
		gtk_timeout_remove (priv->timer_tag);
		priv->timer_tag = 0;
	}

	return TRUE;
}

static void
gil_set_scroll_adjustments (GtkLayout *layout,
			    GtkAdjustment *hadjustment,
			    GtkAdjustment *vadjustment)
{
	Gil *gil = GIL (layout);

	gnome_icon_list_set_hadjustment (gil, hadjustment);
	gnome_icon_list_set_vadjustment (gil, vadjustment);
}

typedef gboolean (*xGtkSignal_BOOL__INT_POINTER) (GtkObject * object,
						  gint     arg1,
						  gpointer arg2,
						  gpointer user_data);
static void
xgtk_marshal_BOOL__INT_POINTER (GtkObject *object, GtkSignalFunc func, gpointer func_data,
				GtkArg *args)
{
  xGtkSignal_BOOL__INT_POINTER rfunc;
  gboolean *return_val;

  return_val = GTK_RETLOC_BOOL (args[2]);
  rfunc = (xGtkSignal_BOOL__INT_POINTER) func;
  *return_val = (*rfunc) (object,
			  GTK_VALUE_INT (args[0]),
			  GTK_VALUE_POINTER (args[1]),
			  func_data);
}

static void
gil_set_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeIconList *gil;
	GtkAdjustment *adjustment;

	gil = GNOME_ICON_LIST (object);

	switch (arg_id) {
	case ARG_HADJUSTMENT:
		adjustment = GTK_VALUE_POINTER (*arg);
		gnome_icon_list_set_hadjustment (gil, adjustment);
		break;

	case ARG_VADJUSTMENT:
		adjustment = GTK_VALUE_POINTER (*arg);
		gnome_icon_list_set_vadjustment (gil, adjustment);
		break;

	default:
		break;
	}
}

static void
gil_get_arg (GtkObject *object, GtkArg *arg, guint arg_id)
{
	GnomeIconList *gil;
	GilPrivate *priv;

	gil = GNOME_ICON_LIST (object);
	priv = gil->priv;

	switch (arg_id) {
	case ARG_HADJUSTMENT:
		GTK_VALUE_POINTER (*arg) = gil->hadj;
		break;

	case ARG_VADJUSTMENT:
		GTK_VALUE_POINTER (*arg) = gil->adj;
		break;

	default:
		arg->type = GTK_TYPE_INVALID;
		break;
	}
}

static void
gil_class_init (GilClass *gil_class)
{
	GtkObjectClass *object_class;
	GtkWidgetClass *widget_class;
	GtkLayoutClass *layout_class;
	GnomeCanvasClass *canvas_class;

	object_class = (GtkObjectClass *)   gil_class;
	widget_class = (GtkWidgetClass *)   gil_class;
	layout_class = (GtkLayoutClass *)   gil_class;
	canvas_class = (GnomeCanvasClass *) gil_class;

	parent_class = gtk_type_class (gnome_canvas_get_type ());

	gtk_object_add_arg_type ("GnomeIconList::hadjustment",
				 GTK_TYPE_ADJUSTMENT,
				 GTK_ARG_READWRITE,
				 ARG_HADJUSTMENT);
	gtk_object_add_arg_type ("GnomeIconList::vadjustment",
				 GTK_TYPE_ADJUSTMENT,
				 GTK_ARG_READWRITE,
				 ARG_VADJUSTMENT);

	gil_signals[SELECT_ICON] =
		gtk_signal_new (
			"select_icon",
			GTK_RUN_FIRST,
			object_class->type,
			GTK_SIGNAL_OFFSET (GnomeIconListClass, select_icon),
			gtk_marshal_NONE__INT_POINTER,
			GTK_TYPE_NONE, 2,
			GTK_TYPE_INT,
			GTK_TYPE_GDK_EVENT);

	gil_signals[UNSELECT_ICON] =
		gtk_signal_new (
			"unselect_icon",
			GTK_RUN_FIRST,
			object_class->type,
			GTK_SIGNAL_OFFSET (GnomeIconListClass, unselect_icon),
			gtk_marshal_NONE__INT_POINTER,
			GTK_TYPE_NONE, 2,
			GTK_TYPE_INT,
			GTK_TYPE_GDK_EVENT);

	gil_signals[TEXT_CHANGED] =
		gtk_signal_new (
			"text_changed",
			GTK_RUN_LAST,
			object_class->type,
			GTK_SIGNAL_OFFSET (GnomeIconListClass, text_changed),
			xgtk_marshal_BOOL__INT_POINTER,
			GTK_TYPE_BOOL, 2,
			GTK_TYPE_INT,
			GTK_TYPE_POINTER);

	gtk_object_class_add_signals (object_class, gil_signals, LAST_SIGNAL);

	object_class->destroy = gil_destroy;
	object_class->set_arg = gil_set_arg;
	object_class->get_arg = gil_get_arg;

	widget_class->size_request = gil_size_request;
	widget_class->size_allocate = gil_size_allocate;
	widget_class->realize = gil_realize;
	widget_class->button_press_event = gil_button_press;
	widget_class->button_release_event = gil_button_release;
	widget_class->motion_notify_event = gil_motion_notify;

	/* we override GtkLayout's set_scroll_adjustments signal instead
	 * of creating a new signal so as to keep binary compatibility.
	 * Anyway, a widget class only needs one of these signals, and
	 * this gives the correct implementation for GnomeIconList */
	layout_class->set_scroll_adjustments = gil_set_scroll_adjustments;

	gil_class->select_icon = real_select_icon;
	gil_class->unselect_icon = real_unselect_icon;
}

static void
gil_init (Gil *gil)
{
	GilPrivate *priv;

	priv = g_new0 (GilPrivate, 1);
	gil->priv = priv;

	priv->row_spacing = DEFAULT_ROW_SPACING;
	priv->col_spacing = DEFAULT_COL_SPACING;
	priv->text_spacing = DEFAULT_TEXT_SPACING;
	priv->icon_border = DEFAULT_ICON_BORDER;
	priv->separators = g_strdup (" ");

	priv->selection_mode = GTK_SELECTION_SINGLE;
	priv->dirty = TRUE;

	/* FIXME:  Make the icon list use the normal canvas/layout adjustments */
	gnome_canvas_set_scroll_region (GNOME_CANVAS (gil), 0.0, 0.0, 1000000.0, 1000000.0);
	gnome_canvas_scroll_to (GNOME_CANVAS (gil), 0, 0);
}

/**
 * gnome_icon_list_get_type:
 *
 * Registers the &GnomeIconList class if necessary, and returns the type ID
 * associated to it.
 *
 * Returns: The type ID of the &GnomeIconList class.
 */
guint
gnome_icon_list_get_type (void)
{
	static guint gil_type = 0;

	if (!gil_type) {
		GtkTypeInfo gil_info = {
			"GnomeIconList",
			sizeof (GnomeIconList),
			sizeof (GnomeIconListClass),
			(GtkClassInitFunc) gil_class_init,
			(GtkObjectInitFunc) gil_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		gil_type = gtk_type_unique (gnome_canvas_get_type (),
					    &gil_info);
	}

	return gil_type;
}

/**
 * gnome_icon_list_set_icon_width:
 * @gil: An icon list.
 * @w:   New width for the icon columns.
 *
 * Sets the amount of horizontal space allocated to the icons, i.e. the column
 * width of the icon list.
 */
void
gnome_icon_list_set_icon_width (GnomeIconList *gil, int w)
{
	GilPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->priv;

	priv->icon_width  = w;

	if (priv->frozen) {
		priv->dirty = TRUE;
		return;
	}

	gil_layout_all_icons (gil);
	gil_scrollbar_adjust (gil);
}

static void
gil_adj_value_changed (GtkAdjustment *adj, Gil *gil)
{
	gnome_canvas_scroll_to (GNOME_CANVAS (gil), 0, adj->value);
}

/**
 * gnome_icon_list_set_hadjustment:
 * @gil: An icon list.
 * @hadj: Adjustment to be used for horizontal scrolling.
 *
 * Sets the adjustment to be used for horizontal scrolling.  This is normally
 * not required, as the icon list can be simply inserted in a &GtkScrolledWindow
 * and scrolling will be handled automatically.
 **/
void
gnome_icon_list_set_hadjustment (GnomeIconList *gil, GtkAdjustment *hadj)
{
	GilPrivate *priv;
	GtkAdjustment *old_adjustment;

	/* hadj isn't used but is here for compatibility with GtkScrolledWindow */

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	if (hadj)
		g_return_if_fail (GTK_IS_ADJUSTMENT (hadj));

	priv = gil->priv;

	if (gil->hadj == hadj)
		return;

	old_adjustment = gil->hadj;

	if (gil->hadj)
		gtk_object_unref (GTK_OBJECT (gil->hadj));

	gil->hadj = hadj;

	if (gil->hadj) {
		gtk_object_ref (GTK_OBJECT (gil->hadj));
		/* The horizontal adjustment is not used, so set some default
		 * values to indicate that everything is visible horizontally.
		 */
		gil->hadj->lower = 0.0;
		gil->hadj->upper = 1.0;
		gil->hadj->value = 0.0;
		gil->hadj->step_increment = 1.0;
		gil->hadj->page_increment = 1.0;
		gil->hadj->page_size = 1.0;
		gtk_adjustment_changed (gil->hadj);
	}

	if (!gil->hadj || !old_adjustment)
		gtk_widget_queue_resize (GTK_WIDGET (gil));
}

/**
 * gnome_icon_list_set_vadjustment:
 * @gil: An icon list.
 * @hadj: Adjustment to be used for horizontal scrolling.
 *
 * Sets the adjustment to be used for vertical scrolling.  This is normally not
 * required, as the icon list can be simply inserted in a &GtkScrolledWindow and
 * scrolling will be handled automatically.
 **/
void
gnome_icon_list_set_vadjustment (GnomeIconList *gil, GtkAdjustment *vadj)
{
	GilPrivate *priv;
	GtkAdjustment *old_adjustment;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	if (vadj)
		g_return_if_fail (GTK_IS_ADJUSTMENT (vadj));

	priv = gil->priv;

	if (gil->adj == vadj)
		return;

	old_adjustment = gil->adj;

	if (gil->adj) {
		gtk_signal_disconnect_by_data (GTK_OBJECT (gil->adj), gil);
		gtk_object_unref (GTK_OBJECT (gil->adj));
	}

	gil->adj = vadj;

	if (gil->adj) {
		gtk_object_ref (GTK_OBJECT (gil->adj));
		gtk_object_sink (GTK_OBJECT (gil->adj));
		gtk_signal_connect (GTK_OBJECT (gil->adj), "value_changed",
				    GTK_SIGNAL_FUNC (gil_adj_value_changed), gil);
		gtk_signal_connect (GTK_OBJECT (gil->adj), "changed",
				    GTK_SIGNAL_FUNC (gil_adj_value_changed), gil);
	}

	if (!gil->adj || !old_adjustment)
		gtk_widget_queue_resize (GTK_WIDGET (gil));
}

/**
 * gnome_icon_list_construct:
 * @gil: An icon list.
 * @icon_width: Width for the icon columns.
 * @adj: Adjustment to be used for vertical scrolling.
 * @flags: A combination of %GNOME_ICON_LIST_IS_EDITABLE and %GNOME_ICON_LIST_STATIC_TEXT.
 *
 * Constructor for the icon list, to be used by derived classes.
 **/
void
gnome_icon_list_construct (GnomeIconList *gil, guint icon_width, GtkAdjustment *adj, int flags)
{
	GilPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->priv;

	gnome_icon_list_set_icon_width (gil, icon_width);
	priv->is_editable = (flags & GNOME_ICON_LIST_IS_EDITABLE) != 0;
	priv->static_text = (flags & GNOME_ICON_LIST_STATIC_TEXT) != 0;

	if (!adj)
		adj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 1, 0.1, 0.1, 0.1));

	gnome_icon_list_set_vadjustment (gil, adj);

}


/**
 * gnome_icon_list_new_flags: [constructor]
 * @icon_width: Width for the icon columns.
 * @adj:        Adjustment to be used for vertical scrolling.
 * @flags:      A combination of %GNOME_ICON_LIST_IS_EDITABLE and %GNOME_ICON_LIST_STATIC_TEXT.
 *
 * Creates a new icon list widget.  The icon columns are allocated a width of
 * @icon_width pixels.  Icon captions will be word-wrapped to this width as
 * well.
 *
 * The adjustment is used to pass an existing adjustment to be used to control
 * the icon list's vertical scrolling.  Normally NULL can be passed here; if the
 * icon list is inserted into a &GtkScrolledWindow, it will handle scrolling
 * automatically.
 *
 * If @flags has the %GNOME_ICON_LIST_IS_EDITABLE flag set, then the user will be
 * able to edit the text in the icon captions, and the "text_changed" signal
 * will be emitted when an icon's text is changed.
 *
 * If @flags has the %GNOME_ICON_LIST_STATIC_TEXT flags set, then the text
 * for the icon captions will not be copied inside the icon list; it will only
 * store the pointers to the original text strings specified by the application.
 * This is intended to save memory.  If this flag is not set, then the text
 * strings will be copied and managed internally.
 *
 * Returns: a newly-created icon list widget
 */
GtkWidget *
gnome_icon_list_new_flags (guint icon_width, GtkAdjustment *adj, int flags)
{
	Gil *gil;

	gtk_widget_push_visual (gdk_imlib_get_visual ());
	gtk_widget_push_colormap (gdk_imlib_get_colormap ());
	gil = GIL (gtk_type_new (gnome_icon_list_get_type ()));
	gtk_widget_pop_visual ();
	gtk_widget_pop_colormap ();

	gnome_icon_list_construct (gil, icon_width, adj, flags);

	return GTK_WIDGET (gil);
}

/**
 * gnome_icon_list_new:
 * @icon_width: Width for the icon columns.
 * @adj: Adjustment to be used for vertical scrolling.
 * @flags: A combination of %GNOME_ICON_LIST_IS_EDITABLE and %GNOME_ICON_LIST_STATIC_TEXT.
 *
 * This function is kept for binary compatibility with old applications.  It is
 * similar in purpose to gnome_icon_list_new_flags(), but it will always turn on
 * the %GNOME_ICON_LIST_IS_EDITABLE flag.
 *
 * Return value: a newly-created icon list widget
 **/
GtkWidget *
gnome_icon_list_new (guint icon_width, GtkAdjustment *adj, int flags)
{
	return gnome_icon_list_new_flags (icon_width, adj, flags & GNOME_ICON_LIST_IS_EDITABLE);
}

/**
 * gnome_icon_list_freeze:
 * @gil:  An icon list.
 *
 * Freezes an icon list so that any changes made to it will not be
 * reflected on the screen until it is thawed with gnome_icon_list_thaw().
 * It is recommended to freeze the icon list before inserting or deleting
 * many icons, for example, so that the layout process will only be executed
 * once, when the icon list is finally thawed.
 *
 * You can call this function multiple times, but it must be balanced with the
 * same number of calls to gnome_icon_list_thaw() before the changes will take
 * effect.
 */
void
gnome_icon_list_freeze (GnomeIconList *gil)
{
	GilPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->priv;

	priv->frozen++;

	/* We hide the root so that the user will not see any changes while the
	 * icon list is doing stuff.
	 */

	if (priv->frozen == 1)
		gnome_canvas_item_hide (GNOME_CANVAS (gil)->root);
}

/**
 * gnome_icon_list_thaw:
 * @gil:  An icon list.
 *
 * Thaws the icon list and performs any pending layout operations.  This
 * is to be used in conjunction with gnome_icon_list_freeze().
 */
void
gnome_icon_list_thaw (GnomeIconList *gil)
{
        GilPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->priv;

	g_return_if_fail (priv->frozen > 0);

	priv->frozen--;

	if (priv->dirty) {
                gil_layout_all_icons (gil);
                gil_scrollbar_adjust (gil);
        }

	if (priv->frozen == 0)
		gnome_canvas_item_show (GNOME_CANVAS (gil)->root);
}

/**
 * gnome_icon_list_set_selection_mode
 * @gil:  An icon list.
 * @mode: New selection mode.
 *
 * Sets the selection mode for an icon list.  The %GTK_SELECTION_MULTIPLE and
 * %GTK_SELECTION_EXTENDED modes are considered equivalent.
 */
void
gnome_icon_list_set_selection_mode (GnomeIconList *gil, GtkSelectionMode mode)
{
	GilPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->priv;

	priv->selection_mode = mode;
	priv->last_selected_idx = -1;
	priv->last_selected_icon = NULL;
}

/**
 * gnome_icon_list_set_icon_data_full:
 * @gil:     An icon list.
 * @pos:     Index of an icon.
 * @data:    User data to set on the icon.
 * @destroy: Destroy notification handler for the icon.
 *
 * Associates the @data pointer to the icon at the index specified by @pos.  The
 * @destroy argument points to a function that will be called when the icon is
 * destroyed, or NULL if no function is to be called when this happens.
 */
void
gnome_icon_list_set_icon_data_full (GnomeIconList *gil,
				    int pos, gpointer data,
				    GtkDestroyNotify destroy)
{
	GilPrivate *priv;
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (pos >= 0 && pos < gil->icons);

	priv = gil->priv;

	icon = g_list_nth (priv->icon_list, pos)->data;
	icon->data = data;
	icon->destroy = destroy;
}

/**
 * gnome_icon_list_get_icon_data:
 * @gil:  An icon list.
 * @pos:  Index of an icon.
 * @data: User data to set on the icon.
 *
 * Associates the @data pointer to the icon at the index specified by @pos.
 */
void
gnome_icon_list_set_icon_data (GnomeIconList *gil, int pos, gpointer data)
{
	gnome_icon_list_set_icon_data_full (gil, pos, data, NULL);
}

/**
 * gnome_icon_list_get_icon_data:
 * @gil: An icon list.
 * @pos: Index of an icon.
 *
 * Returns the user data pointer associated to the icon at the index specified
 * by @pos.
 */
gpointer
gnome_icon_list_get_icon_data (GnomeIconList *gil, int pos)
{
	GilPrivate *priv;
	Icon *icon;

	g_return_val_if_fail (gil != NULL, NULL);
	g_return_val_if_fail (IS_GIL (gil), NULL);
	g_return_val_if_fail (pos >= 0 && pos < gil->icons, NULL);

	priv = gil->priv;

	icon = g_list_nth (priv->icon_list, pos)->data;
	return icon->data;
}

/**
 * gnome_icon_list_find_icon_from_data:
 * @gil:    An icon list.
 * @data:   Data pointer associated to an icon.
 *
 * Returns the index of the icon whose user data has been set to @data,
 * or -1 if no icon has this data associated to it.
 */
int
gnome_icon_list_find_icon_from_data (GnomeIconList *gil, gpointer data)
{
	GilPrivate *priv;
	GList *list;
	int n;
	Icon *icon;

	g_return_val_if_fail (gil != NULL, -1);
	g_return_val_if_fail (IS_GIL (gil), -1);

	priv = gil->priv;

	for (n = 0, list = priv->icon_list; list; n++, list = list->next) {
		icon = list->data;
		if (icon->data == data)
			return n;
	}

	return -1;
}

/* Sets an integer value in the private structure of the icon list, and updates it */
static void
set_value (GnomeIconList *gil, GilPrivate *priv, int *dest, int val)
{
	if (val == *dest)
		return;

	*dest = val;

	if (!priv->frozen) {
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	} else
		priv->dirty = TRUE;
}

/**
 * gnome_icon_list_set_row_spacing:
 * @gil:    An icon list.
 * @pixels: Number of pixels for inter-row spacing.
 *
 * Sets the spacing to be used between rows of icons.
 */
void
gnome_icon_list_set_row_spacing (GnomeIconList *gil, int pixels)
{
	GilPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->priv;
	set_value (gil, priv, &priv->row_spacing, pixels);
}

/**
 * gnome_icon_list_set_col_spacing:
 * @gil:    An icon list.
 * @pixels: Number of pixels for inter-column spacing.
 *
 * Sets the spacing to be used between columns of icons.
 */
void
gnome_icon_list_set_col_spacing (GnomeIconList *gil, int pixels)
{
	GilPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->priv;
	set_value (gil, priv, &priv->col_spacing, pixels);
}

/**
 * gnome_icon_list_set_text_spacing:
 * @gil:    An icon list.
 * @pixels: Number of pixels between an icon's image and its caption.
 *
 * Sets the spacing to be used between an icon's image and its text caption.
 */
void
gnome_icon_list_set_text_spacing (GnomeIconList *gil, int pixels)
{
	GilPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->priv;
	set_value (gil, priv, &priv->text_spacing, pixels);
}

/**
 * gnome_icon_list_set_icon_border:
 * @gil:    An icon list.
 * @pixels: Number of border pixels to be used around an icon's image.
 *
 * Sets the width of the border to be displayed around an icon's image.  This is
 * currently not implemented.
 */
void
gnome_icon_list_set_icon_border (GnomeIconList *gil, int pixels)
{
	GilPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	priv = gil->priv;
	set_value (gil, priv, &priv->icon_border, pixels);
}

/**
 * gnome_icon_list_set_separators:
 * @gil: An icon list.
 * @sep: String with characters to be used as word separators.
 *
 * Sets the characters that can be used as word separators when doing
 * word-wrapping in the icon text captions.
 */
void
gnome_icon_list_set_separators (GnomeIconList *gil, const char *sep)
{
	GilPrivate *priv;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (sep != NULL);

	priv = gil->priv;

	if (priv->separators)
		g_free (priv->separators);

	priv->separators = g_strdup (sep);

	if (priv->frozen) {
		priv->dirty = TRUE;
		return;
	}

	gil_layout_all_icons (gil);
	gil_scrollbar_adjust (gil);
}

/**
 * gnome_icon_list_moveto:
 * @gil:    An icon list.
 * @pos:    Index of an icon.
 * @yalign: Vertical alignment of the icon.
 *
 * Makes the icon whose index is @pos be visible on the screen.  The icon list
 * gets scrolled so that the icon is visible.  An alignment of 0.0 represents
 * the top of the visible part of the icon list, and 1.0 represents the bottom.
 * An icon can be centered on the icon list.
 */
void
gnome_icon_list_moveto (GnomeIconList *gil, int pos, double yalign)
{
	GilPrivate *priv;
	IconLine *il;
	GList *l;
	int i, y, uh, line;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (pos >= 0 && pos < gil->icons);
	g_return_if_fail (yalign >= 0.0 && yalign <= 1.0);

	priv = gil->priv;

	g_return_if_fail (priv->lines != NULL);

	line = pos / gil_get_items_per_line (gil);

	y = 0;
	for (i = 0, l = priv->lines; l && i < line; l = l->next, i++) {
		il = l->data;
		y += icon_line_height (gil, il);
	}

	il = l->data;

	uh = GTK_WIDGET (gil)->allocation.height - icon_line_height (gil,il);
	gtk_adjustment_set_value (gil->adj, y - uh * yalign);
}

/**
 * gnome_icon_list_is_visible:
 * @gil: An icon list.
 * @pos: Index of an icon.
 *
 * Returns whether the icon at the index specified by @pos is visible.  This
 * will be %GTK_VISIBILITY_NONE if the icon is not visible at all,
 * %GTK_VISIBILITY_PARTIAL if the icon is at least partially shown, and
 * %GTK_VISIBILITY_FULL if the icon is fully visible.
 */
GtkVisibility
gnome_icon_list_icon_is_visible (GnomeIconList *gil, int pos)
{
	GilPrivate *priv;
	IconLine *il;
	GList *l;
	int line, y1, y2, i;

	g_return_val_if_fail (gil != NULL, GTK_VISIBILITY_NONE);
	g_return_val_if_fail (IS_GIL (gil), GTK_VISIBILITY_NONE);
	g_return_val_if_fail (pos >= 0 && pos < gil->icons, GTK_VISIBILITY_NONE);

	priv = gil->priv;

	if (priv->lines == NULL)
		return GTK_VISIBILITY_NONE;

	line = pos / gil_get_items_per_line (gil);
	y1 = 0;
	for (i = 0, l = priv->lines; l && i < line; l = l->next, i++) {
		il = l->data;
		y1 += icon_line_height (gil, il);
	}

	y2 = y1 + icon_line_height (gil, (IconLine *) l->data);

	if (y2 < gil->adj->value)
		return GTK_VISIBILITY_NONE;

	if (y1 > gil->adj->value + GTK_WIDGET (gil)->allocation.height)
		return GTK_VISIBILITY_NONE;

	if (y2 <= gil->adj->value + GTK_WIDGET (gil)->allocation.height &&
	    y1 >= gil->adj->value)
		return GTK_VISIBILITY_FULL;

	return GTK_VISIBILITY_PARTIAL;
}

/**
 * gnome_icon_list_get_icon_at:
 * @gil: An icon list.
 * @x:   X position in the icon list window.
 * @y:   Y position in the icon list window.
 *
 * Returns the index of the icon that is under the specified coordinates, which
 * are relative to the icon list's window.  If there is no icon in that
 * position, -1 is returned.
 */
int
gnome_icon_list_get_icon_at (GnomeIconList *gil, int x, int y)
{
	GilPrivate *priv;
	GList *l;
	double wx, wy;
	double dx, dy;
	int cx, cy;
	int n;
	GnomeCanvasItem *item;
	double dist;

	g_return_val_if_fail (gil != NULL, -1);
	g_return_val_if_fail (IS_GIL (gil), -1);

	priv = gil->priv;

	dx = x;
	dy = y;

	gnome_canvas_window_to_world (GNOME_CANVAS (gil), dx, dy, &wx, &wy);
	gnome_canvas_w2c (GNOME_CANVAS (gil), wx, wy, &cx, &cy);

	for (n = 0, l = priv->icon_list; l; l = l->next, n++) {
		Icon *icon = l->data;
		GnomeCanvasItem *image = GNOME_CANVAS_ITEM (icon->image);
		GnomeCanvasItem *text = GNOME_CANVAS_ITEM (icon->text);

		if (wx >= image->x1 && wx <= image->x2 && wy >= image->y1 && wy <= image->y2) {
			dist = (* GNOME_CANVAS_ITEM_CLASS (image->object.klass)->point) (
				image,
				wx, wy,
				cx, cy,
				&item);

			if ((int) (dist * GNOME_CANVAS (gil)->pixels_per_unit + 0.5)
			    <= GNOME_CANVAS (gil)->close_enough)
				return n;
		}

		if (wx >= text->x1 && wx <= text->x2 && wy >= text->y1 && wy <= text->y2) {
			dist = (* GNOME_CANVAS_ITEM_CLASS (text->object.klass)->point) (
				text,
				wx, wy,
				cx, cy,
				&item);

			if ((int) (dist * GNOME_CANVAS (gil)->pixels_per_unit + 0.5)
			    <= GNOME_CANVAS (gil)->close_enough)
				return n;
		}
	}

	return -1;
}
