/*
 * Minimal GUI editor Gtk container widget. 
 *
 * Miguel de Icaza (miguel@gnu.org)
 *
 * Licensed under the terms of the GNU LGPL
 */

#include <config.h>

#include <gtk/gtk.h>
#include "libgnome/libgnomeP.h"
#include <string.h>
#include "gtk-ted.h"

#define MAP_WIDGET_COL(c) 1+(c * 2)
#define MAP_WIDGET_ROW(r) 1+(r * 2)

#define TED_TOP_ROW  5
#define TED_TOP_COL  5

enum {
	user_widget, 
	frame_widget,
	label_widget,
	sep_widget
};

struct ted_widget_info {
	GtkWidget *widget;
	char      *name;
	GtkWidget *label_span_x, *label_span_y;
	int start_col, start_row, col_span, row_span;
	int  sticky;
	int  type;
};

#define STICK_N(x) (x & 1)
#define STICK_S(x) (x & 2)
#define STICK_E(x) (x & 4)
#define STICK_W(x) (x & 8)

typedef struct {
	int col, row;
	GtkTed *ted;
} row_col_t;

static char *app_name;

static void
gtk_ted_destroy (GtkObject *object)
{
	GtkTed *ted = GTK_TED (object);

	if (ted->widgets)
		g_hash_table_destroy (ted->widgets);
	if (ted->widget_box)
		gtk_widget_destroy (ted->widget_box);
	if (ted->dialog_name)
		g_free (ted->dialog_name);
}

static void
gtk_ted_class_init (GtkTedClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) class;

	object_class->destroy = gtk_ted_destroy;
}

static void
gtk_ted_init (GtkTed *ted)
{
	ted->dialog_name = NULL;
	ted->widget_box  = NULL;
	ted->need_gui = 0;
	ted->in_gui   = 0;
	ted->widgets = g_hash_table_new (g_str_hash, g_str_equal);
}

guint
gtk_ted_get_type ()
{
	static guint ted_type = 0;
	
	if (!ted_type){
		GtkTypeInfo ted_info = {
			"GtkTed",
			sizeof (GtkTed),
			sizeof (GtkTedClass),
			(GtkClassInitFunc) gtk_ted_class_init,
			(GtkObjectInitFunc) gtk_ted_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
		};
		
		ted_type = gtk_type_unique (gtk_table_get_type (), &ted_info);
	}
	
	return ted_type;
}

void
gtk_ted_set_app_name (const gchar *str)
{
	app_name = g_strdup (str);
}

static struct ted_widget_info *
gtk_ted_widget_info_new (GtkWidget *widget, const gchar *name, gint col, gint row)
{
 	struct ted_widget_info *wi;
	
	wi = g_new (struct ted_widget_info, 1);

	wi->widget = widget;
	wi->start_col = col;
	wi->start_row = row;
	wi->col_span  = 1;
	wi->row_span  = 1;
#if 0
	wi->flags_x   = GTK_FILL;
	wi->flags_y   = GTK_FILL;
	wi->tcb       = 1;
	wi->lcr       = 1;
#endif
	wi->sticky    = 0;
	wi->type      = user_widget;
	wi->name      = g_strdup (name);
	return wi;
}

static GtkWidget *
gtk_ted_align_new (GtkWidget *dest, struct ted_widget_info *wi)
{
	gfloat x_pos, y_pos, fill_x, fill_y;
	int s = wi->sticky;
	
	if (!dest)
		dest = gtk_alignment_new (0.0, 0.0, 0.0, 0.0);

	x_pos  = (!STICK_E (s) && !STICK_W (s)) ? 0.5 : (STICK_E (s) ? 1.0 : 0.0);
	y_pos  = (!STICK_N (s) && !STICK_S (s)) ? 0.5 : (STICK_S (s) ? 1.0 : 0.0);
	fill_x = (STICK_E(s) && STICK_W(s)) ? 1.0 : 0.0;
	fill_y = (STICK_N(s) && STICK_S(s)) ? 1.0 : 0.0;
	
	gtk_alignment_set (GTK_ALIGNMENT (dest), x_pos, y_pos, fill_x, fill_y);
	return dest;
}

/*
 * Place a widget on the table according to the values specified in WI
 */
static void
gtk_ted_attach (GtkTed *ted, GtkWidget *widget, struct ted_widget_info *wi)
{
	GtkWidget *align;
	
	if (!GTK_IS_ALIGNMENT (widget)){
		if (!GTK_IS_ALIGNMENT (widget->parent)){
			g_warning ("Mhm this should be an alignemnet\n");
			align = NULL;
		} else
			align = widget;
	} else
		align = widget;

	gtk_table_attach (GTK_TABLE (ted), align,
			  1+ wi->start_col * 2, 1+ (wi->start_col * 2)+(wi->col_span * 2)-1,
			  1+ wi->start_row * 2, 1+ (wi->start_row * 2)+(wi->row_span * 2)-1,
			  GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND,
			  0, 0);

	gtk_ted_align_new (align, wi);

			   
	/*
	 * Hackety hack: the frames and separators should be below any regular widget
	 * Lower their event box, and then lower all the control columns (the buttons).
	 */
	if (wi->type == frame_widget || wi->type == sep_widget){
		GList *child;
		GtkTableChild *tc;
		
		gdk_window_lower (GTK_BIN (widget)->child->window);

		for (child = ted->table.children; child; child = child->next){
			tc = (GtkTableChild *) child->data;

			if (!(tc->left_attach & 1) && GTK_IS_BUTTON (tc->widget))
				gdk_window_lower (tc->widget->window);
		}
	}
}

