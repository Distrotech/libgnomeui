/*
 * text-item.c: implements a wrapping text display and editing Canvas
 * Item for the Gnome Icon List.  
 *
 * Author:
 *   Miguel de Icaza (miguel@gnu.org).
 *
 * Fixme: Provide a ref-count fontname caching like thing.  */
#include <gnome.h>
#include <gdk/gdkkeysyms.h>
#include <libgnomeui/gnome-icon-item.h>

/* Margins used to display the information */
#define MARGIN_X 2
#define MARGIN_Y 2

/* Aliases to minimize screen use in my laptop */
#define ITI(x)       GNOME_ICON_TEXT_ITEM(x)
#define ITI_CLASS(x) GNOME_ICON_TEXT_ITEM_CLASS(x)
#define IS_ITI(x)    GNOME_IS_ICON_TEXT_ITEM(x)

typedef GnomeIconTextItem Iti;

static GnomeCanvasItem *parent_class;

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

static char *
iti_default_font_name (void)
{
	return "-adobe-helvetica-medium-r-normal--*-100-*-*-*-*-*-*";
}

static void
font_load (Iti *iti)
{
	if (iti->fontname){
		iti->font = gdk_font_load (iti->fontname);
		g_free (iti->fontname);
		iti->fontname = NULL;
	}
	
	if (!iti->font){
		if (!default_font)
			default_font = gdk_font_load (iti_default_font_name ());
		
		iti->font = gdk_font_ref (default_font);
	}
}

static void
iti_queue_redraw (Iti *iti)
{
	GnomeCanvasItem *item = GNOME_CANVAS_ITEM (iti);
	
	gnome_canvas_request_redraw (
		item->canvas,
		item->x1, item->y1,
		item->x2+1, item->y2+1);
}

static char *
iti_separators (Iti *iti)
{
	return " \t-.[]#";
}

static int
iti_get_width (Iti *iti, int *center_offset)
{
	int w, x = 0;

	if (!center_offset)
		center_offset = &x;
	
	if (iti->ti && iti->ti->rows) {
		if (iti->ti->rows->next){
			*center_offset = 0;
			w = iti->width;
		} else {
			GnomeIconTextInfoRow *row = iti->ti->rows->data;
			
			w = iti->ti->width;
			*center_offset =  (iti->width - row->width) / 2;
		}
	} else { 
		w = 0;
		*center_offset = iti->width/2 + MARGIN_X;
	}

	return w;
}

static void
get_bounds (Iti *iti, int *x1, int *y1, int *x2, int *y2)
{
	int w;

	w = iti_get_width (iti, NULL);

	*x1 = iti->x + (iti->width - w) / 2 - MARGIN_X;
	*y1 = iti->y;
	*x2 = iti->x + (iti->width - w) / 2 + w + 2 * MARGIN_X;
	*y2 = 2 * MARGIN_Y + iti->y + (iti->ti ? iti->ti->height : 0);
}

static void
recompute_bounding_box (Iti *iti)
{
	GnomeCanvasItem *item = GNOME_CANVAS_ITEM (iti);
	int nx1, ny1, nx2, ny2;
	int height_changed, width_changed;
	double dx, dy;

	dx = dy = 0.0;

	gnome_canvas_item_i2w (item, &dx, &dy);

	get_bounds (iti, &nx1, &ny1, &nx2, &ny2);
	nx1 += dx;
	ny1 += dy;
	nx2 += dx + 1;
	ny2 += dy + 1;

	/* See if our dimenssions match the item bounding box */
	if (!(nx1 != item->x1 || ny1 != item->y1 || nx2 != item->x2 || ny2 != item->y2))
		return;

	height_changed = (ny2-ny1) != (item->y2-item->y1);
	width_changed = (nx2-nx1) != (item->x2-item->x1);

	item->x1 = nx1;
	item->y1 = ny1;
	item->x2 = nx2;
	item->y2 = ny2;

	gnome_canvas_group_child_bounds (
		GNOME_CANVAS_GROUP (item->parent), item);

	if (height_changed && iti->ti)
		gtk_signal_emit (GTK_OBJECT (iti), iti_signals [HEIGHT_CHANGED]);

	if (width_changed && iti->ti)
		gtk_signal_emit (GTK_OBJECT (iti), iti_signals [WIDTH_CHANGED]);
}

