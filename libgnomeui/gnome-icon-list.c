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
#include <gdk_imlib.h>
#include <gtk/gtkadjustment.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmain.h>
#include <libgnomeui/gnome-icon-list.h>
#include <libgnomeui/gnome-icon-item.h>
#include <libgnomeui/gnome-canvas-image.h>
#include <libgnomeui/gnome-canvas-rect-ellipse.h>
#include <string.h>
#include <stdio.h>

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

static guint gil_signals [LAST_SIGNAL] = { 0 };

/* Inheritance */
static GtkContainerClass *parent_class;

typedef struct {
	GnomeCanvasImage  *image;
	GnomeIconTextItem *text;
	GtkStateType state;

	gpointer data;
	GtkDestroyNotify destroy;
} Icon;

typedef struct {
	int y;
	int icon_height, text_height;
	GList *line_icons;
} IconLine;

static inline int
icon_line_height (Gil *gil, IconLine *il)
{
	return il->icon_height + il->text_height +
		gil->row_spacing + gil->text_spacing;
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
	int items_per_line;
	
	items_per_line = GTK_WIDGET (gil)->allocation.width / (gil->icon_width + gil->col_spacing);
	if (items_per_line == 0)
		items_per_line = 1;

	return items_per_line;
}

/**
 * gnome_icon_list_get_items_per_line:
 * @gil: the icon list
 *
 * Returns the number of icons that fit in a line
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
	int y_offset;
	
	if (icon_height > icon->image->height)
		y_offset = (icon_height - icon->image->height) / 2;
	else
		y_offset = 0;
	
	gnome_canvas_item_set (GNOME_CANVAS_ITEM (icon->image),
			       "x",  (double) x + gil->icon_width/2,
			       "y",  (double) y + y_offset,
			       "anchor", GTK_ANCHOR_N,
			       NULL);
	gnome_icon_text_item_setxy (
		icon->text, x,
		y + icon_height + gil->text_spacing);
}

static void
gil_layout_line (Gil *gil, IconLine *il)
{
	GList *l;
	int x;

	x = 0;
	for (l = il->line_icons; l; l = l->next){
		Icon *icon = l->data;
		
		gil_place_icon (gil, icon, x, il->y, il->icon_height);
		x += gil->icon_width + gil->col_spacing;
	}
}

static void
gil_add_and_layout_line (Gil *gil, GList *line_icons, int y,
			 int icon_height, int text_height)
{
	IconLine *il;
	
	il = g_new (IconLine, 1);
	il->line_icons = line_icons;
	il->y = y;
	il->icon_height = icon_height;
	il->text_height = text_height;
	
	gil_layout_line (gil, il);
	gil->lines = g_list_append (gil->lines, il);
}

static void
gil_relayout_icons_at (Gil *gil, int pos, int y)
{
	int col, row, text_height, icon_height;
	int items_per_line, n;
	GList *line_icons, *l;
		
	items_per_line = gil_get_items_per_line (gil);

	col = row = text_height = icon_height = 0;
	line_icons = NULL;

	l = g_list_nth (gil->icon_list, pos);

	for (n = pos; l; l = l->next, n++){
		Icon *icon = l->data;
		int ih, th;

		if (!(n % items_per_line)){
			if (line_icons){
				gil_add_and_layout_line (
					gil, line_icons, y,
					icon_height, text_height);
				line_icons = NULL;
				
				y += icon_height + text_height +
					gil->row_spacing + gil->text_spacing;
			}
			
			icon_height = 0;
			text_height = 0;
		}

		icon_get_height (icon, &ih, &th);
		
		icon_height = MAX (ih, icon_height);
		text_height = MAX (th, text_height);

		line_icons = g_list_append (line_icons, icon);
	}
	if (line_icons){
		gil_add_and_layout_line (
			gil, line_icons, y, icon_height, text_height);
	}
}

static void
gil_free_line_info (Gil *gil)
{
	GList *l = gil->lines;

	for (l = gil->lines; l; l = l->next){
		IconLine *il = l->data;

		g_list_free (il->line_icons);
		g_free (il);
	}
	g_list_free (gil->lines);
	gil->lines = NULL;
}

static void
gil_free_line_info_from (Gil *gil, int first_line)
{
	GList *l, *ll;

	ll = g_list_nth (gil->lines, first_line);

	for (l = ll; l; l = l->next){
		IconLine *il = l->data;
		
		g_list_free (il->line_icons);
		g_free (il);
	}
	if (gil->lines){
		if (ll->prev)
			ll->prev->next = NULL;
		else
			gil->lines = NULL;
	}
	g_list_free (ll);
}

static void
gil_layout_from_line (Gil *gil, int line)
{
	GList *l;
	int height;
	
	gil_free_line_info_from (gil, line);

	height = 0;
	for (l = gil->lines; l; l = l->next){
		IconLine *il = l->data;

		height += icon_line_height (gil, il);
	}
	gil_relayout_icons_at (gil, line * gil_get_items_per_line (gil), height);
}

static void
gil_layout_all_icons (Gil *gil)
{
	if (!GTK_WIDGET_REALIZED (gil))
		return;
	
	gil_free_line_info (gil);
	gil_relayout_icons_at (gil, 0, 0);
	gil->dirty = FALSE;
}

static int
gil_icon_to_index (Gil *gil, Icon *icon)
{
	GList *l;
	int n;
	
	n = 0;
	for (l = gil->icon_list; l; n++, l = l->next){
		if (l->data == icon){
			return n;
		}
	}

	return 0;
}

static void
gil_scrollbar_adjust (Gil *gil)
{
	GList *l;
	double wx, wy;
	int height, step_increment;
	
	if (!GTK_WIDGET_REALIZED (gil))
		return;

	height = 0;
	step_increment = 0;
	for (l = gil->lines; l; l = l->next){
		IconLine *il = l->data;

		height += icon_line_height (gil, il);
		
		if (l == gil->lines)
			step_increment = height;
	}
	if (!step_increment)
		step_increment = 10;
	
	wx = wy = 0;
	gnome_canvas_window_to_world (GNOME_CANVAS (gil), 0, 0, &wx, &wy);
	
	gil->adj->upper = height;
	gil->adj->value = wy;
	gil->adj->step_increment = step_increment;
	gil->adj->page_increment = GTK_WIDGET (gil)->allocation.height;
	gil->adj->page_size = GTK_WIDGET (gil)->allocation.height;

	gtk_adjustment_changed (gil->adj);
}

/**
 * gnome_icon_list_unselect_all:
 * @gil:   the icon list
 * @event: event which triggered this action (might be NULL)
 * @keep:  pointer to an icon to keep (might be NULL).
 *
 * Unselects all of the icons in the GIL icon list.  EVENT is the event
 * that triggered the unselect action (or NULL if the event is not available).
 *
 * The keep parameter is only used internally in the Icon LIst code, it can be NULL. 
 */