/*
 * Widget was dropped at drop_x, drop_y, put it on the table
 */
static int
gtk_ted_widget_drop (GtkTed *ted, GtkWidget *w, int drop_x, int drop_y)
{
	int x, y, col, row, xp, yp;
	struct ted_widget_info *wi;
	
	gdk_window_get_origin (GTK_WIDGET (ted)->window, &x, &y);
	x = drop_x - x;
	y = drop_y - y;

	for (xp = col = 0; col < ted->table.ncols-1; col++){
		if ((x > xp+ted->table. cols [col].allocation) &&
		    (x < xp+ted->table.cols [col+1].allocation+ted->table.cols [col].allocation)){
			break;
		}
		xp += ted->table.cols [col].allocation;
	}

	if (col == ted->table.ncols-1){
		return 0;
	}
	
	for (yp = row = 0; row < ted->table.nrows-1; row++){
		if ((y > yp+ted->table.rows [row].allocation) &&
		    (y < yp+ted->table.rows [row+1].allocation+ted->table.rows [row].allocation)){
			break;
		}
		yp += ted->table.rows [row].allocation;
	}

	if (row == ted->table.nrows-1){
		return 0;
	}

	col = (col/2) * 2;
	row = (row/2) * 2;

	gtk_widget_hide (w);
	gtk_widget_ref (w);
	gtk_widget_reparent (w, GTK_WIDGET (ted));
	gtk_container_remove (GTK_CONTAINER (ted), w);
	gtk_widget_show (w);
	
	wi = gtk_object_get_data (GTK_OBJECT (w), "ted_widget_info");
	if (wi == NULL){
		printf ("Fatal should not happen!\n");
		wi = gtk_ted_widget_info_new (w, "Unnamed", col/2, row/2);
		gtk_object_set_data (GTK_OBJECT (w), "ted_widget_info", wi);
	} else {
		wi->start_col = col/2;
		wi->start_row = row/2;
	}
	
	gtk_ted_attach (ted, w, wi);
	gtk_widget_unref (w);
	return 1;
}

/*
 * Handle the dragging part of the widget
 */
static void
moveme (GtkWidget *w, GdkEvent *event)
{
	GtkWidget *top;

	gtk_grab_remove (w);
	for (top = w; top->parent; top = top->parent)
		;
	gtk_widget_set_uposition (top, event->motion.x_root, event->motion.y_root);
}

/*
 * Widget dropped
 */
static void
releaseme (GtkWidget *w, GdkEventButton *event, GtkTed *ted)
{
	GtkWidget *real_widget;
	GtkWidget *parent, *top;

	for (top = w; top->parent; top = top->parent)
		;
	real_widget = w->parent;
	parent = real_widget->parent;
	
	gdk_pointer_ungrab (GDK_CURRENT_TIME);
	gdk_flush ();
	if (gtk_ted_widget_drop (ted, GTK_BIN (top)->child, event->x_root, event->y_root)){
		gtk_widget_destroy (top);
	}
}

/*
 * start dragging the widget
 */
static void
lift_me (GtkWidget *w, GdkEventButton *event)
{
	GtkWidget *top;

	top = gtk_window_new (GTK_WINDOW_POPUP);
	
	gtk_widget_realize (top);
	g_return_if_fail (GTK_IS_ALIGNMENT (w->parent));
	gtk_widget_reparent (w->parent, top);
	gtk_widget_set_uposition (top, (int) event->x_root, (int) event->y_root);
	gtk_widget_show (top);
	gdk_pointer_grab (w->window, FALSE, GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			  NULL, NULL, GDK_CURRENT_TIME);
	gtk_signal_emit_stop_by_name (GTK_OBJECT (w), "button_press_event");
}

/*
 * Connect the signals in the widget for drag and drop.
 */
static void
gtk_ted_prepare_editable_widget (struct ted_widget_info *wi, GtkWidget *ted_table)
{
	GtkWidget *w = wi->widget;
	GtkWidget *window;
	gint tag;
	
	/* window is the actual widget, as it is packed inside a GtkAdjustment */
	window = GTK_WIDGET (GTK_BIN (w)->child);

	/* Buttons have the "clicked" signal enabled, remove this one, this is
	 * a gmc-specific hack;  the right fix is to find a way to disconnect the
	 * "clicked" event
	 */
	tag = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (window), "click-signal-tag"));
	if (tag)
		gtk_signal_disconnect (GTK_OBJECT (window), tag);
	
	if (GTK_WIDGET_NO_WINDOW (window)){
		GtkWidget *eventbox;

		eventbox = gtk_event_box_new ();
		gtk_widget_ref (window);
		gtk_container_remove (GTK_CONTAINER (w), window);
		gtk_container_add (GTK_CONTAINER (eventbox), window);
		gtk_container_add (GTK_CONTAINER (w), eventbox);
		gtk_widget_unref (window);
		gtk_widget_show (eventbox);
		window = eventbox;
	}

	gtk_signal_connect (GTK_OBJECT (window), "button_press_event", GTK_SIGNAL_FUNC (lift_me), w);
	gtk_signal_connect (GTK_OBJECT (window), "motion_notify_event", GTK_SIGNAL_FUNC (moveme), ted_table);
	gtk_signal_connect (GTK_OBJECT (window), "button_release_event", GTK_SIGNAL_FUNC (releaseme), ted_table);

	gtk_widget_set_events (window, gtk_widget_get_events (window) |
			       GDK_BUTTON_RELEASE_MASK |
			       GDK_BUTTON1_MOTION_MASK |
			       GDK_BUTTON_PRESS_MASK);
}