/*
 * layout_text:
 *
 * Relayouts the text and computes the bounding box
 */
static inline void
layout_text (Iti *iti)
{
	char *text;
	
	if (iti->ti)
		gnome_icon_text_info_free (iti->ti);

	if (iti->editing)
		text = gtk_entry_get_text (iti->entry);
	else
		text = iti->text;
	
	iti->ti = gnome_icon_layout_text (
		iti->font, text, iti_separators (iti), iti->width, TRUE);

	recompute_bounding_box (iti);
}

/*
 * iti_stop_editing:
 *
 * Puts the Iti on the editing = FALSE state
 */
static void
iti_stop_editing (Iti *iti)
{
	iti->editing = FALSE;
	
	gtk_widget_destroy (iti->entry_top);
	iti->entry = NULL;
	iti->entry_top = NULL;

	iti_queue_redraw (iti);
	gtk_signal_emit (GTK_OBJECT (iti), iti_signals[EDITING_STOPPED]);
}

/*
 * iti_edition_accept:
 *
 * Invoked to carry out all of the changes due to the user
 * pressing enter while editing, or due to us loosing the focus
 */
static void
iti_edition_accept (Iti *iti)
{
	gboolean accept = TRUE;
	
	gtk_signal_emit (GTK_OBJECT (iti), iti_signals [TEXT_CHANGED], &accept);

	iti_queue_redraw (iti);
	if (accept){
		if (iti->is_text_allocated){
			g_free (iti->text);
		}
		iti->text = g_strdup (gtk_entry_get_text (iti->entry));
		iti->is_text_allocated = 1;
	}

	iti_stop_editing (iti);
	layout_text (iti);
	iti_queue_redraw (iti);
}

static void
iti_entry_activate (GtkWidget *entry, Iti *iti)
{
	iti_edition_accept (iti);
}

/*
 * iti_start_editing
 *
 * Puts the Iti on the editing mode
 */
static void
iti_start_editing (Iti *iti)
{
	if (iti->editing)
		return;

	/*
	 * Trick: The actual edition of the entry takes place in a
	 * GtkEntry which is placed offscreen.  That way we get all of the
	 * advantages from GtkEntry without duplicating code.
	 */
	iti->entry = (GtkEntry *) gtk_entry_new ();
	gtk_entry_set_text (iti->entry, iti->text);
	gtk_signal_connect (GTK_OBJECT (iti->entry), "activate",
			    GTK_SIGNAL_FUNC (iti_entry_activate), iti);
	
	iti->entry_top = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_container_add (GTK_CONTAINER (iti->entry_top), (GTK_WIDGET (iti->entry)));
	gtk_widget_set_uposition (iti->entry_top, 20000, 20000);
	gtk_widget_show_all (iti->entry_top);

	gtk_editable_select_region (GTK_EDITABLE (iti->entry), 0, -1);
	iti->editing = TRUE;

	gtk_signal_emit (GTK_OBJECT (iti), iti_signals[EDITING_STARTED]);
}

/*
 * Allocates the X resources used by the Iti.
 */
static void
iti_realize (GnomeCanvasItem *item)
{
	Iti *iti = ITI (item);
	
	if (GNOME_CANVAS_ITEM_CLASS (parent_class)->realize)
		(*GNOME_CANVAS_ITEM_CLASS (parent_class)->realize)(item);
	
	/* We can be realized before we are configured */
	if (!iti->text)
		return;
	
	font_load (iti);
	layout_text (iti);
}

/*
 * iti_unrealize:
 *
 * Release the X resources we used
 */
static void
iti_unrealize (GnomeCanvasItem *item)
{
	Iti *iti = ITI (item);

	if (iti->editing)
		iti_stop_editing (iti);
	
	if (iti->font){
		gdk_font_unref (iti->font);
		iti->font = NULL;
	}

	if (iti->ti){
		gnome_icon_text_info_free (iti->ti);
		iti->ti = NULL;
	}

	if (GNOME_CANVAS_ITEM_CLASS (parent_class)->unrealize)
		(*GNOME_CANVAS_ITEM_CLASS (parent_class)->unrealize)(item);
}

/*
 * iti_destroy
 *
 * Destructor for Iti objects
 */