int
gnome_icon_list_unselect_all (GnomeIconList *gil, GdkEvent *event, void *keep)
{
	GList *icon_list = gil->icon_list;
	Icon *icon;
	int i, idx = 0;
	
	g_return_val_if_fail (gil != NULL, 0);
	g_return_val_if_fail (IS_GIL (gil), 0);

	for (i = 0; icon_list; icon_list = icon_list->next, i++){
		icon = icon_list->data;

		if (icon == keep)
			idx = i;
		
		if (icon != keep && icon->text->selected)
			gtk_signal_emit (GTK_OBJECT (gil), gil_signals [UNSELECT_ICON], i, event);
	}

	return idx;
}

static void
toggle_icon (Gil *gil, Icon *icon, GdkEvent *event)
{
	gint n, count, idx;
	GList *l;
	Icon *selected_icon;

	n = 0;
	idx = -1;
	selected_icon = NULL;

	switch (gil->selection_mode){
	case GTK_SELECTION_SINGLE: 
		selected_icon = icon;
		
		for (l = gil->icon_list; l; l = l->next, n++){
			Icon *i = l->data;

			if (i == icon){
				idx = n;
				continue;
			}

			if (i->text->selected && event->button.button == 1)
				gtk_signal_emit (GTK_OBJECT (gil), gil_signals[UNSELECT_ICON], n, event);
		}

		if (selected_icon && (selected_icon->text->selected) && (event->button.button == 1))
			gtk_signal_emit (GTK_OBJECT (gil), gil_signals [UNSELECT_ICON], idx, event);
		else if (event->button.button == 1)
			gtk_signal_emit (GTK_OBJECT (gil), gil_signals [SELECT_ICON], idx, event);
		break;

	case GTK_SELECTION_BROWSE:
		for (l = gil->icon_list; l; l = l->next, n++){
			Icon *i = l->data;

			if (i == icon){
				idx = n;
				continue;
			}
			
			if (icon->text->selected && (event->button.button == 1))
				gtk_signal_emit (GTK_OBJECT (gil), gil_signals [UNSELECT_ICON], n, event);
		}

		if (event->button.button == 1)
			gtk_signal_emit (GTK_OBJECT (gil), gil_signals [SELECT_ICON], idx, event);

		break;

	case GTK_SELECTION_MULTIPLE:
		/* If neither the shift or control keys are pressed, clear the selection */
		if (!(event->button.state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK))){
			
			if ((event->button.button == 1) || (!icon->text->selected))
				idx = gnome_icon_list_unselect_all (gil, event, icon);

			if (idx == -1){
				GList *l = gil->icon_list;
				int i = 0;
				
				for (; l; l = l->next, i++){
					if (l->data == icon){
						idx = i;
						break;
					}
				}
			}
			g_assert (idx != -1);
			
			gtk_signal_emit (GTK_OBJECT (gil), gil_signals [SELECT_ICON], idx, event);
			gil->last_selected = idx;
			break;
		}

		/* If we have not looked up the index for this Icon yet, look it up */
		if (idx == -1){
			for (idx = 0, l = gil->icon_list; l; l = l->next, idx++){
				if (l->data == icon)
					break;
			}
		}

		g_return_if_fail (idx != -1);
		
		if ((event->button.state & GDK_SHIFT_MASK) && (event->button.button == 1)){
			int first, last, pos;

			if (gil->last_selected < idx){
				first = gil->last_selected;
				last  = idx;
			} else {
				first = idx;
				last  = gil->last_selected;
			}

			count = last - first + 1;
			l = g_list_nth (gil->icon_list, first);
			pos = first;
			while (count--){
				icon = l->data;

				if (!icon->text->selected)
					gtk_signal_emit (GTK_OBJECT (gil), gil_signals [SELECT_ICON], pos, event);

				l = l->next;
				pos++;
			}
		} else if ((event->button.state & GDK_CONTROL_MASK) && (event->button.button == 1)){
			int signal;
			
			signal = icon->text->selected ? UNSELECT_ICON : SELECT_ICON;
			gtk_signal_emit (GTK_OBJECT (gil), gil_signals [signal], idx, event);
		}
		gil->last_selected = idx;
		break;

	case GTK_SELECTION_EXTENDED:
		break;

	default:
		break;
	}
}

static void
sync_selection (Gil *gil, int pos, SyncType type)
{
	GList *list;

	for (list = gil->icon_list; list; list = list->next) {
		if (GPOINTER_TO_INT (list->data) >= pos){
			int i = GPOINTER_TO_INT (list->data);
		
			switch (type) {
			case SYNC_INSERT:
				list->data = GINT_TO_POINTER (i+1);
				break;

			case SYNC_REMOVE:
				list->data = GINT_TO_POINTER (i-1);
				break;

			default:
				g_assert_not_reached ();
			}
		}
	}
}

static int
icon_event (Gil *gil, Icon *icon, GdkEvent *event)
{
	GList *l;
	int n;
	
	switch (event->type){
	case GDK_BUTTON_PRESS:
		gil->last_clicked = icon;
		
		if (icon->text->selected && (event->button.button == 1 || event->button.button == 3)){
			gil->last_clicked = icon;
		
			for (n = 0, l = gil->icon_list; l; l = l->next, n++)
				if (l->data == icon)
					break;
			
			gtk_signal_emit (GTK_OBJECT (gil), gil_signals [SELECT_ICON], n, event);
		} else {
			gil->last_clicked = NULL;
			toggle_icon (gil, icon, event);
		}
		return TRUE;

	case GDK_2BUTTON_PRESS:
		gil->last_clicked = NULL;
		toggle_icon (gil, icon, event);
		return TRUE;
		
	case GDK_BUTTON_RELEASE:
		if (gil->last_clicked == NULL)
			return FALSE;

		toggle_icon (gil, gil->last_clicked, event);
		return TRUE;

	default:
		return FALSE;
	}
}