static void
gtk_ted_free_rw (GtkWidget *w, void *data)
{
	g_free (data);
}

static void gtk_ted_separator_clicked (GtkWidget *w, row_col_t *rw);

static GtkWidget *
gtk_ted_separator (GtkTed *ted, int col, int row)
{
	GtkWidget *sep;
	row_col_t *rw;

	rw = g_new (row_col_t, 1);
	rw->col = col;
	rw->row = row;
	rw->ted = ted;
	sep = gtk_button_new ();
	gtk_signal_connect (GTK_OBJECT (sep), "clicked",
			    GTK_SIGNAL_FUNC (gtk_ted_separator_clicked), rw);
	gtk_signal_connect (GTK_OBJECT (sep), "destroy",
			    GTK_SIGNAL_FUNC (gtk_ted_free_rw), rw);
	return sep;
}

static void
gtk_ted_add_control_col_at (GtkTed *t, int col)
{
	GtkWidget *l;
	char buf [40];

	g_snprintf (buf, sizeof(buf), "%d", col);
	l = gtk_label_new (buf);
	gtk_table_attach (GTK_TABLE (t), l,
			  MAP_WIDGET_COL (col), MAP_WIDGET_COL (col)+1,
			  0, 1,
			  GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show (l);
}

static void
gtk_ted_add_control_row_at (GtkTed *t, int row)
{
	GtkWidget *l;
	char buf [40];

	g_snprintf (buf, sizeof(buf), "%d", row);
	l = gtk_label_new (buf);
	gtk_table_attach (GTK_TABLE (t), l,
			  0, 1,
			  MAP_WIDGET_ROW (row), MAP_WIDGET_ROW(row)+1,
			  GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show (l);
}

static void
gtk_ted_separator_row (GtkTed *ted, int row, int cols)
{
	GtkWidget *sep;

	sep = gtk_ted_separator (ted, -1, row);
	gtk_table_attach (GTK_TABLE (ted), sep,
			  0, cols * 2,
			  MAP_WIDGET_ROW (row) + 1, MAP_WIDGET_ROW (row) + 2,
			  GTK_EXPAND | GTK_FILL, 0, 0, 0);
	gtk_widget_show (sep);
}

static void
gtk_ted_separator_col (GtkTed *ted, int col, int rows)
{
	GtkWidget *sep;
	
	sep = gtk_ted_separator (ted, col, -1);
	gtk_table_attach (GTK_TABLE (ted), sep,
			  MAP_WIDGET_COL (col) + 1, MAP_WIDGET_COL (col) + 2,
			  0, rows * 2,
			  0, GTK_EXPAND | GTK_FILL, 0, 0);
	gtk_widget_show (sep);
}

/* Pretty ugly hacks that resize the button widgets inside the table */
static void
gtk_ted_resize_rows (GtkTed *ted)
{
	GList *child;
	GtkTableChild *tc;
	
	for (child = ted->table.children; child; child = child->next){
		tc = (GtkTableChild *) child->data;

		if (tc->left_attach != 0)
			continue;

		if (!GTK_IS_BUTTON (tc->widget))
			continue;
		
		tc->right_attach = ted->top_col * 2;
	}
}

static void
gtk_ted_resize_cols (GtkTed *ted)
{
	GList *child;
	GtkTableChild *tc;
	
	for (child = ted->table.children; child; child = child->next){
		tc = (GtkTableChild *) child->data;

		if (tc->top_attach != 0)
			continue;

		if (!GTK_IS_BUTTON (tc->widget))
			continue;
		
		tc->bottom_attach = ted->top_row * 2;
	}
}

static void
gtk_ted_separator_clicked (GtkWidget *w, row_col_t *rw)
{
	if (rw->row == -1){
		gtk_ted_separator_col (rw->ted, rw->ted->top_col, rw->ted->top_row);
		gtk_ted_add_control_col_at (rw->ted, rw->ted->top_col-1);
		rw->ted->top_col++;
		gtk_ted_resize_rows (rw->ted);
	} else {
		gtk_ted_separator_row (rw->ted, rw->ted->top_row, rw->ted->top_col);
		gtk_ted_add_control_row_at (rw->ted, rw->ted->top_row-1);
		rw->ted->top_row++;
		gtk_ted_resize_cols (rw->ted);
	}
}

static void
gtk_ted_add_divisions (GtkTed *ted, int cols, int rows)
{
	int row, col;
	
	for (row = 0; row < rows-1; row++)
		gtk_ted_separator_row (ted, row, cols);

	for (col = 0; col < cols-1; col++)
		gtk_ted_separator_col (ted, col, rows);
}

static void
gtk_ted_add_controls (GtkTed *t, int cols, int rows)
{
	int col, row;
	
	for (col = 0; col < cols; col++)
		gtk_ted_add_control_col_at (t, col);

	for (row = 0; row < rows; row++)
		gtk_ted_add_control_row_at (t, row);
}

static void
gtk_ted_input_activate (GtkWidget *entry, void *data)
{
	gtk_main_quit ();
}

static gchar *
gtk_ted_get_string (const gchar *title)
{
	GtkWidget *dialog, *box, *entry, *label;
	char *str;
	
	dialog = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	
	entry  = gtk_entry_new ();
	gtk_widget_show (entry);
	label  = gtk_label_new (title);
	gtk_widget_show (label);
	box = gtk_vbox_new (0, 0);
	gtk_widget_show (box);
	gtk_container_add (GTK_CONTAINER (box), label);
	gtk_container_add (GTK_CONTAINER (box), entry);
	gtk_container_add (GTK_CONTAINER (dialog), box);
	gtk_widget_show (dialog);
	gtk_signal_connect (GTK_OBJECT (entry), "activate", GTK_SIGNAL_FUNC (gtk_ted_input_activate), 0);
		
	gtk_grab_add (dialog);
	gtk_main ();
	gtk_grab_remove (dialog);

	str = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	gtk_widget_destroy (dialog);
	
	return str;
}

static void
gtk_ted_add_frame (GtkWidget *widget, GtkTed *ted)
{
	struct ted_widget_info *wi;
	GtkWidget *f, *e;
	gchar *name;
	gchar my_name [40];
	static gint frame_count;
	
	name = gtk_ted_get_string ("Type in frame title:");
	if (!name)
		return;
	e = gtk_event_box_new ();
	gtk_widget_show (e);
	f = gtk_frame_new (name);
	gtk_widget_show (f);
	gtk_container_add (GTK_CONTAINER (e), f);

	g_snprintf (my_name, sizeof(my_name), "Frame-%d", frame_count++);
	wi = gtk_ted_widget_info_new (e, my_name, 0, 0);
	wi->type = frame_widget;
	gtk_object_set_data (GTK_OBJECT (e), "ted_widget_info", wi);
	g_hash_table_insert (ted->widgets, wi->name, wi);
	gtk_ted_add (ted, e, my_name);
}

static void
gtk_ted_add_separator (GtkWidget *widget, GtkTed *ted)
{
	struct ted_widget_info *wi;
	GtkWidget *e, *s;
	gchar my_name [40];
	static gint sep_count;
	
	e = gtk_event_box_new ();
	gtk_widget_show (e);
	s = gtk_hseparator_new ();
	gtk_widget_show (s);
	gtk_container_add (GTK_CONTAINER (e), s);

	g_snprintf (my_name, sizeof(my_name), "Separator-%d", sep_count++);
	wi = gtk_ted_widget_info_new (e, my_name, 0, 0);
	g_hash_table_insert (ted->widgets, wi->name, wi);
	wi->type = sep_widget;
	gtk_object_set_data (GTK_OBJECT (e), "ted_widget_info", wi);
	
	gtk_container_set_border_width (GTK_CONTAINER (e), 2);
	gtk_ted_add (ted, e, my_name);
}

static gchar *
gtk_ted_render_pos (gint f)
{
	static gchar buf [8];
	gchar *p = buf;

	if (STICK_S (f))
		*p++ = 's';
	if (STICK_N (f))
		*p++ = 'n';
	if (STICK_E (f))
		*p++ = 'e';
	if (STICK_W (f))
		*p++ = 'w';
	*p = 0;
	return buf;
}

static int
gtk_ted_parse_pos (gchar *str)
{
	gint flags;

	flags = 0;
	
	for (;*str; str++){
		if (*str == 'n')
			flags |= 1;
		if (*str == 's')
			flags |= 2;
		if (*str == 'e')
			flags |= 4;
		if (*str == 'w')
			flags |= 8;
	}
	return flags;
}

static void
gtk_ted_save (GtkWidget *widget, GtkTed *ted)
{
	GList *child;
	gchar *filename;
	
	filename = gtk_ted_get_string ("Output file name (use 'layout' for quickly load/save testing)");
	if (!filename)
		return;
	
	for (child = ted->table.children; child; child = child->next){
		GtkTableChild *tc = (GtkTableChild *) child->data;
		struct ted_widget_info *wi; 
		char *key, *kind;
		char buffer [60];
		
		wi = gtk_object_get_data (GTK_OBJECT (tc->widget), "ted_widget_info");
		if (!wi)
			continue;

		switch (wi->type){
		case user_widget:
			kind = "Widget";
			break;

		case frame_widget:
			kind = "Frame";
			break;

		case sep_widget:
			kind = "Separator";
			break;

		case label_widget:
			kind = "Label";
			break;
		default:
			kind = "UNKNOWN";
			break;
		}

		key = g_strconcat ("=", filename, "=/", ted->dialog_name, "-", kind, "-", wi->name, "/", NULL);
		gnome_config_push_prefix (key);
		g_snprintf (buffer, sizeof(buffer), "%d,%d,%d,%d",
			    wi->start_col, wi->start_row,
			    wi->col_span, wi->row_span);
		gnome_config_set_string ("geometry", buffer);
#if 0
		gnome_config_set_string ("flags_x", gtk_ted_render_flags (wi->flags_x, wi->lcr));
		gnome_config_set_string ("flags_y", gtk_ted_render_flags (wi->flags_y, wi->tcb));
#endif
		gnome_config_set_string ("flags=", gtk_ted_render_pos (wi->sticky));
		if (wi->type == label_widget){
			
			gnome_config_set_string ("text", GTK_LABEL (GTK_BIN (GTK_BIN (wi->widget)->child)->child)->label);
		}
		if (wi->type == frame_widget){
			gnome_config_set_string ("text", GTK_FRAME (GTK_BIN (GTK_BIN (wi->widget)->child)->child)->label);
		}
		g_free (key);
		gnome_config_pop_prefix ();
	}
	g_free (filename);
	gnome_config_sync ();
}

static GtkWidget *
gtk_ted_control_bar_new (GtkTed *ted)
{
	GtkWidget *hbox, *b;

	hbox = gtk_hbox_new (0, 0);

	/* Frame add */
	b = gtk_button_new_with_label ("New frame");
	gtk_widget_show (b);
	gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (gtk_ted_add_frame), ted);
	gtk_container_add (GTK_CONTAINER (hbox), b);

	/* Separator add */
	b = gtk_button_new_with_label ("New separator");
	gtk_widget_show (b);
	gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (gtk_ted_add_separator), ted);
	gtk_container_add (GTK_CONTAINER (hbox), b);

	/* Label add */
	b = gtk_button_new_with_label ("New label");
	gtk_widget_show (b);
	gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (gtk_ted_add_separator), ted);
	gtk_container_add (GTK_CONTAINER (hbox), b);
	
	/* Save */
	b = gtk_button_new_with_label ("Save layout");
	gtk_widget_show (b);
	gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (gtk_ted_save), ted);
	gtk_container_add (GTK_CONTAINER (hbox), b);
	
	return hbox;
}