static void
iti_destroy (GtkObject *object)
{
	Iti *iti = ITI (object);

	if (iti->fontname)
		g_free (iti->fontname);
	if (iti->text){
		if (iti->is_text_allocated)
			g_free (iti->text);
	}
	
	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(*GTK_OBJECT_CLASS (parent_class)->destroy)(object);
}

static void
iti_update (GnomeCanvasItem *item, double *affine, ArtSVP *clip_path, int flags)
{
}

/*
 * Draw the text
 */
static void
iti_paint_text (Iti *iti, GdkDrawable *drawable, int x, int y, GtkJustification just)
{
	GtkWidget *canvas = GTK_WIDGET (GNOME_CANVAS_ITEM (iti)->canvas);
        GnomeIconTextInfoRow *row;
	GnomeIconTextInfo *ti;
	GtkStyle *style = canvas->style;
	GdkGC *fg_gc, *bg_gc;
	GdkGC *gc, *bgc, *sgc, *bsgc;
        GList *item;
        int xpos, len;
	double dx, dy;

	dx = dy = 0.0;
	gnome_canvas_item_i2w (GNOME_CANVAS_ITEM (iti), &dx, &dy);
	
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

        for (item = ti->rows; item; item = item->next, len += (row ? strlen (row->text) : 0)){
		char *text;
		
		row = item->data;

                if (!row) {
			y += ti->baseline_skip / 2;
			continue;
		}
		
		text = row->text;
		
		switch (just) {
		case GTK_JUSTIFY_LEFT:
			xpos = 0;
			break;
			
		case GTK_JUSTIFY_RIGHT:
			xpos = ti->width - row->width;
			break;
			
		case GTK_JUSTIFY_CENTER:
			xpos = (iti->width - row->width) / 2;
			break;
			
		default:
                                /* Anyone care to implement GTK_JUSTIFY_FILL? */
		  g_warning ("Justification type %d not supported.  Using "
			     "left-justification.", (int) just);
			xpos = 0;
		}

		/*
		 * Draw the insertion cursor if we are in editting mode.
		 */
		if (iti->editing){
			int cursor, offset, i;
			int sel_start, sel_end;

			sel_start = GTK_EDITABLE (iti->entry)->selection_start_pos - len;
			sel_end   = GTK_EDITABLE (iti->entry)->selection_end_pos - len;
			offset = 0;
			cursor = GTK_EDITABLE (iti->entry)->current_pos - len;
			for (i = 0; *text; text++, i++){
				int   size, px;
				
				size = gdk_text_width (ti->font, text, 1);
				
				if (i >= sel_start && i < sel_end){
					fg_gc = sgc;
					bg_gc = bsgc;
				} else {
					fg_gc = gc;
					bg_gc = bgc;
				}

				px = x + xpos + offset;
				gdk_draw_rectangle (
					drawable, bg_gc, TRUE,
					dx + px, dy + y - ti->font->ascent,
					size, ti->baseline_skip);
						    
				gdk_draw_text (
					drawable, ti->font, fg_gc,
					dx + px, dy + y, text, 1);

				if (cursor == i)
					gdk_draw_line (
						drawable, gc,
						dx + px-1, dy + y - ti->font->ascent,
						dx + px-1, dy + y + ti->font->descent-1);

				offset += size;
			}
			if (cursor == i){
				int px = x + xpos + offset;
				
				gdk_draw_line (
					drawable, gc,
					dx + px-1, dy + y - ti->font->ascent,
					dx + px-1, dy + y + ti->font->descent-1);
			}
		} else {
			int xtra = iti->selected ? 2 : 0;
			fg_gc = iti->selected ? sgc : gc;
			bg_gc = iti->selected ? bsgc : bgc;

			gdk_draw_rectangle (drawable, bg_gc, TRUE,
					    dx + x + xpos - xtra,
					    dy + y - ti->font->ascent - xtra,
					    gdk_string_width (ti->font, text) + xtra * 2,
					    ti->baseline_skip + xtra*2);
			gdk_draw_string (drawable, ti->font, fg_gc, dx + x + xpos, dy + y, text);
		}
			
		y += ti->baseline_skip;
        }
}