static int
image_event (GnomeCanvasImage *img, GdkEvent *event, Icon *icon)
{
	Gil *gil = GIL (GNOME_CANVAS_ITEM (icon->text)->canvas);

	if (icon_event (gil, icon, event))
		return 1;
	
	return 0;
}

static int
text_event (GnomeIconTextItem *iti, GdkEvent *event, Icon *icon)
{
	Gil *gil = GIL (GNOME_CANVAS_ITEM (icon->text)->canvas);
				      
	if (icon_event (gil, icon, event))
		return 1;

	return 0;
}

static gboolean
text_changed (GnomeCanvasItem *item, Icon *icon)
{
	Gil *gil = GIL (item->canvas);
	gboolean accept = TRUE;
	int idx;

	idx = gil_icon_to_index (gil, icon);
	gtk_signal_emit (GTK_OBJECT (gil),
			 gil_signals [TEXT_CHANGED],
			 idx, gnome_icon_text_item_get_text (icon->text),
			 &accept);
	
	return accept;
}

static void
height_changed (GnomeCanvasItem *item, Icon *icon)
{
	Gil *gil = GIL (item->canvas);
	int n;
	
	if (!GTK_WIDGET_REALIZED (gil))
		return;

	if (gil->frozen){
		gil->dirty = TRUE;
		return;
	}

	n = gil_icon_to_index (gil, icon);
	gil_layout_from_line (gil, n / gil_get_items_per_line (gil));
	gil_scrollbar_adjust (gil);
}

static Icon *
icon_new_from_imlib (GnomeIconList *gil, GdkImlibImage *im, const char *text)
{
	GnomeCanvas *canvas = GNOME_CANVAS (gil);
	GnomeCanvasGroup *group = GNOME_CANVAS_GROUP (canvas->root);
	Icon *icon;

	icon = g_new (Icon, 1);

	icon->image = GNOME_CANVAS_IMAGE (gnome_canvas_item_new (
		group,
		gnome_canvas_image_get_type (),
		"x",      (double) 0,
		"y",      (double) 0,
		"width",  (double) im->rgb_width,
		"height", (double) im->rgb_height,
		"image",  im,
		NULL));

	icon->text = GNOME_ICON_TEXT_ITEM (gnome_canvas_item_new (
		group,
		gnome_icon_text_item_get_type (),
		NULL));

	gnome_icon_text_item_configure (
		icon->text,
		0, 0, gil->icon_width, NULL,
		text, gil->is_editable);

	icon->data = NULL;
	icon->destroy = NULL;

	gtk_signal_connect (
		GTK_OBJECT (icon->image), "event",
		GTK_SIGNAL_FUNC (image_event), icon);
	gtk_signal_connect_after (
		GTK_OBJECT (icon->text) , "event",
		GTK_SIGNAL_FUNC (text_event), icon);
	gtk_signal_connect (
		GTK_OBJECT (icon->text), "text_changed",
		GTK_SIGNAL_FUNC (text_changed), icon);
	gtk_signal_connect (
		GTK_OBJECT (icon->text), "height_changed",
		GTK_SIGNAL_FUNC (height_changed), icon);
	return icon;
}

static Icon *
icon_new (Gil *gil, const char *icon_filename, const char *text)
{
	GdkImlibImage *im;

	if (icon_filename)
		im = gdk_imlib_load_image ((char *)icon_filename);
	else
		im = NULL;
	return icon_new_from_imlib (gil, im, text);
}

static int
icon_list_append (Gil *gil, Icon *icon)
{
	int pos;
	
	pos = gil->icons++;
	gil->icon_list = g_list_append (gil->icon_list, icon);
	
	switch (gil->selection_mode) {
	case GTK_SELECTION_BROWSE:
		gnome_icon_list_select_icon (gil, 0);
		break;
		
	default:
		break;
	}

	if (!gil->frozen){
		/* FIXME: this should only layout the last line */
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	} else
		gil->dirty = TRUE;

	return gil->icons - 1;
}

static void
icon_list_insert (Gil *gil, int pos, Icon *icon)
{
	if (pos == gil->icons){
		icon_list_append (gil, icon);
		return;
	}
	gil->icon_list = g_list_insert (gil->icon_list, icon, pos);
	gil->icons++;

	switch (gil->selection_mode) {
	case GTK_SELECTION_BROWSE:
		gnome_icon_list_select_icon (gil, 0);
		break;
		
	default:
		break;
	}

	if (!gil->frozen){
		/*
		 * FIXME: this should only layout the lines from then
		 * one containing the Icon to the end.
		 */
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	} else
		gil->dirty = TRUE;

	sync_selection (gil, pos, SYNC_INSERT);
}

/**
 * gnome_icon_list_insert_imlib:
 * @gil:  the icon list.
 * @pos:  index where the icon should be inserted
 * @im:   Imlib image containing the icons
 * @text: text to display for the icon
 *
 * Inserts the at the POS index in the GIL icon list an icon which is
 * on the IM imlib imageand with the TEXT string as its label.
 */
void
gnome_icon_list_insert_imlib (GnomeIconList *gil, int pos, GdkImlibImage *im, const char *text)
{
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	icon = icon_new_from_imlib (gil, im, text);
	icon_list_insert (gil, pos, icon);
	return;
}

/**
 * gnome_icon_list_insert:
 * @gil:           the icon list.
 * @pos:           index where the icon should be inserted
 * @icon_filename: filename containing the graphic image for the icon.
 * @text:          text to display for the icon
 *
 * Inserts the at the POS index in the GIL icon list an icon which is
 * contained on the file icon_filename and with the TEXT string as its
 * label.
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
 * gnome_icon_list_append:
 * @gil:  the icon list.
 * @im:   Imlib image containing the icons
 * @text: text to display for the icon
 *
 * Appends to the GIL icon list an icon which is on the IM 
 * Imlib image and with the TEXT string as its label.
 */