static void
gtk_ted_init_divisions (GtkTed *ted)
{
	gtk_ted_add_controls (ted, ted->top_col+1, ted->top_row+1);
	gtk_ted_add_divisions (ted, ted->top_col+1, ted->top_row+1);
}

static void
gtk_ted_init_editor (GtkTed *ted)
{
	GtkWidget *control_window, *control_bar;

	control_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	ted->widget_box = gtk_table_new (TED_TOP_ROW+2, TED_TOP_COL, 0);
	gtk_widget_show (ted->widget_box);
	gtk_container_add (GTK_CONTAINER (control_window), ted->widget_box);
	gtk_widget_show (control_window);
	gtk_widget_realize (control_window);

	control_bar = gtk_ted_control_bar_new (ted);
	gtk_widget_show (control_bar);
	gtk_table_attach_defaults (GTK_TABLE (ted->widget_box), control_bar,
				   0, TED_TOP_COL-1, TED_TOP_ROW, TED_TOP_ROW+1);
	
	ted->last_col = 0;
	ted->last_row = 0;
	ted->top_col = TED_TOP_COL;
	ted->top_row = TED_TOP_ROW;
}

static struct ted_widget_info *
gtk_ted_load_widget (GtkTed *ted, char *prefix, char *secname)
{
	char *full = g_strconcat (prefix, "/", ted->dialog_name, "-", secname, "/", NULL);
	struct ted_widget_info *wi = g_new (struct ted_widget_info, 1);
	char *str, *s;
	
	wi->widget = 0;
	
	gnome_config_push_prefix (full);
	str = gnome_config_get_string ("geometry");
	sscanf (str, "%d,%d,%d,%d", &wi->start_col, &wi->start_row, &wi->col_span, &wi->row_span);
	g_free (str);
	wi->sticky  = gtk_ted_parse_pos (s = gnome_config_get_string ("flags"));
	g_free (s);
	wi->type = user_widget;
	wi->name = g_strdup (secname + 7);
	gnome_config_pop_prefix ();
	g_free (full);
	return wi;
}