static void
iti_draw (GnomeCanvasItem *item, GdkDrawable *drawable, int x, int y, int width, int height)
{
	Iti *iti = ITI (item);
	int rx = iti->x - x;
	int ry = iti->y - y;
	GtkWidget *canvas = GTK_WIDGET (GNOME_CANVAS_ITEM (iti)->canvas);
	GdkGC *gc;
		
	gc = canvas->style->fg_gc [GTK_STATE_NORMAL];
	
	if (iti->editing){
		int h, w, center_offset;

		h = iti->ti->height;
		if (!h)
			h = iti->font->ascent + iti->font->descent;

		w = iti_get_width (iti, &center_offset);

		gdk_draw_rectangle (drawable, gc, FALSE,
				    rx + center_offset, ry,
				    w + MARGIN_X*2, h + MARGIN_Y*2);
	}

	iti_paint_text (iti, drawable,
			rx + MARGIN_X, ry + MARGIN_Y,
			GTK_JUSTIFY_CENTER);
}

static double
iti_point (GnomeCanvasItem *item, double x, double y, int cx, int cy,
		 GnomeCanvasItem **actual_item)
{
	*actual_item = NULL;
	
	if (cx < item->x1 || cy < item->y1)
		return INT_MAX;
	if (cx > item->x2 || cy > item->y2)
		return INT_MAX;

	*actual_item = item;
	return 0.0;
}

/*
 * iti_idx_from_x_y:
 *
 * Given X, Y, a mouse position, return a valid index
 * inside the edited text
 */
static int
iti_idx_from_x_y (Iti *iti, int x, int y)
{
        GnomeIconTextInfoRow *row;
	int lines = g_list_length (iti->ti->rows);
	int line, col, i, idx;
	GList *l;

	line = y / iti->ti->baseline_skip;
	if (lines < line+1)
		line = lines-1;

	/* Compute the base index for this line */
	for (l = iti->ti->rows, idx = i = 0; i < line; l = l->next, i++){
		row = l->data;

		idx += strlen (row->text);
	}
	
	row = g_list_nth (iti->ti->rows, line)->data;
	col = 0;
	if (row != NULL){
		int first_char = ((iti->canvas_item.x2 - iti->canvas_item.x1) - row->width)/2;
		int last_char  = first_char + row->width;
		
		if (x < first_char){
			/* nothing */
		} else if (x > last_char){
			col = strlen (row->text);
		} else {
			char *s = row->text;
			int pos = first_char;
			
			while (pos < last_char){
				pos += gdk_text_width (iti->ti->font, s, 1);
				if (pos > x)
					break;
				col++;
				s++;
			}
		}
	} 

	idx += col;

	g_assert (idx <= GTK_ENTRY (iti->entry)->text_size);

	return idx;
}

/*
 * iti_start_selecting:
 *
 * Button has been pressed in the Iti: start the selection.
 */
static void
iti_start_selecting (Iti *iti, int idx, guint32 event_time)
{
	GtkEditable *e = GTK_EDITABLE (iti->entry);
	GdkCursor *ibeam;
	
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

	iti_queue_redraw (iti);

	gtk_signal_emit (GTK_OBJECT (iti), iti_signals[SELECTION_STARTED]);
}

/*
 * iti_stop_selecting
 *
 * The user released the button.
 */
static void
iti_stop_selecting (Iti *iti, guint32 event_time)
{
	GnomeCanvasItem *item = GNOME_CANVAS_ITEM (iti);
	GtkEditable *e = GTK_EDITABLE (iti->entry);

	gnome_canvas_item_ungrab (item, event_time);
	e->has_selection = FALSE;
	iti->selecting = FALSE;

	iti_queue_redraw (iti);
	gtk_signal_emit (GTK_OBJECT (iti), iti_signals[SELECTION_STOPPED]);
}

/*
 * iti_selection_motion:
 *
 * 
 */
static void
iti_selection_motion (Iti *iti, int idx)
{
	GtkEditable *e = GTK_EDITABLE (iti->entry);

	if (idx < e->current_pos){
		e->selection_start_pos = idx;
		e->selection_end_pos   = e->current_pos;
	} else {
		e->selection_start_pos = e->current_pos;
		e->selection_end_pos  = idx;
	}
	
	iti_queue_redraw (iti);
}

static void
iti_translate (GnomeCanvasItem *item, double dx, double dy)
{
}