int
gnome_icon_list_append_imlib (GnomeIconList *gil, GdkImlibImage *im, char *text)
{
	Icon *icon;

	g_return_val_if_fail (gil != NULL, -1);
	g_return_val_if_fail (IS_GIL (gil), -1);

	icon = icon_new_from_imlib (gil, im, text);
	return icon_list_append (gil, icon);
}

/**
 * gnome_icon_list_append:
 * @gil:           the icon list.
 * @icon_filename: filename containing the graphic image for the icon.
 * @text:          text to display for the icon
 *
 * Appends to the GIL icon list an icon which is contained on the file
 * icon_filename and with the TEXT string as its label.
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
		(*icon->destroy)(icon->data);

	gtk_object_destroy (GTK_OBJECT (icon->image));
	gtk_object_destroy (GTK_OBJECT (icon->text));
	g_free (icon);
}

/**
 * gnome_icon_list_remove:
 * @gil: the icon list
 * @pos: the index of the icon to remove
 *
 * Removes the icon at index position POS.
 */
void
gnome_icon_list_remove (GnomeIconList *gil, int pos)
{
	int was_selected;
	GList *list;
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (pos >= 0 && pos < gil->icons);

	was_selected = FALSE;

	list = g_list_nth (gil->icon_list, pos);
	icon = list->data;

	if (icon->text->selected){
		was_selected = TRUE;

		switch (gil->selection_mode) {
		case GTK_SELECTION_SINGLE:
		case GTK_SELECTION_BROWSE:
		case GTK_SELECTION_MULTIPLE:
			gnome_icon_list_unselect_icon (gil, pos);
			break;

		default:
			break;
		}
	}

	gil->icon_list = g_list_remove_link (gil->icon_list, list);
	g_list_free_1(list);
	gil->icons--;

	sync_selection (gil, pos, SYNC_REMOVE);

	if (was_selected) {
		switch (gil->selection_mode) {
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

	icon_destroy (icon);

	if (!gil->frozen) {
		/*
		 * FIXME: Optimize, only re-layout from pos to end
		 */
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	} else
		gil->dirty = TRUE;
}

/**
 * gnome_icon_list_clear:
 * @gil: the icon list to clear
 *
 * Clears the contents for the icon list
 */
void
gnome_icon_list_clear (GnomeIconList *gil)
{
	GList *l;
	
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	for (l = gil->icon_list; l; l = l->next)
		icon_destroy (l->data);
	gil_free_line_info (gil);

	gil->icon_list = NULL;
	gil->icons = 0;
}

static void
gil_destroy (GtkObject *object)
{
	Gil *gil;
	
	gil = GIL (object);
	
	g_free (gil->separators);

	gil->frozen = 1;
	gil->dirty  = TRUE;
	gnome_icon_list_clear (gil);

	gtk_object_unref (GTK_OBJECT (gil->adj));
	
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
select_icon (Gil *gil, int pos, GdkEvent *event)
{
	gint i;
	GList *list;
	Icon *icon;

	switch (gil->selection_mode) {
	case GTK_SELECTION_SINGLE:
	case GTK_SELECTION_BROWSE:
		i = 0;

		for (list = gil->icon_list; list; list = list->next) {
			icon = list->data;

			if ((i != pos) && (icon->text->selected))
				gtk_signal_emit (
					GTK_OBJECT (gil),
					gil_signals [UNSELECT_ICON],
					i, event);
			i++;
		}

		gtk_signal_emit (GTK_OBJECT (gil), gil_signals [SELECT_ICON],
				 pos, event);

		break;

	case GTK_SELECTION_MULTIPLE:
		gtk_signal_emit (GTK_OBJECT (gil), gil_signals [SELECT_ICON],
				 pos, event);
		break;

	case GTK_SELECTION_EXTENDED:
		break;

	default:
		break;
	}
}

/**
 * gnome_icon_list_select_icon:
 * @gil: The icon list.
 * @pos: The index of the icon to select.
 *
 * Selects the icon at index position POS.
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
	switch (gil->selection_mode) {
	case GTK_SELECTION_SINGLE:
	case GTK_SELECTION_BROWSE:
	case GTK_SELECTION_MULTIPLE:
		gtk_signal_emit (GTK_OBJECT (gil), gil_signals[UNSELECT_ICON],
				 pos, event);
		break;

	case GTK_SELECTION_EXTENDED:
		break;

	default:
		break;
	}
}

/**
 * gnome_icon_list_unselect_icon:
 * @gil: The icon list.
 * @pos: The index of the icon to unselect
 *
 * Removes the selection from the icon at the index position POS
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
	Gil *gil = GIL (widget);
	
	if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
		(* GTK_WIDGET_CLASS (parent_class)->size_allocate)
			(widget, allocation);
	if (gil->frozen)
		return;
	gil_layout_all_icons (gil);
	gil_scrollbar_adjust (gil);
}

static void
gil_realize (GtkWidget *widget)
{
	Gil *gil = GIL (widget);

	gil->frozen++;
	if (GTK_WIDGET_CLASS (parent_class)->realize)
		(* GTK_WIDGET_CLASS (parent_class)->realize)(widget);
	gil->frozen--;
	
	if (gil->frozen)
		return;
	if (gil->dirty){
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	}
}

static void
real_select_icon (Gil *gil, gint num, GdkEvent *event)
{
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (num >= 0 && num < gil->icons);

	icon = g_list_nth (gil->icon_list, num)->data;

	if (!icon->text->selected){
		gnome_icon_text_item_select (icon->text, 1);
		gil->selection = g_list_append (gil->selection, GINT_TO_POINTER (num));
	}
}

static void
real_unselect_icon (Gil *gil, gint num, GdkEvent *event)
{
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (num >= 0 && num < gil->icons);

	icon = g_list_nth (gil->icon_list, num)->data;

	if (icon->text->selected){
		gnome_icon_text_item_select (icon->text, 0);

		gil->selection = g_list_remove (gil->selection, GINT_TO_POINTER (num));
	}
}

/*
 * This routine could use some enhancements, for example, we could invoke
 * the item->point method to figure out if the rectangle is actually touching
 * a non-transparent pixel on the image. 
 */
static int
touches_item (GnomeCanvasItem *item, double x1, double y1, double x2, double y2)
{
	double ix1, iy1, ix2, iy2;
	
	gnome_canvas_item_get_bounds (item, &ix1, &iy1, &ix2, &iy2);

	return (MAX (x1, ix1) <= MIN (x2, ix2) && MAX (y1, iy1) <= MIN (y2, iy2));
}

static GList *
gil_get_icons_in_region (Gil *gil, double x1, double y1, double x2, double y2)
{
	GList *icons, *l;

	icons = NULL;
	
	for (l = gil->icon_list; l; l = l->next){
		Icon *icon = l->data;
		GnomeCanvasItem *image = GNOME_CANVAS_ITEM (icon->image);
		GnomeCanvasItem *text = GNOME_CANVAS_ITEM (icon->text);
		
		if (touches_item (image, x1, y1, x2, y2) ||
		    touches_item (text,  x1, y1, x2, y2)){
			icons = g_list_prepend (icons, icon);

		}
	}
	
	return icons;
}

static void
gil_mark_region (Gil *gil, GdkEvent *event, double x, double y)
{
	GList *icons, *l;
	double x1, x2, y1, y2;
	int idx;
	
	x1 = MIN (gil->sel_start_x, x);
	y1 = MIN (gil->sel_start_y, y);
	x2 = MAX (gil->sel_start_x, x);
	y2 = MAX (gil->sel_start_y, y);

	if (x1 < 0)
		x1 = 0;

	if (y1 < 0)
		y1 = 0;

	if (x2 >= GTK_WIDGET (gil)->allocation.width)
		x2 = GTK_WIDGET (gil)->allocation.width - 1;

	gnome_canvas_item_set (gil->sel_rect,
			       "x1", x1,
			       "y1", y1,
			       "x2", x2,
			       "y2", y2,
			       NULL);


	icons = gil_get_icons_in_region (gil, x1, y1, x2, y2);
		
	for (l = gil->icon_list, idx = 0; l; l = l->next, idx++){
		Icon *icon = l->data;

		if (icons && g_list_find (icons, icon)){
			if (!icon->text->selected)
				gtk_signal_emit (
					GTK_OBJECT (gil),
					gil_signals [SELECT_ICON],
					idx, event);
			
		} else if (icon->text->selected){
			int deselect = FALSE;
			
			if (gil->preserve_selection){
				void *v = GINT_TO_POINTER (idx);
				
				if (!g_list_find (gil->preserve_selection, v))
					deselect = TRUE;
			} else
				deselect = TRUE;

			if (deselect)
				gtk_signal_emit (
					GTK_OBJECT (gil),
					gil_signals [UNSELECT_ICON],
					idx, event);
		}
	}
	g_list_free (icons);
}

#define gray50_width 2
#define gray50_height 2
static const char gray50_bits[] = {
  0x02, 0x01, };

static gint
gil_button_press (GtkWidget *widget, GdkEventButton *event)
{
	Gil *gil = GIL (widget);
	int v;
	GdkBitmap *stipple;

	v = (*GTK_WIDGET_CLASS (parent_class)->button_press_event) (widget, event);

	if (v)
		return TRUE;

	if (gil->selection_mode != GTK_SELECTION_MULTIPLE)
		return FALSE;

	if (gil->sel_rect)     /* Already selecting */
	        return FALSE;

	gnome_canvas_window_to_world (
		GNOME_CANVAS (gil),
		event->x, event->y, 
		&gil->sel_start_x, &gil->sel_start_y);

	/*
	 * If the Shift or control keys are pressed, then keep a list
	 * of the current selection
	 */
	if (event->state & (GDK_SHIFT_MASK|GDK_CONTROL_MASK)){
		GList *l = NULL;

		for (l = gil->selection; l; l = l->next){
			void *data = l->data;
			
			gil->preserve_selection = g_list_prepend (gil->preserve_selection, data);
		}
	}

	stipple = gdk_bitmap_create_from_data (NULL, gray50_bits, gray50_width, gray50_height);
	gil->sel_rect = gnome_canvas_item_new (
		gnome_canvas_root (GNOME_CANVAS (gil)),
		gnome_canvas_rect_get_type (),
		"x1", (double) event->x,
		"y1", (double) event->y,
		"x2", (double) event->x,
		"y2", (double) event->y,
		"outline_color", "black",
		"width_pixels", 1,
		"outline_stipple", stipple,
		NULL);
	gdk_bitmap_unref (stipple);

	gnome_canvas_item_grab (gil->sel_rect, GDK_POINTER_MOTION_MASK | GDK_BUTTON_RELEASE_MASK, NULL, event->time);
	return TRUE;
}

static gint
gil_button_release (GtkWidget *widget, GdkEventButton *event)
{
	Gil *gil = GIL (widget);
	double x, y;

	if (gil->sel_rect){
		gnome_canvas_window_to_world (GNOME_CANVAS (gil), event->x, event->y, &x, &y);
		gil_mark_region (gil, (GdkEvent *) event, x, y);
		gnome_canvas_item_ungrab (gil->sel_rect, event->time);
		gtk_object_destroy (GTK_OBJECT (gil->sel_rect));
		gil->sel_rect = NULL;
	}

	g_list_free (gil->preserve_selection);
	gil->preserve_selection = NULL;

	if (gil->timer_tag != -1){
		gtk_timeout_remove (gil->timer_tag);
		gil->timer_tag = -1;
	}
	
	(*GTK_WIDGET_CLASS (parent_class)->button_release_event) (widget, event);

	return TRUE;
}

static gint
scroll_timeout (gpointer data)
{
	Gil *gil = data;
	double x, y;

	if (gil->adj->value + gil->value_diff > gil->adj->upper)
		return TRUE;
	
	gtk_adjustment_set_value (gil->adj, gil->adj->value + gil->value_diff);

	gnome_canvas_window_to_world (GNOME_CANVAS (gil), gil->event_last_x, gil->event_last_y, &x, &y);
	gil_mark_region (gil, NULL, x, y);

	return TRUE;
}

static gint
gil_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
	Gil *gil = GIL (widget);
	double x, y;
	
	if (!gil->sel_rect)
		return FALSE;

	gnome_canvas_window_to_world (GNOME_CANVAS (gil), event->x, event->y, &x, &y);
	gil_mark_region (gil, (GdkEvent *) event, x, y);
	
	/*
	 * If we are out of bounds, schedule a timeout that will do
	 * the scrolling
	 */
	if (event->y < 0 || event->y > widget->allocation.height){
		if (gil->timer_tag == -1){
			gil->timer_tag = gtk_timeout_add (
				30, scroll_timeout, gil);
		}

		if (event->y < 0)
			gil->value_diff = event->y;
		else
			gil->value_diff = event->y - widget->allocation.height;

		gil->event_last_x = event->x;
		gil->event_last_y = event->y;
		
		/*
		 * Make the steppings be relative to the mouse distance
		 * from the canvas.  Also notice the timeout above is small
		 * to give a more smooth movement
		 */
		gil->value_diff /= 5;
	} else {
		if (gil->timer_tag != -1){
			gtk_timeout_remove (gil->timer_tag);
			gil->timer_tag = -1;
		}
	}
	return TRUE;
}

typedef gboolean (*xGtkSignal_BOOL__INT_POINTER) (GtkObject * object,
						  gint     arg1,
						  gpointer arg2,
						  gpointer user_data);
static void 
xgtk_marshal_BOOL__INT_POINTER (GtkObject *object, GtkSignalFunc func, gpointer func_data, GtkArg *args)
{
  xGtkSignal_BOOL__INT_POINTER rfunc;
  gboolean *return_val;
  
  return_val = GTK_RETLOC_BOOL (args[2]);
  rfunc = (xGtkSignal_BOOL__INT_POINTER) func;
  *return_val = (*rfunc) (object,
			  GTK_VALUE_INT     (args [0]),
			  GTK_VALUE_POINTER (args [1]),
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

	gil = GNOME_ICON_LIST (object);

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
	GnomeCanvasClass *canvas_class;

	object_class = (GtkObjectClass *)   gil_class;
	widget_class = (GtkWidgetClass *)   gil_class;
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
	
	gil_signals [SELECT_ICON] =
		gtk_signal_new (
			"select_icon",
			GTK_RUN_FIRST,
			object_class->type,
			GTK_SIGNAL_OFFSET (GnomeIconListClass, select_icon),
			gtk_marshal_NONE__INT_POINTER,
			GTK_TYPE_NONE, 2,
			GTK_TYPE_INT,
			GTK_TYPE_GDK_EVENT);
	
	gil_signals [UNSELECT_ICON] =
		gtk_signal_new (
			"unselect_icon",
			GTK_RUN_FIRST,
			object_class->type,
			GTK_SIGNAL_OFFSET (GnomeIconListClass, unselect_icon),
			gtk_marshal_NONE__INT_POINTER,
			GTK_TYPE_NONE, 2,
			GTK_TYPE_INT,
			GTK_TYPE_GDK_EVENT);

	gil_signals [TEXT_CHANGED] =
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

	object_class->destroy              = gil_destroy;
	object_class->set_arg              = gil_set_arg;
	object_class->get_arg              = gil_get_arg;
	
	widget_class->size_request         = gil_size_request;
	widget_class->size_allocate        = gil_size_allocate;
	widget_class->realize              = gil_realize; 
	widget_class->button_press_event   = gil_button_press;
	widget_class->button_release_event = gil_button_release;
	widget_class->motion_notify_event  = gil_motion_notify;

	gil_class->select_icon             = real_select_icon;
	gil_class->unselect_icon           = real_unselect_icon;
}


static void
gil_init (Gil *gil)
{
	gil->row_spacing = DEFAULT_ROW_SPACING;
	gil->col_spacing = DEFAULT_COL_SPACING;
	gil->text_spacing = DEFAULT_TEXT_SPACING;
	gil->icon_border = DEFAULT_ICON_BORDER;
	gil->separators = g_strdup (" ");


	gil->mode = GNOME_ICON_LIST_TEXT_BELOW;
	gil->selection_mode = GTK_SELECTION_SINGLE;
	gil->frozen = 0;
	gil->dirty  = TRUE;
	gil->timer_tag = -1;

	/*
	 * FIXME: Figure out exactly why using canvas->height
	 * and canvas->width does not work
	 */
	gnome_canvas_set_scroll_region (GNOME_CANVAS (gil),
					0, 0, 1000000, 1000000);
	gnome_canvas_scroll_to (GNOME_CANVAS (gil), 0, 0);
}

/**
 * gnome_icon_list_get_type:
 *
 * Returns the type assigned for the GnomeIconList widget
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
 * @gil: The icon list
 * @w:   the new width for the icons
 *
 * Use this routine to change the icon width
 */
void
gnome_icon_list_set_icon_width (GnomeIconList *gil, int w)
{
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	gil->icon_width  = w;

	if (gil->frozen){
		gil->dirty = TRUE;
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

void
gnome_icon_list_set_hadjustment (GnomeIconList *gil, GtkAdjustment *hadj)
{
	GtkAdjustment *old_adjustment;
  
	/* hadj isn't used but is here for compatibility with GtkScrolledWindow */

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	if (hadj)
		g_return_if_fail (GTK_IS_ADJUSTMENT (hadj));

	if (gil->hadj == hadj)
		return;

	old_adjustment = gil->hadj;

	if (gil->hadj)
		gtk_object_unref (GTK_OBJECT (gil->hadj));

	gil->hadj = hadj;

	if (gil->hadj)
		gtk_object_ref (GTK_OBJECT (gil->hadj));

	if (!gil->hadj || !old_adjustment)
		gtk_widget_queue_resize (GTK_WIDGET (gil));
}

void
gnome_icon_list_set_vadjustment (GnomeIconList *gil, GtkAdjustment *vadj)
{
	GtkAdjustment *old_adjustment;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	if (vadj)
		g_return_if_fail (GTK_IS_ADJUSTMENT (vadj));

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
	}

	if (!gil->adj || !old_adjustment)
		gtk_widget_queue_resize (GTK_WIDGET (gil));
}

void           
gnome_icon_list_construct (GnomeIconList *gil, guint icon_width, GtkAdjustment *adj, gboolean is_editable)
{
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	gnome_icon_list_set_icon_width (gil, icon_width);
	gil->is_editable = is_editable;

	if (!adj)
		adj = GTK_ADJUSTMENT (gtk_adjustment_new (0, 0, 1, 0.1, 0.1, 0.1));
			
	gnome_icon_list_set_vadjustment (gil, adj);

}


/**
 * gnome_icon_list_new: [constructor]
 * @icon_width:  Icon width.
 * @adj:         Scrolling adjustment.
 * @is_editable: whether editing changes can be made to this icon_list.
 *
 * Creates a new GnomeIconList widget.  Icons will be assumed to be at 
 * most ICON_WIDTH pixels of width.  Any text displayed for those icons
 * will be wrapped at this width as well.
 * 
 * The adjustment is used to pass an existing adjustment to be used to
 * control the icon list display.  If ADJ is NULL, then a new adjustment
 * will be created.
 *
 * Applications can use this adjustment stored inside the
 * GnomeIconList structure to construct scrollbars if they so desire.
 *
 * If IS_EDITABLE is TRUE, then the text on the icons will be permited to
 * be edited.  If the name changes the "text_changed" signal will be emitted.
 *
 * Please note that the GnomeIconList starts life in Frozen state.  You are
 * supposed to fall gnome_icon_list_thaw on it as soon as possible.
 *
 */
GtkWidget *
gnome_icon_list_new (guint icon_width, GtkAdjustment *adj, gboolean is_editable)
{
	Gil *gil;

	gtk_widget_push_visual (gdk_imlib_get_visual ());
	gtk_widget_push_colormap (gdk_imlib_get_colormap ());
	gil = GIL (gtk_type_new (gnome_icon_list_get_type ()));
	gtk_widget_pop_visual ();
	gtk_widget_pop_colormap ();
	
	gnome_icon_list_construct (gil, icon_width, adj, is_editable);

	return GTK_WIDGET (gil);
}

/**
 * gnome_icon_list_freeze:
 * @gil:  The GnomeIconList
 *
 * Freezes any changes made to the GnomeIconList.  This is useful
 * to avoid expensive computations to be performed if you are making
 * many changes to an existing icon list.  For example, call this routine
 * before inserting a bunch of icons.
 *
 * You can call this routine multiple times, you will have to call
 * gnome_icon_list_thaw an equivalent number of times to make any
 * changes done to the icon list to take place.
 */
void
gnome_icon_list_freeze (GnomeIconList *gil)
{
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	gil->frozen++;

	/* We hide the root so that the user will not see any changes while the
	 * icon list is doing stuff.
	 */

	if (gil->frozen == 1)
		gnome_canvas_item_hide (GNOME_CANVAS (gil)->root);
}

/**
 * gnome_icon_list_thaw:
 * @gil:  The GnomeIconList
 *
 * If the freeze count reaches zero it will relayout any pending
 * layout changes that might have been delayed due to the icon list
 * be frozen by a call to gnome_icon_list_freeze.
 */
void
gnome_icon_list_thaw (GnomeIconList *gil)
{
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (gil->frozen > 0);

	gil->frozen--;

	if (!gil->dirty)
		return;

	gil_layout_all_icons (gil);
	gil_scrollbar_adjust (gil);

	if (gil->frozen == 0)
		gnome_canvas_item_show (GNOME_CANVAS (gil)->root);
}

/**
 * gnome_icon_list_set_selection_mode
 * @gil:  The GnomeIconList
 * @mode: Selection mode
 *
 * Sets the GnomeIconList selection mode, it can be any of the
 * modes defined in the GtkSelectionMode enumeration.
 */
void
gnome_icon_list_set_selection_mode (GnomeIconList *gil, GtkSelectionMode mode)
{
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	gil->selection_mode = mode;
	gil->last_selected = 0;
}

/**
 * gnome_icon_list_get_icon_data_full:
 * @gil:     The GnomeIconList
 * @pos:     icon index.
 * @data:    data to be set.
 * @destroy: Destroy notification handler
 *
 * Associates the DATA pointer to the icon at index POS.
 * When the Icon is destroyed, the handled specified in DESTROY
 * will be invoked.
 */
void
gnome_icon_list_set_icon_data_full (GnomeIconList *gil,
				     int pos, gpointer data,
				     GtkDestroyNotify destroy)
{
	Icon *icon;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (pos >= 0 && pos < gil->icons);

	icon = g_list_nth (gil->icon_list, pos)->data;
	icon->data = data;
	icon->destroy = destroy;
}

/**
 * gnome_icon_list_get_icon_data:
 * @gil:  The GnomeIconList
 * @pos:  icon index.
 * @data: data to be set.
 *
 * Associates the DATA pointer to the icon at index POS.
 */
void
gnome_icon_list_set_icon_data (GnomeIconList *gil, int pos, gpointer data)
{
	gnome_icon_list_set_icon_data_full (gil, pos, data, NULL);
}

/**
 * gnome_icon_list_get_icon_data:
 * @gil: The GnomeIconList
 * @pos: icon index. 
 *
 * Returns the per-icon data associated with the icon at index position POS
 */
gpointer
gnome_icon_list_get_icon_data (GnomeIconList *gil, int pos)
{
	Icon *icon;

	g_return_val_if_fail (gil != NULL, NULL);
	g_return_val_if_fail (IS_GIL (gil), NULL);
	g_return_val_if_fail (pos >= 0 && pos < gil->icons, NULL);
	
	icon = g_list_nth (gil->icon_list, pos)->data;
	return icon->data;
}

/**
 * gnome_icon_list_find_icon_from_data:
 * @gil:    The GnomeIconList
 * @data:   data pointer.
 *
 * Returns the index of the icon whose per-icon data has been set to
 * DATA.
 */
int
gnome_icon_list_find_icon_from_data (GnomeIconList *gil, gpointer data)
{
	GList *list;
	int n;
	Icon *icon;

	g_return_val_if_fail (gil != NULL, -1);
	g_return_val_if_fail (IS_GIL (gil), -1);

	for (n = 0, list = gil->icon_list; list; n++, list = list->next) {
		icon = list->data;
		if (icon->data == data)
			return n;
	}

	return -1;
}

static void
gil_set_if (Gil *gil, int n, int offset)
{
	int *v;

	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));

	v = (int *)(((char *)gil) + offset);

	if (*v == n)
		return;

	*v = n;

	if (!gil->frozen){
		gil_layout_all_icons (gil);
		gil_scrollbar_adjust (gil);
	} else
		gil->dirty = TRUE;
}

/**
 * gnome_icon_list_set_row_spacing:
 * @gil:    The GnomeIconList
 * @pixels: number of pixels for the inter-row spacing
 *
 * Sets the number of pixels for the inter-row spacing.
 */
void
gnome_icon_list_set_row_spacing (GnomeIconList *gil, int pixels)
{
	gil_set_if (gil, pixels, GTK_STRUCT_OFFSET (Gil, row_spacing));
}

/**
 * gnome_icon_list_set_col_spacing:
 * @gil:    The GnomeIconList
 * @pixels: number of pixels for the inter-column spacing.
 *
 * Sets the number of pixels for the inter-column spacing.
 */
void
gnome_icon_list_set_col_spacing (GnomeIconList *gil, int pixels)
{
	gil_set_if (gil, pixels, GTK_STRUCT_OFFSET (Gil, col_spacing));
}

/**
 * gnome_icon_list_set_text_spacing:
 * @gil:    The GnomeIconList
 * @pixels: number of pixels for the text spacing.
 *
 * Sets the number of pixels for the text spacing.
 */
void
gnome_icon_list_set_text_spacing (GnomeIconList *gil, int pixels)
{
	gil_set_if (gil, pixels, GTK_STRUCT_OFFSET (Gil, text_spacing));
}

/**
 * gnome_icon_list_set_icon_border:
 * @gil:    The GnomeIconList
 * @pixels: number of pixels for the border.
 *
 * Sets the number of pixels for the icon borders
 */
void
gnome_icon_list_set_icon_border (GnomeIconList *gil, int pixels)
{
	gil_set_if (gil, pixels, GTK_STRUCT_OFFSET (Gil, icon_border));
}

/**
 * gnome_icon_list_set_separators:
 * @gil: The GnomeIconList
 * @sep: A list of characters used to split the string
 *
 * Sets the separator characters used to optimally split this
 * string.  See gnome-icon-text.h
 */
void
gnome_icon_list_set_separators (GnomeIconList *gil, const char *sep)
{
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (sep != NULL);

	if (gil->separators)
		g_free (gil->separators);
	gil->separators = g_strdup (sep);

	if (gil->frozen){
		gil->dirty = TRUE;
		return;
	}
	gil_layout_all_icons (gil);
	gil_scrollbar_adjust (gil);
}

/**
 * gnome_icon_list_moveto:
 * @gil:    The GnomeIconList
 * @pos:    Icon index
 * @yalign: Desired alignement.
 *
 * Makes the icon whose index is POS visible on the screen, the
 * YALIGN double controls the alignment of the icon inside the GnomeIconList
 * 0.0 represents the top, while 1.0 represents the bottom.
 */
void
gnome_icon_list_moveto (GnomeIconList *gil, int pos, double yalign)
{
	IconLine *il;
	GList *l;
	int i, y, uh, line;
	
	g_return_if_fail (gil != NULL);
	g_return_if_fail (IS_GIL (gil));
	g_return_if_fail (pos >= 0 && pos < gil->icons);
	g_return_if_fail (yalign >= 0.0 && yalign <= 1.0);
	g_return_if_fail (gil->lines != NULL);
	
	line = pos / gil_get_items_per_line (gil);

	y = 0;
	for (i = 0, l = gil->lines; l && i < line; l = l->next, i++){
		il = l->data;

		y += icon_line_height (gil, il);
	}
	il = l->data;

	uh = GTK_WIDGET (gil)->allocation.height - icon_line_height (gil,il);
	gtk_adjustment_set_value (gil->adj, y - uh * yalign);
}

/**
 * gnome_icon_list_is_visible:
 * @gil: GnomeIconList
 * @pos: icon index
 *
 * Returns GTK_VISIBILITY_NONE if the POS icon is currently visible.
 * Returns GTK_VISIBILITY_PARTIAL if the POS icon is partically visible.
 * Returns GTK_VISIBILITY_FULL if the icon is completely visible
 */
GtkVisibility
gnome_icon_list_icon_is_visible (GnomeIconList *gil, int pos)
{
	IconLine *il;
	GList *l;
	int line, y1, y2, i;
	
	g_return_val_if_fail (gil != NULL, GTK_VISIBILITY_NONE);
	g_return_val_if_fail (IS_GIL (gil), GTK_VISIBILITY_NONE);
	g_return_val_if_fail (pos >= 0 && pos < gil->icons, GTK_VISIBILITY_NONE);

	if (gil->lines == NULL)
		return GTK_VISIBILITY_NONE;
	
	line = pos / gil_get_items_per_line (gil);
	y1 = 0;
	for (i = 0, l = gil->lines; l && i < line; l = l->next, i++){
		il = l->data;

		y1 += icon_line_height (gil, il);
	}
	y2 = y1 + icon_line_height (gil, (IconLine *) l->data);

	if (y2 < gil->adj->value)
		return GTK_VISIBILITY_NONE;

	if (y1 > gil->adj->value + GTK_WIDGET (gil)->allocation.height)
		return GTK_VISIBILITY_NONE;
	
	if (y2 <= gil->adj->value + GTK_WIDGET (gil)->allocation.height)
		return GTK_VISIBILITY_FULL;

	return GTK_VISIBILITY_PARTIAL;
}

/**
 * gnome_icon_list_get_icon_at:
 * @gil: the icon list
 * @x:   screen x position.
 * @y:   screen y position.
 *
 * Returns the icon index which is at x, y coordinates on the screen.  If
 * there is no icon at that location it will return -1.
 */
int
gnome_icon_list_get_icon_at (GnomeIconList *gil, int x, int y)
{
	GList *l;
	double dx, dy;
	double wx, wy;
	int n;
	
	g_return_val_if_fail (gil != NULL, -1);
	g_return_val_if_fail (IS_GIL (gil), -1);

	dx = x;
	dy = y;
	
	gnome_canvas_window_to_world (GNOME_CANVAS (gil), dx, dy, &wx, &wy);

	for (n = 0, l = gil->icon_list; l; l = l->next, n++){
		Icon *icon = l->data;
		GnomeCanvasItem *image = GNOME_CANVAS_ITEM (icon->image);
		GnomeCanvasItem *text  = GNOME_CANVAS_ITEM (icon->text);

		if (touches_item (image, wx, wy, wx, wy) ||
		    touches_item (text, wx, wy, wx, wy))
			return n;
	}

	return -1;
}