static GtkWidget *
gtk_ted_wrap (GtkWidget *widget, struct ted_widget_info *wi)
{
	GtkWidget *align;

	align = gtk_ted_align_new (NULL, wi);
	gtk_widget_show (align);
	gtk_container_add (GTK_CONTAINER (align), widget);
	gtk_widget_show (widget);
	return align;
}

static struct ted_widget_info *
gtk_ted_load_frame (GtkTed *ted, char *prefix, char *secname)
{
	GtkWidget *w;
	char *full = g_strconcat (prefix, "/", ted->dialog_name, "-", secname, "/", NULL);
	struct ted_widget_info *wi = g_new (struct ted_widget_info, 1);
	char *str, *p;
	
	wi->widget = 0;
	
	gnome_config_push_prefix (full);
	str = gnome_config_get_string ("geometry");
	sscanf (str, "%d,%d,%d,%d", &wi->start_col, &wi->start_row, &wi->col_span, &wi->row_span);
	g_free (str);
	wi->sticky = gtk_ted_parse_pos (str = gnome_config_get_string ("flags"));
	g_free (str);
	wi->type = frame_widget;
	wi->widget = gtk_ted_wrap (w = gtk_frame_new (p = gnome_config_get_string ("text")), wi);
	gtk_object_set_data (GTK_OBJECT (wi->widget), "ted_widget_info", wi);
	gtk_widget_show (wi->widget);
	g_free (p);
	wi->name = g_strdup (secname + 6);
	gnome_config_pop_prefix ();
	g_free (full);
	return wi;
}

static void
gtk_ted_load_layout (GtkTed *ted, const gchar *layout_file)
{
	struct ted_widget_info *wi;
	char *name;
	gchar *full, *sec_name, *layout;
	void *iter;
	int  len = strlen (ted->dialog_name);
	char *p;
			
	if (layout_file)
		full = g_strdup(layout_file);
	else {
		name = g_strconcat ("layout/", app_name, NULL);
		full = gnome_datadir_file (name);
		g_free (name);
	}
	
	if (g_file_exists (full)){
		layout = g_strconcat ("=", full, "=", NULL);
	} else {
		layout = g_strdup ("=./layout=");
	}

	g_free (full);

	iter = gnome_config_init_iterator_sections (layout);
	while ((iter = gnome_config_iterator_next (iter, &sec_name, NULL)) != NULL){
		if (strncmp (sec_name, ted->dialog_name, len))
			continue;

		if (strncmp (sec_name+len+1, "Widget-", 7) == 0){
			if ((p = strchr (sec_name+len+8, '|')) != NULL)
				*p = 0;

			wi = gtk_ted_load_widget (ted, layout, sec_name+len+1);
			g_hash_table_insert (ted->widgets, wi->name, wi);
			if (p)
				*p = '|';
		}
		if (strncmp (sec_name+len+1, "Frame-", 6) == 0){
			wi = gtk_ted_load_frame (ted, layout, sec_name+len+1);
			g_hash_table_insert (ted->widgets, wi->name, wi);
		}

		if (strncmp (sec_name+len+1, "Separator-", 10) == 0){
			wi = gtk_ted_load_frame (ted, layout, sec_name+len+1);
			g_hash_table_insert (ted->widgets, wi->name, wi);
		}
	}
	g_free (layout);
}