static gint
iti_event (GnomeCanvasItem *item, GdkEvent *event)
{
	Iti *iti = ITI (item);
	int idx;
	double x, y;

	switch (event->type){
	case GDK_KEY_PRESS:
		if (!iti->editing)
			return FALSE;

		iti_queue_redraw (iti);
		if (event->key.keyval == GDK_Escape)
			iti_stop_editing (iti);
		else
			gtk_widget_event (GTK_WIDGET (iti->entry), event);

		layout_text (iti);
		iti_queue_redraw (iti);
		return TRUE;

	case GDK_BUTTON_PRESS:
		if (!iti->is_editable)
			return FALSE;

		if (!iti->selected) {
			iti->unselected_click = TRUE;
			return FALSE;
		} else {
			iti->unselected_click = FALSE;
		}
		
		if (iti->editing){
			x = event->button.x - item->x1;
			y = event->button.y - item->y1;
			idx = iti_idx_from_x_y (iti, x, y);
			
			iti_start_selecting (iti, idx, event->button.time); 
		}
		break;

	case GDK_MOTION_NOTIFY:
		if (!iti->is_editable)
			return FALSE;

		if (!iti->entry)
			return FALSE;

		if (!iti->selecting)
			return FALSE;

		x = event->motion.x - item->x1;
		y = event->motion.y - item->y1;
		idx = iti_idx_from_x_y (iti, x, y);
		iti_selection_motion (iti, idx);
		break;
			
	case GDK_BUTTON_RELEASE:
		if (!iti->is_editable)
			return FALSE;

		if (!iti->entry) {
			if (iti->unselected_click || iti->editing)
				return FALSE;

			gnome_canvas_item_grab_focus (item);
			iti_start_editing (iti);
			iti_queue_redraw (iti);
		} else {
			iti_stop_selecting (iti, event->button.time);
		}

		break;
		
	case GDK_FOCUS_CHANGE:
		if (iti->editing && event->focus_change.in == FALSE)
			iti_edition_accept (iti);
		break;

	default:
		return FALSE;
	}
	return TRUE;
}

static void
iti_bounds (GnomeCanvasItem *item, double *x1, double *y1, double *x2, double *y2)
{
	Iti *iti = ITI (item);
	int ix1, iy1, ix2, iy2;

	/* If we have not been realized, realize us */
	if (iti->ti == NULL)
		iti_realize (item);

	get_bounds (iti, &ix1, &iy1, &ix2, &iy2);

	*x1 = ix1;
	*y1 = iy1;
	*x2 = ix2;
	*y2 = iy2;
}

