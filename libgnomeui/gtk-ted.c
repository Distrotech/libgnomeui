/*
 * Minimal GUI editor Gtk container widget. 
 *
 * Miguel de Icaza (miguel@gnu.org)
 *
 * Licensed under the terms of the GNU LGPL
 */

#include <gnome.h>
#include <string.h>
#include "gtk-ted.h"

#define MAP_WIDGET_COL(c) 1+(c * 2)
#define MAP_WIDGET_ROW(r) 1+(r * 2)

#define TED_TOP_ROW  5
#define TED_TOP_COL  5

enum {
	user_widget, 
	frame_widget,
	sep_widget
};

struct ted_widget_info {
	GtkWidget *widget;
	char      *name;
	GtkWidget *label_span_x, *label_span_y;
	char start_col, start_row, col_span, row_span;
	int  flags_x, flags_y;
	int  type;
};

typedef struct {
	int col, row;
	GtkTed *ted;
} row_col_t;

static char *app_name;

static GtkTableClass *parent_class = NULL;

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

GtkWidget *
gtk_ted_init (GtkTed *ted)
{
	GtkWidget *table;

	ted->dialog_name = NULL;
	ted->widget_box = NULL;
	ted->need_gui = 0;
	ted->in_gui   = 0;
	
	ted->widgets = g_hash_table_new (g_string_hash, g_string_equal);

	return table;
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
gtk_ted_set_app_name (char *str)
{
	app_name = g_strdup (str);
}

static struct ted_widget_info *
gtk_ted_widget_info_new (GtkWidget *widget, char *name, int col, int row)
{
 	struct ted_widget_info *wi;
	
	wi = g_new (struct ted_widget_info, 1);

	wi->widget = widget;
	wi->start_col = col;
	wi->start_row = row;
	wi->col_span  = 1;
	wi->row_span  = 1;
	wi->flags_x   = GTK_FILL;
	wi->flags_y   = GTK_FILL;
	wi->type      = user_widget;
	wi->name      = g_strdup (name);
	return wi;
}

/*
 * Place a widget on the table according to the values specified in WI
 */
static void
gtk_ted_attach (GtkTed *ted, GtkWidget *widget, struct ted_widget_info *wi)
{
	gtk_table_attach (GTK_TABLE (ted), widget,
			  1+ wi->start_col * 2, 1+ (wi->start_col * 2)+(wi->col_span * 2)-1,
			  1+ wi->start_row * 2, 1+ (wi->start_row * 2)+(wi->row_span * 2)-1,
			  wi->flags_x,
			  wi->flags_y,
			  0, 0);
	
	/*
	 * Hackety hack: the frames and separators should be below any regular widget
	 * Lower their event box, and then lower all the control columns (the buttons).
	 */
	if (GTK_IS_EVENT_BOX (widget)){
		GList *child;
		GtkTableChild *tc;
		
		gdk_window_lower (widget->window);

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
	return 1;
}

/*
 * Handle the dragging part of the widget
 */
static void
moveme (GtkWidget *w, GdkEvent *event, GtkWidget *widget)
{
	GtkWidget *parent;

	parent = w->parent;
	
	gtk_widget_set_uposition (parent, event->motion.x_root, event->motion.y_root);

}

/*
 * Widget dropped
 */
static void
releaseme (GtkWidget *w, GdkEventButton *event, GtkTed *ted)
{
	GtkWidget *parent = w->parent;
	
	gdk_pointer_ungrab (GDK_CURRENT_TIME);
	gdk_flush ();
	if (gtk_ted_widget_drop (ted, w, event->x_root, event->y_root)){
		gtk_widget_destroy (parent);
	}
}

/*
 * start dragging the widget
 */
static void
lift_me (GtkWidget *w, GdkEventButton *event, GtkWidget *target)
{
	GtkWidget *top;
	
	top = gtk_window_new (GTK_WINDOW_POPUP);
	
	gtk_widget_realize (top);
	gtk_widget_reparent (w, top);
	gtk_widget_set_uposition (top, (int) event->x_root, (int) event->y_root);
	gtk_widget_show (top);
	gdk_pointer_grab (w->window, FALSE, GDK_BUTTON1_MOTION_MASK | GDK_BUTTON_RELEASE_MASK,
			  NULL, NULL, GDK_CURRENT_TIME);
}

/*
 * Connect the signals in the widget for drag and drop.
 */
static void
gtk_ted_prepare_editable_widget (GtkWidget *w, GtkWidget *ted_table, char *name)
{
	gtk_signal_connect (GTK_OBJECT (w), "button_press_event", GTK_SIGNAL_FUNC (lift_me), w);
	gtk_signal_connect (GTK_OBJECT (w), "motion_notify_event", GTK_SIGNAL_FUNC (moveme), ted_table);
	gtk_signal_connect (GTK_OBJECT (w), "button_release_event", GTK_SIGNAL_FUNC (releaseme), ted_table);
	gtk_widget_set_events (w, gtk_widget_get_events (w) |
			       GDK_BUTTON_RELEASE_MASK |
			       GDK_BUTTON1_MOTION_MASK |
			       GDK_BUTTON_PRESS_MASK);
	gtk_object_set_data (GTK_OBJECT (w), "ted_widget_id", name);
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

	sprintf (buf, "%d", col);
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

	sprintf (buf, "%d", row);
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
		gtk_ted_separator_col (rw->ted, rw->ted->top_col-2, rw->ted->top_row);
		gtk_ted_add_control_col_at (rw->ted, rw->ted->top_col-1);
		rw->ted->top_col++;
		gtk_ted_resize_rows (rw->ted);
	} else {
		gtk_ted_separator_row (rw->ted, rw->ted->top_row-2, rw->ted->top_col);
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

static char *
gtk_ted_get_string (char *title)
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
	char *name;
	char my_name [40];
	static int frame_count;
	
	name = gtk_ted_get_string ("Type in frame title:");
	if (!name)
		return;
	e = gtk_event_box_new ();
	gtk_widget_show (e);
	f = gtk_frame_new (name);
	gtk_widget_show (f);
	gtk_container_add (GTK_CONTAINER (e), f);

	sprintf (my_name, "Frame-%d", frame_count++);
	wi = gtk_ted_widget_info_new (e, my_name, 0, 0);
	wi->type = frame_widget;
	gtk_object_set_data (GTK_OBJECT (e), "ted_widget_info", wi);
	gtk_ted_add (ted, e, my_name);
}

static void
gtk_ted_add_separator (GtkWidget *widget, GtkTed *ted)
{
	struct ted_widget_info *wi;
	GtkWidget *e, *s;
	char my_name [40];
	static int sep_count;
	
	e = gtk_event_box_new ();
	gtk_widget_show (e);
	s = gtk_hseparator_new ();
	gtk_widget_show (s);
	gtk_container_add (GTK_CONTAINER (e), s);

	sprintf (my_name, "Separator-%d", sep_count++);
	wi = gtk_ted_widget_info_new (e, my_name, 0, 0);
	wi->type = sep_widget;
	gtk_object_set_data (GTK_OBJECT (e), "ted_widget_info", wi);
	
	gtk_container_border_width (GTK_CONTAINER (e), 2);
	gtk_ted_add (ted, e, my_name);
}

static char *
gtk_ted_render_flags (int flags)
{
	static char buf [4];
	char *p = buf;
	
	if (flags & GTK_EXPAND)
		*p++ = 'e';
	if (flags & GTK_FILL)
		*p++ = 'f';
	if (flags & GTK_SHRINK)
		*p++ = 's';
	*p = 0;
	
	return buf;
}

static void
gtk_ted_save (GtkWidget *widget, GtkTed *ted)
{
	GList *child;
	char *filename;
	
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
		}

		key = g_copy_strings ("=", filename, "=/", ted->dialog_name, "-", kind, "-", wi->name, "/", NULL);
		gnome_config_push_prefix (key);
		sprintf (buffer, "%d,%d,%d,%d", wi->start_col, wi->start_row, wi->col_span, wi->row_span);
		gnome_config_set_string ("geometry", buffer);
		gnome_config_set_string ("flags_x", gtk_ted_render_flags (wi->flags_x));
		gnome_config_set_string ("flags_y", gtk_ted_render_flags (wi->flags_y));

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

	/* Save */
	b = gtk_button_new_with_label ("Save layout");
	gtk_widget_show (b);
	gtk_signal_connect (GTK_OBJECT (b), "clicked", GTK_SIGNAL_FUNC (gtk_ted_save), ted);
	gtk_container_add (GTK_CONTAINER (hbox), b);
	
	return hbox;
}

void
gtk_ted_init_editor (GtkTed *ted)
{
	GtkWidget *control_window, *control_bar;
	int rows = 6;
	int cols = 6;
	
	gtk_ted_add_controls (ted, cols, rows);
	gtk_ted_add_divisions (ted, cols, rows);	

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

static int
gtk_ted_parse_flags (char *str)
{
	int flags = 0;
	
	while (*str){
		if (*str == 'e')
			flags |= GTK_EXPAND;
		else if (*str == 'f')
			flags |= GTK_FILL;
		else if (*str == 's')
			flags |= GTK_SHRINK;
		str++;
	}
	return flags;
}

static struct ted_widget_info *
gtk_ted_load_widget (GtkTed *ted, char *prefix, char *secname)
{
	char *full = g_copy_strings (prefix, "/", ted->dialog_name, "-", secname, "/", NULL);
	struct ted_widget_info *wi = g_new (struct ted_widget_info, 1);
	char *str;
	
	wi->widget = 0;
	
	gnome_config_push_prefix (full);
	str = gnome_config_get_string ("geometry");
	sscanf (str, "%d,%d,%d,%d", &wi->start_col, &wi->start_row, &wi->col_span, &wi->row_span);
	g_free (str);
	wi->flags_x = gtk_ted_parse_flags (gnome_config_get_string ("flags_x"));
	wi->flags_y = gtk_ted_parse_flags (gnome_config_get_string ("flags_y"));
	wi->type = user_widget;
	wi->name = g_strdup (secname + 7);
	g_free (full);
	return wi;
}

void
gtk_ted_load_layout (GtkTed *ted)
{
	struct ted_widget_info *wi;
	char *name = g_copy_strings ("layout/", app_name, NULL);
	char *full, *sec_name, *layout;
	void *iter;
	int  len = strlen (ted->dialog_name);
	
	full = gnome_datadir_file (name);
	g_free (name);
	
	if (g_file_exists (full)){
		layout = g_copy_strings ("=", full, "=", NULL);
	} else {
		layout = g_strdup ("=./layout=");
	}
	g_free (full);

	iter = gnome_config_init_iterator_sections (layout);
	while ((iter = gnome_config_iterator_next (iter, &sec_name, NULL)) != NULL){
		if (strncmp (sec_name, ted->dialog_name, len))
			continue;
		
		if (strncmp (sec_name+len+1, "Widget-", 7) == 0){
			wi = gtk_ted_load_widget (ted, layout, sec_name+len+1);
			g_hash_table_insert (ted->widgets, wi->name, wi);
		}
		printf ("section name: %s\n", sec_name);
	}
	g_free (layout);
}

static void
gtk_ted_update_position (struct ted_widget_info *wi)
{
	if (GTK_IS_TABLE (wi->widget->parent)){
		GtkTed *ted = GTK_TED (wi->widget->parent);
		char buf [40];
		
		gtk_container_remove (GTK_CONTAINER (wi->widget->parent), wi->widget);
		gtk_ted_attach (ted, wi->widget, wi);

		sprintf (buf, "%d", wi->col_span);
		gtk_label_set (GTK_LABEL (wi->label_span_x), buf);
		sprintf (buf, "%d", wi->row_span);
		gtk_label_set (GTK_LABEL (wi->label_span_y), buf);
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

static void
gtk_ted_checkbox_toggled (GtkWidget *widget, struct gtk_ted_checkbox_info *ci)
{
	if (ci->is_y)
		ci->wi->flags_y ^= ci->value;
	else
		ci->wi->flags_x ^= ci->value;
	gtk_ted_update_position (ci->wi);
}

static void
gtk_ted_new_checkbox (GtkWidget *box, struct ted_widget_info *wi, char *string, int value, int is_y)
{
	GtkWidget *check;
	struct gtk_ted_checkbox_info *ci;
	int toggle_val;

	ci = g_new (struct gtk_ted_checkbox_info, 1);

	ci->wi = wi;
	ci->is_y = is_y;
	ci->value = value;

	toggle_val = 0;
	if (is_y){
		if (wi->flags_y & value)
			toggle_val = 1;
	} else {
		if (wi->flags_x & value)
			toggle_val = 1;
	}
	
	check = gtk_check_button_new_with_label (string);
	gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (check), toggle_val);
	gtk_box_pack_end_defaults (GTK_BOX (box), check);
	gtk_widget_show (check);
	gtk_signal_connect (GTK_OBJECT (check), "toggled", GTK_SIGNAL_FUNC (gtk_ted_checkbox_toggled), ci);
}

static GtkWidget *
gtk_ted_span_control (char *str, struct ted_widget_info *wi, int is_y)
{
	GtkWidget *more, *less, *hbox, *vbox, *frame, *label;
	char buf [40];

	vbox = gtk_vbox_new (0, 0);
	gtk_widget_show (vbox);

	hbox = gtk_hbox_new (0, 0);
	gtk_widget_show (hbox);
	gtk_container_add (GTK_CONTAINER (vbox), hbox);
	
	frame = gtk_frame_new (str);
	gtk_widget_show (frame);
	gtk_container_add (GTK_CONTAINER (frame), hbox);
	gtk_container_border_width (GTK_CONTAINER (frame), 2);
	gtk_container_border_width (GTK_CONTAINER (hbox), 2);
	more  = gtk_button_new_with_label ("+");
	gtk_widget_show (more);
	gtk_box_pack_end_defaults (GTK_BOX (hbox), more);

	sprintf (buf, "%d", is_y ? wi->row_span : wi->col_span);
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

	gtk_ted_new_checkbox (hbox, wi, "expand", GTK_EXPAND, is_y);
	gtk_ted_new_checkbox (hbox, wi, "fill",   GTK_FILL,   is_y);
	gtk_ted_new_checkbox (hbox, wi, "shrink", GTK_SHRINK, is_y);
	return frame;
}

static GtkWidget *
gtk_ted_size_control_new (struct ted_widget_info *wi)
{
	GtkWidget *vbox;
		
	vbox = gtk_vbox_new (0, 0);
	gtk_widget_show (vbox);
	gtk_box_pack_start (GTK_BOX (vbox), gtk_ted_span_control ("span x", wi, 0), 0, 0, 0);
	gtk_box_pack_start (GTK_BOX (vbox), gtk_ted_span_control ("span y", wi, 1), 0, 0, 0);

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
gtk_ted_add (GtkTed *ted, GtkWidget *widget, char *name)
{
	struct ted_widget_info *wi;
	
	if ((wi = g_hash_table_lookup (ted->widgets, name)) != NULL){
		wi->widget = widget;
		gtk_object_set_data (GTK_OBJECT (widget), "ted_widget_info", wi);
		return;
	} else {
		ted->need_gui = 1;
		wi = gtk_ted_widget_info_new (widget, name, 0, 0);
		g_hash_table_insert (ted->widgets, wi->name, wi);
		gtk_object_set_data (GTK_OBJECT (widget), "ted_widget_info", wi);
	}

	/* Insertions when we are running the GUI should be properly prepared */
	if (ted->in_gui){
		gtk_ted_prepare_editable_widget (wi->widget, GTK_WIDGET (ted), name);
		gtk_ted_widget_control_new (ted, wi->widget, name);
	}
}

static void
gtk_ted_prepare_widgets_edit (gpointer key, gpointer value, gpointer user_data)
{
	GtkTed *ted = user_data;
	struct ted_widget_info *wi = value;

	gtk_ted_prepare_editable_widget (wi->widget, GTK_WIDGET (ted), key);
	gtk_ted_widget_control_new (ted, wi->widget, key);
}

static void
gtk_ted_setup_layout (gpointer key, gpointer value, gpointer user_data)
{
	GtkTed *ted = user_data;
	struct ted_widget_info *wi = value;

	gtk_table_attach (GTK_TABLE (ted), wi->widget,
			  wi->start_col, wi->start_col + wi->col_span,
			  wi->start_row, wi->start_row + wi->row_span,
			  wi->flags_x, wi->flags_y, 0, 0);

	ted->top_col = MAX (ted->top_col, wi->start_col + wi->col_span);
	ted->top_row = MAX (ted->top_row, wi->start_row + wi->row_span);
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
	} else {
		gtk_table_set_row_spacings (GTK_TABLE (ted), 6);
		gtk_table_set_col_spacings (GTK_TABLE (ted), 6);
		g_hash_table_foreach (ted->widgets, gtk_ted_setup_layout, ted);
	}
}

GtkWidget *
gtk_ted_new (char *name)
{
	GtkTed *ted;

	ted = gtk_type_new (gtk_ted_get_type ());
	ted->dialog_name = g_strdup (name);
	gtk_ted_load_layout (ted);

	return GTK_WIDGET (ted);
}