static void
gtk_ted_update_position (struct ted_widget_info *wi)
{
	if (GTK_IS_TABLE (wi->widget->parent)){
		GtkTed *ted = GTK_TED (wi->widget->parent);
		char buf [40];

		gtk_widget_ref (wi->widget);
		gtk_container_remove (GTK_CONTAINER (wi->widget->parent), wi->widget);
		gtk_ted_attach (ted, wi->widget, wi);
		gtk_widget_unref (wi->widget);

		g_snprintf (buf, sizeof(buf), "%d", wi->col_span);
		gtk_label_set_text (GTK_LABEL (wi->label_span_x), buf);
		g_snprintf (buf, sizeof(buf), "%d", wi->row_span);
		gtk_label_set_text (GTK_LABEL (wi->label_span_y), buf);
	}
}

static void
gtk_ted_click_y_more (GtkWidget *widget, struct ted_widget_info *wi)
{
	wi->row_span++;
	gtk_ted_update_position (wi);
}

static void
gtk_ted_click_y_less (GtkWidget *widget, struct ted_widget_info *wi)
{
	if (wi->row_span > 1){
		wi->row_span--;
		gtk_ted_update_position (wi);
	}
}

static void
gtk_ted_click_x_more (GtkWidget *widget, struct ted_widget_info *wi)
{
	wi->col_span++;
	gtk_ted_update_position (wi);
}

static void
gtk_ted_click_x_less (GtkWidget *widget, struct ted_widget_info *wi)
{
	if (wi->col_span > 1){
		wi->col_span--;
		gtk_ted_update_position (wi);
	}
}

struct gtk_ted_checkbox_info {
	struct ted_widget_info *wi;
	int is_y;
	int value;
	
};

#if 0
static void
gtk_ted_checkbox_toggled (GtkWidget *widget, struct gtk_ted_checkbox_info *ci)
{
#if 0
	if (ci->is_y)
		ci->wi->flags_y ^= ci->value;
	else
		ci->wi->flags_x ^= ci->value;
#endif
	gtk_ted_update_position (ci->wi);
}

static void
gtk_ted_orient_cb (GtkWidget *w, void *data)
{
	/*
	if (strcmp (data, "Left") == 0){
		wi->lcr = index;
	} else
		wi->tcb = index;
	gtk_ted_update_position (wi);
	*/
}

static void
gtk_ted_setup_radio (GtkWidget *vbox, GtkWidget *widget, gchar *str, struct ted_widget_info *wi, gint idx, gint active)
{
	if (active)
		gtk_widget_set_state (GTK_WIDGET (widget), GTK_STATE_ACTIVE);
	else
		gtk_widget_set_state (GTK_WIDGET (widget), GTK_STATE_NORMAL);

	gtk_signal_connect (GTK_OBJECT (widget), "toggled",
			    GTK_SIGNAL_FUNC (gtk_ted_orient_cb), str);
	gtk_object_set_data (GTK_OBJECT (widget), "ted_wi", wi);
	gtk_object_set_data (GTK_OBJECT (widget), "index", GINT_TO_POINTER (idx));
			     
	gtk_box_pack_start_defaults (GTK_BOX (vbox), widget);
	gtk_widget_show (widget);
}
#endif

static GtkWidget *
gtk_ted_span_control (char *str, struct ted_widget_info *wi, int is_y)
{
	GtkWidget *more, *less, *hbox, *vbox, *frame, *label;
	char buf [40];

	vbox = gtk_vbox_new (0, 0);
	gtk_widget_show (vbox);

	frame = gtk_frame_new (str);
	gtk_widget_show (frame);
	hbox = gtk_hbox_new (0, 0);
	gtk_widget_show (hbox);
	
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_container_add (GTK_CONTAINER (vbox), frame);
	
	
	gtk_container_set_border_width (GTK_CONTAINER (frame), 2);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 2);
	more  = gtk_button_new_with_label ("+");
	gtk_widget_show (more);
	gtk_box_pack_end_defaults (GTK_BOX (hbox), more);

	g_snprintf (buf, sizeof(buf), "%d", is_y ? wi->row_span : wi->col_span);
	label = gtk_label_new (buf);
	gtk_box_pack_end_defaults (GTK_BOX (hbox), label);
	gtk_widget_show (label);
	if (is_y)
		wi->label_span_y = label;
	else
		wi->label_span_x = label;
	
	less  = gtk_button_new_with_label ("-");
	gtk_widget_show (less);
	gtk_box_pack_end_defaults (GTK_BOX (hbox), less);

	gtk_signal_connect (GTK_OBJECT (more), "clicked",
			    GTK_SIGNAL_FUNC ((is_y? gtk_ted_click_y_more : gtk_ted_click_x_more)), wi);
	gtk_signal_connect (GTK_OBJECT (less), "clicked",
			    GTK_SIGNAL_FUNC ((is_y? gtk_ted_click_y_less : gtk_ted_click_x_less)), wi);

	return vbox;
}