static void
iti_class_init (GnomeIconTextItemClass *text_item_class)
{
	GtkObjectClass  *object_class;
	GnomeCanvasItemClass *item_class;

	parent_class = gtk_type_class (gnome_canvas_item_get_type());
	
	object_class = (GtkObjectClass *) text_item_class;
	item_class   = (GnomeCanvasItemClass *) text_item_class;

	/* object class method overrides */
	object_class->destroy = iti_destroy;
	
	/* GnomeCanvasItem method overrides */
	item_class->realize     = iti_realize;
	item_class->unrealize   = iti_unrealize;
	item_class->update      = iti_update;
	item_class->draw        = iti_draw;
	item_class->point       = iti_point;
	item_class->translate   = iti_translate;
	item_class->event       = iti_event;
	item_class->bounds      = iti_bounds;
	
	/* Our signals */
	iti_signals [TEXT_CHANGED] =
		gtk_signal_new (
			"text_changed",
			GTK_RUN_LAST,
			object_class->type,
			GTK_SIGNAL_OFFSET(GnomeIconTextItemClass,text_changed),
			gtk_marshal_BOOL__NONE,
			GTK_TYPE_BOOL, 0);
				
	iti_signals [HEIGHT_CHANGED] =
		gtk_signal_new (
			"height_changed",
			GTK_RUN_LAST,
			object_class->type,
			GTK_SIGNAL_OFFSET(GnomeIconTextItemClass,height_changed),
			gtk_marshal_NONE__NONE,
			GTK_TYPE_NONE, 0);

	iti_signals [WIDTH_CHANGED] =
		gtk_signal_new (
			"width_changed",
			GTK_RUN_LAST,
			object_class->type,
			GTK_SIGNAL_OFFSET(GnomeIconTextItemClass,width_changed),
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
}

/**
 * gnome_icon_text_item_setxy:
 * @iti:  The GnomeIconTextItem object
 * @x: canvas x position
 * @y: canvas y position
 *
 * Puts the GnomeIconTextItem in the canvas location specified by
 * the @x and @y parameters
 */
void
gnome_icon_text_item_setxy (GnomeIconTextItem *iti, int x, int y)
{
	g_return_if_fail (iti != NULL);
	g_return_if_fail (IS_ITI (iti));

	iti->x = x;
	iti->y = y;

	iti_queue_redraw (iti);
	recompute_bounding_box (iti);
	iti_queue_redraw (iti);
}

/**
 * gnome_icon_text_item_configure:
 * @iti:   The GnomeIconTextItem object
 * @x:     Canvas position to place the object
 * @y:     Canvas position to place the object
 * @width: The allowed width for this object, in pixels
 * @fontname: Font that should be used to display the text
 * @text:   The text that is going to be displayed.
 * @is_editable: Whether editing is enabled for this item
 *
 * This routine is used to configure a GnomeIconTextItem.
 */
void
gnome_icon_text_item_configure (GnomeIconTextItem *iti, int x, int y,
				int width, const char *fontname,
				const char *text,
				gboolean is_editable, gboolean is_static)
{
	g_return_if_fail (iti != NULL);
	g_return_if_fail (IS_ITI (iti));
	g_return_if_fail (text != NULL);
	
	iti->x = x;
	iti->y = y;
	iti->width = width - MARGIN_X * 2;
	iti->is_editable = is_editable;
	
	if (iti->text){
		if (iti->is_text_allocated)
			g_free (iti->text);
	}

	iti->is_text_allocated = !is_static;
	if (is_static)
		iti->text = text;
	else
		iti->text = g_strdup (text);
	
	if (iti->fontname)
		g_free (iti->fontname);
	iti->fontname = g_strdup (fontname);

	if (iti->font){
		gdk_font_unref (iti->font);
		iti->font = NULL;
	}

	if (GTK_OBJECT (iti)->flags & GNOME_CANVAS_ITEM_REALIZED){
		font_load (iti);
		layout_text (iti);
	}
}

/**
 * gnome_icon_text_item_select:
 * @iti: The GnomeIconTextItem object
 * @sel: boolean flag, if true the text should be displayed as selected
 *       otherwise not
 *
 * This is used to control the way the selection is displayed for a 
 * GnomeIconTextItem object
 */
void
gnome_icon_text_item_select (GnomeIconTextItem *iti, int sel)
{
	g_return_if_fail (iti != NULL);
	g_return_if_fail (IS_ITI (iti));

	if (iti->selected == sel)
		return;

	iti->selected = sel;

	if (iti->selected == 0 && iti->editing)
		iti_stop_editing (iti);

	iti_queue_redraw (iti);
}

/**
 * gnome_icon_text_item_get_text:
 * @iti: The GnomeIconTextItem object
 *
 * Returns the text contained to a GnomeIconTextItem
 */
char *
gnome_icon_text_item_get_text (GnomeIconTextItem *iti)
{
	g_return_val_if_fail (iti != NULL, NULL);
	g_return_val_if_fail (IS_ITI (iti), NULL);

	if (iti->editing)
		return gtk_entry_get_text (iti->entry);
	else
		return iti->text;
}

void
gnome_icon_text_item_stop_editing (GnomeIconTextItem *iti,
				   gboolean accept)
{
	g_return_if_fail (iti != NULL);
	g_return_if_fail (IS_ITI (iti));

	if (accept)
		iti_edition_accept (iti);
	else
		iti_stop_editing (iti);
}

GtkType
gnome_icon_text_item_get_type (void)
{
	static GtkType iti_type = 0;

	if (!iti_type) {
		GtkTypeInfo iti_info = {
			"GnomeIconTextItem",
			sizeof (GnomeIconTextItem),
			sizeof (GnomeIconTextItemClass),
			(GtkClassInitFunc) iti_class_init,
			(GtkObjectInitFunc) NULL,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		iti_type = gtk_type_unique (gnome_canvas_item_get_type (), &iti_info);
	}

	return iti_type;
}