static void
gtk_ted_pos_toggle (GtkWidget *widget, struct ted_widget_info *wi)
{
	int val = GPOINTER_TO_INT (gtk_object_get_data (GTK_OBJECT (widget), "value"));

	wi->sticky ^= val;
	gtk_ted_update_position (wi);
}

static void
gtk_ted_pos_prep (GtkWidget *box, gchar *str, gint val, struct ted_widget_info *wi)
{
	GtkWidget *w;
	
	w = gtk_check_button_new_with_label (str);
	GTK_TOGGLE_BUTTON (w)->active =  (wi->sticky & val) ? 1 : 0;
	gtk_box_pack_start_defaults (GTK_BOX (box), w);
	gtk_widget_show (w);
	gtk_signal_connect (GTK_OBJECT (w), "toggled", GTK_SIGNAL_FUNC (gtk_ted_pos_toggle), wi);
	gtk_object_set_data (GTK_OBJECT (w), "value", GINT_TO_POINTER (val));
}

static GtkWidget *
gtk_ted_pos_control (struct ted_widget_info *wi)
{
	GtkWidget *vbox;

	vbox = gtk_vbox_new (0, 0);
	gtk_ted_pos_prep (vbox, "North", 1, wi);
	gtk_ted_pos_prep (vbox, "South", 2, wi);
	gtk_ted_pos_prep (vbox, "East", 4, wi);
	gtk_ted_pos_prep (vbox, "West", 8, wi);

	gtk_widget_show (vbox);
	return vbox;
}

static GtkWidget *
gtk_ted_size_control_new (struct ted_widget_info *wi)
{
	GtkWidget *vbox, *hbox;
		
	vbox = gtk_vbox_new (0, 0);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (vbox), gtk_ted_span_control ("span x", wi, 0), 0, 0, 0);
	gtk_box_pack_start (GTK_BOX (vbox), gtk_ted_span_control ("span y", wi, 1), 0, 0, 0);
	hbox = gtk_hbox_new (0, 0);
	gtk_widget_show (hbox);

	gtk_box_pack_start_defaults (GTK_BOX (hbox), gtk_ted_pos_control (wi));
	/*
	gtk_box_pack_start_defaults (GTK_BOX (hbox), gtk_ted_nse_control (wi, "Top", "Bottom", wi->tcb));
	gtk_box_pack_start_defaults (GTK_BOX (hbox), gtk_ted_nse_control (wi, "Left", "Right", wi->lcr));
	*/
	gtk_box_pack_start_defaults (GTK_BOX (vbox), hbox);

	return vbox;
}

static void
gtk_ted_widget_options (GtkWidget *widget, struct ted_widget_info *wi)
{
	GtkWidget *control, *window;

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	control = gtk_ted_size_control_new (wi);
	gtk_container_add (GTK_CONTAINER (window), control);
	gtk_widget_show (window);
}

/*
 * Add the widgets to the control window.
 *
 */
static void
gtk_ted_widget_control_new (GtkTed *ted, GtkWidget *widget, char *name)
{
	GtkWidget *vbox, *l, *hbox, *control;
	struct ted_widget_info *wi;
	int col, row;
	
	l = gtk_frame_new (name);
	gtk_widget_show (l);
	hbox = gtk_hbox_new (0, 0);
	gtk_widget_show (hbox);
	vbox = gtk_vbox_new (0, 0);
	gtk_widget_show (vbox);
	gtk_container_add (GTK_CONTAINER (l), vbox);
	control = gtk_button_new_with_label ("Options...");
	gtk_widget_show (control);
	gtk_box_pack_start_defaults (GTK_BOX (vbox), control);

	wi = gtk_object_get_data (GTK_OBJECT (widget), "ted_widget_info");
	row = ted->last_row;
	col = ted->last_col;
	gtk_table_attach_defaults (GTK_TABLE (ted->widget_box), l,
				   col, col+1, row, row+1);
	
	if (wi == NULL){
		wi = gtk_ted_widget_info_new (widget, name, 0, 0);
		g_hash_table_insert (ted->widgets, wi->name, wi);
		gtk_object_set_data (GTK_OBJECT (widget), "ted_widget_info", wi);
		gtk_box_pack_start_defaults (GTK_BOX (vbox), widget);
	} else {
		gtk_ted_attach (ted, wi->widget, wi);
	}
	
	gtk_signal_connect (GTK_OBJECT (control), "clicked", GTK_SIGNAL_FUNC (gtk_ted_widget_options), wi);

	col++;
	if (col >= ted->top_col){
		col = 0;
		row++;
	}
	ted->last_row = row;
	ted->last_col = col;
}

void
gtk_ted_add (GtkTed *ted, GtkWidget *widget, const gchar *original_name)
{
	GtkWidget *align;
	struct ted_widget_info *wi;
	gchar *name = g_strdup (original_name), *p;

	if ((p = strchr (name, '|')) != NULL){
		*p = 0;
	}
	align = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
	gtk_widget_show (align);
	gtk_container_add (GTK_CONTAINER (align), widget);
	widget = align;
	
	if ((wi = g_hash_table_lookup (ted->widgets, name)) != NULL){
		wi->widget = widget;
		gtk_ted_align_new (widget, wi);
	} else {
		ted->need_gui = 1;
		wi = gtk_ted_widget_info_new (widget, name, 0, 0);
		g_hash_table_insert (ted->widgets, wi->name, wi);
	}
	gtk_object_set_data (GTK_OBJECT (widget), "ted_widget_info", wi);

	/* Insertions when we are running the GUI should be properly prepared */
	if (ted->in_gui){
		gtk_ted_prepare_editable_widget (wi, GTK_WIDGET (ted));
		gtk_ted_widget_control_new (ted, wi->widget, name);
	}
	g_free (name);
}

static void
gtk_ted_prepare_widgets_edit (gpointer key, gpointer value, gpointer user_data)
{
	GtkTed *ted = user_data;
	struct ted_widget_info *wi = value;
	int top_col, top_row;
	
	/* This is a widget that had layout information, but does not exist in this
	 * instance of the execution
	 */
	if (wi->widget == 0)
		return;
	
	gtk_ted_prepare_editable_widget (wi, GTK_WIDGET (ted));
	gtk_ted_widget_control_new (ted, wi->widget, key);
	top_col = wi->start_col + wi->col_span;
	top_row = wi->start_row + wi->row_span;
	ted->top_col = MAX (ted->top_col, wi->start_col + wi->col_span);
	ted->top_row = MAX (ted->top_row, wi->start_row + wi->row_span);
}

static void
gtk_ted_setup_layout (gpointer key, gpointer value, gpointer user_data)
{
	GtkTed *ted = user_data;
	struct ted_widget_info *wi = value;
	int top_col, top_row;

	/* If the widget does no longer exist, but it is present on the layout
	 * file, return
	 */
	if (!wi->widget)
		return;
	
	top_col = wi->start_col + wi->col_span;
	top_row = wi->start_row + wi->row_span;
	
	gtk_table_attach (GTK_TABLE (ted), wi->widget,
			  wi->start_col, top_col,
			  wi->start_row, top_row, 
			  GTK_FILL | GTK_EXPAND, GTK_FILL | GTK_EXPAND, 0, 0);
	
	if (top_col > ted->top_col)
		ted->top_col = top_col;
	if (top_row > ted->top_row)
		ted->top_row = top_row;
	
	gtk_ted_align_new (wi->widget, wi);
	ted->top_col = MAX (ted->top_col, wi->start_col + wi->col_span);
	ted->top_row = MAX (ted->top_row, wi->start_row + wi->row_span);
}

static void
gtk_ted_set_spacings (GtkTed *ted)
{
	int i;
	
	for (i = 0; i < ted->top_col; i++){
		GtkWidget *spacer;

		spacer = gtk_hbox_new (0, 0);
		gtk_widget_set_usize (spacer, 5, 5);
		gtk_table_attach (GTK_TABLE (ted), spacer,
				  i, i+1, 0, 1, 0, 0, 0, 0);
		gtk_widget_show (spacer);
	}
	
	for (i = 0; i < ted->top_row; i++){
		GtkWidget *spacer;

		spacer = gtk_hbox_new (0, 0);
		gtk_widget_set_usize (spacer, 5, 5);
		gtk_table_attach (GTK_TABLE (ted), spacer,
				  0, 1, i, i+1, 0, 0, 0, 0);
		gtk_widget_show (spacer);
	}
	gtk_table_set_row_spacings (GTK_TABLE (ted), 5);
	gtk_table_set_col_spacings (GTK_TABLE (ted), 5);
}

void
gtk_ted_prepare (GtkTed *ted)
{
	char   *env;

	env = getenv ("TEDIT");
	if (env && (strcmp (env, ted->dialog_name) == 0))
			ted->need_gui = 1;

	if (ted->need_gui){
		gtk_ted_init_editor (ted);
		ted->in_gui = 1;
		g_hash_table_foreach (ted->widgets, gtk_ted_prepare_widgets_edit, ted);
		gtk_ted_init_divisions (ted);
	} else {
		ted->top_col = ted->top_row = 0;
		g_hash_table_foreach (ted->widgets, gtk_ted_setup_layout, ted);
		gtk_ted_set_spacings (ted);
	}
}

GtkWidget *
gtk_ted_new_layout (const gchar *name, const gchar *layout)
{
	GtkTed *ted;

	ted = gtk_type_new (gtk_ted_get_type ());
	ted->dialog_name = g_strdup (name);
	gtk_ted_load_layout (ted, layout);
	gtk_container_set_border_width (GTK_CONTAINER (ted), 6);
	
	return GTK_WIDGET (ted);
}

GtkWidget *
gtk_ted_new (const gchar *name)
{
	return gtk_ted_new_layout (name, NULL);
}


/*
  n    	-> align (0.5, 0.0, fill_x, fill_y)
  s    	-> align (0.5, 1.0, fill_x, fill_y)
  e    	-> align (1.0, 0.5, fill_x, fill_y)
  w    	-> align (0.0, 0.5, fill_x, fill_y)

  ns   	-> align (0.5, 0.5, fill_x, fill_y)
  ew   	-> align (0.5, 0.5, fill_x, fill_y)

  ne    -> align (1.0, 0.0, fill_x, fill_y);
  nw    -> align (0.0, 0.0, fill_x, fill_y);

  se    -> align (1.0, 1.0, fill_x, fill_y);
  sw    -> align (0.0, 1.0, fill_x, fill_y);

  nsew  -> align (0.5, 0.5, fill_x, fill_y)

  fill_x = (e && w ? 1.0 : 0.0);
  fill_y = (n && s ? 1.0 : 0.0);

  x_pos  = (!e && !w) ? 0.5 : (e ? 1.0 : 0.0);
  y_pos  = (!n && !s) ? 0.5 : (s ? 1.0 : 0.0);
 */
