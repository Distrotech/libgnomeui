/* gtkcauldron.c - creates complex dialogs from a format string in a single line
 * Copyright (C) 1998 Paul Sheer
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "gtkcauldron.h"
#define HAVE_GNOME
#include <gtk/gtk.h>
#ifdef HAVE_GNOME
#include "libgnomeui/gnome-file-entry.h"
#include "libgnomeui/gnome-number-entry.h"
#include "libgnomeui/gnome-stock.h"
#endif
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

gchar *GTK_CAULDRON_ENTER = "GTK_CAULDRON_ENTER";
gchar *GTK_CAULDRON_ESCAPE = "GTK_CAULDRON_ESCAPE";

enum {
    VERTICAL_HOMOGENOUS,
    VERTICAL,
    HORIZONTAL_HOMOGENOUS,
    HORIZONTAL
};

#define CAULDRON_FMT_OPTIONS "xfpseocrdqjhvag"

struct cauldron_result {
    void (*get_result) (GtkWidget *, void *);
    GtkWidget *w;
    void *result;
    struct cauldron_result *next;
    struct cauldron_result *prev;
};

/* user data for button "click" callback points to this struct */
struct cauldron_button {
    gchar *label;
    gchar **result;
    struct cauldron_result *r;	/* results to assign when button is pressed or NULL */
    GtkWidget *window;		/* window to kill when button is pressed or NULL */
};

/* loop through all widgets that must return a value - is called before exit */
static void gtk_cauldron_get_results (struct cauldron_result *r)
{
    while (r->prev)
	r = r->prev;
    while (r) {
	(*r->get_result) (r->w, r->result);
	r = r->next;
    }
}

/* when a button is pressed for to quit the dialog, this function is called */
void gtk_cauldron_button_exit (GtkWidget * w, struct cauldron_button *b)
{
    if (b->r)
	gtk_cauldron_get_results (b->r);
    if (b->window) {
	*(b->result) = b->label;	/* returned from gtk_dialog_cauldron */
	gtk_main_quit ();
    }
}

/* get text of an entry widget */
void get_entry_result (GtkWidget * w, void *x)
{
    gchar **result = (gchar **) x;
    *result = strdup (gtk_entry_get_text (GTK_ENTRY (w)));
}

#ifdef HAVE_GNOME

/* get text of an gnome entry widget */
void get_gnome_entry_result (GtkWidget * w, void *x)
{
    gchar **result = (gchar **) x;
    GnomeEntry *gentry;
    GtkWidget *entry;
    gentry = GNOME_ENTRY (w);
    entry = gnome_entry_gtk_entry (gentry);
    *result = strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
}

/* get text of an gnome file entry widget */
void get_gnome_file_entry_result (GtkWidget * w, void *x)
{
    gchar **result = (gchar **) x;
    GnomeFileEntry *gentry;
    GtkWidget *entry;
    gentry = GNOME_FILE_ENTRY (w);
    entry = gnome_file_entry_gtk_entry (gentry);
    *result = strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
}

/* get text of an gnome number entry widget */
void get_gnome_number_entry_result (GtkWidget * w, void *x)
{
    gchar **result = (gchar **) x;
    GnomeNumberEntry *gentry;
    GtkWidget *entry;
    gentry = GNOME_NUMBER_ENTRY (w);
    entry = gnome_number_entry_gtk_entry (gentry);
    *result = strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
}

#endif

/* get text of an text widget */
void get_text_result (GtkWidget * w, void *x)
{
    gchar **result = (gchar **) x;
    gint i, l;
    l = gtk_text_get_length (GTK_TEXT (w));
    *result = malloc (l + 1);
    for (i = 0; i < l; i++) {
	(*result)[i] = GTK_TEXT_INDEX (GTK_TEXT (w), i);
    }
    (*result)[l] = '\0';
}

/* get state of a check box or a radio button */
void get_check_result (GtkWidget * w, void *x)
{
    gint *result = (gint *) x;
    *result = GTK_TOGGLE_BUTTON (w)->active;
}

/* get state of a check box or a radio button */
void get_spin_result (GtkWidget * w, void *x)
{
    gdouble *result = (gdouble *) x;
    *result = gtk_spin_button_get_value_as_float (GTK_SPIN_BUTTON (w));
}

/* {{{ parsing utilities */

#define LEFT_BRACKETS		"<([{"
#define RIGHT_BRACKETS		">)]}"

/* *p is equal to '(' when this is called */
static gint find_breaker (gchar * p)
{
    int depth = 0;
    gchar *q;
    for (q = p; *q; q++) {
	if (strchr (LEFT_BRACKETS, *q)) {
	    depth++;
	    continue;
	}
	if (strchr (RIGHT_BRACKETS, *q)) {
	    depth--;
	    continue;
	}
	if (depth < 1)
	    return HORIZONTAL;
	if (depth == 1) {
	    if (!strncmp (q, "||", 2))
		return HORIZONTAL_HOMOGENOUS;
	    if (!strncmp (q, "//", 2))
		return VERTICAL_HOMOGENOUS;
	    if (!strncmp (q, "|", 1))
		return HORIZONTAL;
	    if (!strncmp (q, "/", 1))
		return VERTICAL;
	}
    }
    g_error ("bracketing error in call to gtk_dialog_cauldron()");
    return 0;
}

gint space_after (gchar * p)
{
    int s = 0;
    p++;
    while ((*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z'))
	p++;
    while (*p == ' ') {
	p++;
	s++;
    }
    return s;
}

/* }}} parsing utilities */

/* {{{ a simple stack */

static gint widget_stack_pointer = 0;
static GtkWidget *widget_stack[256] =
{0};

static void widget_stack_push (GtkWidget * x)
{
    if (widget_stack_pointer >= 250)
	g_error ("to many widget's in gtk_dialog_cauldron()");
    widget_stack[widget_stack_pointer++] = x;
    widget_stack[widget_stack_pointer] = 0;
}

static GtkWidget *widget_stack_pop (void)
{
    if (widget_stack_pointer <= 0)
	g_error ("bracketing error in call to gtk_dialog_cauldron()");
    return widget_stack[--widget_stack_pointer];
}

static GtkWidget *widget_stack_top (void)
{
    if (widget_stack_pointer <= 0)
	g_error ("bracketing error in call to gtk_dialog_cauldron()");
    widget_stack[widget_stack_pointer] = 0;
    return widget_stack[widget_stack_pointer - 1];
}

/* }}} a simple stack */

/* {{{ process packing options */

static int option_is_present (gchar * p, gchar c)
{
    int present = 0;
    while (*p && strchr (CAULDRON_FMT_OPTIONS, *p)) {
	if (*p == c)
	    present++;
	p++;
    }
    return present;
}

static void gtk_cauldron_box_add (GtkWidget * b, GtkWidget * w, gchar ** p, gint pixels_per_space)
{
    gint expand = FALSE, fill = FALSE, padding = 0, shadow = -1;
    if (option_is_present (1 + *p, 'd')) {	/* 'd' for 'd'efault values */
	gtk_container_add (GTK_CONTAINER (b), w);
	while ((*p)[1] && strchr (CAULDRON_FMT_OPTIONS, (*p)[1]))
	    (*p)++;
	return;
    }
    while ((*p)[1] && strchr (CAULDRON_FMT_OPTIONS, (*p)[1])) {
	(*p)++;
	switch (**p) {
	case 'x':
	    expand = TRUE;
	    break;
	case 'f':
	    fill = TRUE;
	    break;
	case 'p':
	    padding += pixels_per_space;
	    break;
	case 's':
	    (*p)++;
	    switch (**p) {
	    case 'i':
		shadow = GTK_SHADOW_IN;
		break;
	    case 'o':
		shadow = GTK_SHADOW_OUT;
		break;
	    case 'e':
		(*p)++;
		switch (**p) {
		case 'i':
		    shadow = GTK_SHADOW_ETCHED_IN;
		    break;
		case 'o':
		    shadow = GTK_SHADOW_ETCHED_OUT;
		    break;
		}
		break;
	    }
	    if (GTK_IS_FRAME (w) && shadow != -1) {
		gtk_frame_set_shadow_type (GTK_FRAME (w), shadow);
	    } else if (GTK_IS_VIEWPORT (w) && shadow != -1) {
		gtk_viewport_set_shadow_type (GTK_VIEWPORT (w), shadow);
	    } else {
		g_warning ("gtk_dialog_cauldron(): widget doesn't support setting of shadow type");
	    }
	    break;
	default:
	    break;
	}
    }
    if (GTK_IS_BOX (b))
	gtk_box_pack_start (GTK_BOX (b), w, expand, fill, padding);
    else if (GTK_IS_CONTAINER (b))
	gtk_container_add (GTK_CONTAINER (b), w);
}

/* }}} process packing options */

/* {{{ key event handler */

struct key_press_event_data {
    gchar **result;
    glong options;
    struct cauldron_result **r;
};

/* event->keyval */

static gint key_press_event (GtkWidget * window, GdkEventKey * event, struct key_press_event_data *d)
{
    if (!(d->options & GTK_CAULDRON_IGNOREENTER)) {
/* FIXME: is this really necessary? : */
	if (event->keyval == GDK_Return || event->keyval == GDK_KP_Enter || event->keyval == GDK_ISO_Enter || event->keyval == GDK_3270_Enter) {
	    *d->result = GTK_CAULDRON_ENTER;
	    gtk_cauldron_get_results (*d->r);
	    gtk_widget_destroy (window);
	    return TRUE;
	}
    }
    if (!(d->options & GTK_CAULDRON_IGNOREESCAPE)) {
	if (event->keyval == GDK_Escape) {
	    *d->result = GTK_CAULDRON_ESCAPE;
	    gtk_widget_destroy (window);
	    return TRUE;
	}
    }
    return FALSE;
}

/* }}} key event handler */


/* {{{ main */

#define new_result(l,f,w,d)						\
	if (l) {							\
	    (l)->next = malloc (sizeof (*(l)));			\
	    (l)->next->prev = (l);					\
	    (l) = (l)->next;					\
	    (l)->next = 0;					\
	} else {							\
	    (l) = malloc (sizeof (*(l)));				\
	    (l)->next = 0;					\
	    (l)->prev = 0;					\
	}							\
	(l)->get_result = (f);					\
	(l)->w = (w);						\
	(l)->result = (void *) (d);


static void user_callbacks (GtkWidget * w, gchar * p, GtkCauldronNextArgCallback next_arg, gpointer user_data, GtkAccelGroup * accel_table)
{
    int npresent, i;
    npresent = option_is_present (p, 'a');
    for (i = 0; i < npresent; i++) {
	gchar *signal;
	gint key, mods;
	next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &signal);
	next_arg (GTK_CAULDRON_TYPE_INT, user_data, &key);
	next_arg (GTK_CAULDRON_TYPE_INT, user_data, &mods);
	gtk_widget_add_accelerator (w, signal, accel_table, key, mods, GTK_ACCEL_VISIBLE);
    }
    if (option_is_present (p, 'c')) {
	GtkCauldronCustomCallback fud;
	gpointer fud_user_data;
	next_arg (GTK_CAULDRON_TYPE_CALLBACK, user_data, &fud);
	next_arg (GTK_CAULDRON_TYPE_USERDATA_P, user_data, &fud_user_data);
	(*fud) ((w), fud_user_data);
    }
}

#define MAX_TEXT_WIDGETS 64

gchar *gtk_dialog_cauldron_parse (gchar * title, glong options, const gchar * format,
		 GtkCauldronNextArgCallback next_arg, gpointer user_data)
{
    GtkWidget *w = 0, *window;
    GSList *radio_group = 0;
    gchar *p, *return_result = 0, *fmt;
    gint pixels_per_space = 3;
    struct cauldron_button b[256];
    gint ncauldron_button = 0;
    gint how;
    struct cauldron_result *r = 0;
    struct key_press_event_data d;
    GtkAccelGroup *accel_table;

/* text widgets may only add text after the widget is realized,
   so we store text to be added in these, and then add it 
   at the end. */
    GtkWidget *text_widget[MAX_TEXT_WIDGETS];
    gint ntext_widget = 0;
    gchar *text[MAX_TEXT_WIDGETS];

    if (options & GTK_CAULDRON_SPACE_MASK)
	pixels_per_space = (options & GTK_CAULDRON_SPACE_MASK) >> GTK_CAULDRON_SPACE_SHIFT;

    how = GTK_WINDOW_DIALOG;
    if (options & GTK_CAULDRON_TOPLEVEL)
	how = GTK_WINDOW_TOPLEVEL;
    else if (options & GTK_CAULDRON_DIALOG)
	how = GTK_WINDOW_DIALOG;
    else if (options & GTK_CAULDRON_POPUP)
	how = GTK_WINDOW_POPUP;
    window = gtk_window_new (how);
    accel_table = gtk_accel_group_new ();
    gtk_window_add_accel_group (GTK_WINDOW (window), accel_table);

    if (title)
	gtk_window_set_title (GTK_WINDOW (window), title);

    d.result = &return_result;
    d.options = options;
    d.r = &r;

    gtk_signal_connect (GTK_OBJECT (window), "delete_event",
			GTK_SIGNAL_FUNC (gtk_main_quit),
			NULL);

    gtk_signal_connect (GTK_OBJECT (window), "key_press_event",
			(GtkSignalFunc) key_press_event, (gpointer) & d);

    widget_stack_push (window);

/* add outermost brackets to make parsing easier : */
    p = fmt = malloc (strlen (format) + 8);
    strcpy (fmt, "(");
    strcat (fmt, (gchar *) format);
    strcat (fmt, ")xf ");

/* main loop */
    for (; *p; p++) {
	switch (*p) {
	    int c;
	case '/':
	case '|':
	case ' ':
	    break;
	case '{':
	    switch (find_breaker (p)) {
	    case VERTICAL:
	    case VERTICAL_HOMOGENOUS:
		widget_stack_push (gtk_vpaned_new ());
		break;
	    case HORIZONTAL:
	    case HORIZONTAL_HOMOGENOUS:
		widget_stack_push (gtk_hpaned_new ());
		break;
	    }
	    gtk_container_border_width (GTK_CONTAINER (widget_stack_top ()), pixels_per_space * space_after (p));
	    break;
	case '(':
	    if (strchr ("% \t([{<>}])", p[1])) {
		gchar *q;
		gint s;
		q = strchr (p, '%');
		if (!q)
		    q = "%S )";
		s = space_after (q) * pixels_per_space;
		switch (find_breaker (p)) {
		case VERTICAL:
		    widget_stack_push (gtk_vbox_new (FALSE, s));
		    break;
		case VERTICAL_HOMOGENOUS:
		    widget_stack_push (gtk_vbox_new (TRUE, s));
		    break;
		case HORIZONTAL:
		    widget_stack_push (gtk_hbox_new (FALSE, s));
		    break;
		case HORIZONTAL_HOMOGENOUS:
		    widget_stack_push (gtk_hbox_new (TRUE, s));
		    break;
		}
	    } else {
/* place a label right in here */
		gchar *label;
		label = strdup (p + 1);
		p = strchr (p, ')');
		if (!p) {
		    g_error ("bracketing error in call to gtk_dialog_cauldron()");
		    return 0;
		}
		*(strchr (label, ')')) = '\0';
		w = gtk_label_new (label);
		free (label);
		user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		gtk_cauldron_box_add (widget_stack_top (), w, &p, pixels_per_space);
		gtk_widget_show (w);
		break;
	    }
	    gtk_container_border_width (GTK_CONTAINER (widget_stack_top ()), pixels_per_space * space_after (p));
	    break;
	case '[':
	    widget_stack_push (gtk_frame_new (NULL));
	    break;
	case '%':
	    switch (c = *(++p)) {
	    case '[':{
		    char *label;
		    next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &label);
		    widget_stack_push (gtk_frame_new (label));
		    break;
		}
	    case 'L':{
		    gchar *label;
		    next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &label);
		    w = gtk_label_new (label);
		    user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		    gtk_cauldron_box_add (widget_stack_top (), w, &p, pixels_per_space);
		    gtk_widget_show (w);
		    break;
		}
	    case 'T':{
		    gchar **result;
		    GtkWidget *hscrollbar = 0, *vscrollbar = 0, *table = 0;
		    int cols = 1, rows = 1;
		    w = gtk_text_new (NULL, NULL);
		    next_arg (GTK_CAULDRON_TYPE_CHAR_P_P, user_data, &result);
		    if (option_is_present (p + 1, 'e')) {
			new_result (r, get_text_result, w, result);
			gtk_text_set_editable (GTK_TEXT (w), TRUE);
		    } else {
			gtk_text_set_editable (GTK_TEXT (w), FALSE);
		    }
		    if (option_is_present (p + 1, 'h')) {
			rows++;
			hscrollbar = gtk_hscrollbar_new (GTK_TEXT (w)->hadj);
		    }
		    if (option_is_present (p + 1, 'v')) {
			cols++;
			vscrollbar = gtk_vscrollbar_new (GTK_TEXT (w)->vadj);
		    }
		    if (hscrollbar || vscrollbar) {
			table = gtk_table_new (cols, rows, FALSE);
			gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
			gtk_table_set_col_spacing (GTK_TABLE (table), 0, 2);
		    }
		    if (hscrollbar) {
			gtk_table_attach (GTK_TABLE (table), hscrollbar, 0, 1, 1, 2,
					  GTK_EXPAND | GTK_FILL | GTK_SHRINK, GTK_FILL, 0, 0);
			gtk_widget_show (hscrollbar);
		    }
		    if (vscrollbar) {
			gtk_table_attach (GTK_TABLE (table), vscrollbar, 1, 2, 0, 1,
					  GTK_FILL, GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
			gtk_widget_show (vscrollbar);
		    }
		    user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		    if (table) {
			gtk_cauldron_box_add (widget_stack_top (), table, &p, pixels_per_space);
			gtk_table_attach (GTK_TABLE (table), w, 0, 1, 0, 1,
				      GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			       GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
			gtk_widget_show (table);
		    } else {
			gtk_cauldron_box_add (widget_stack_top (), w, &p, pixels_per_space);
		    }
		    gtk_widget_show (w);
		    if (*result) {
			text_widget[ntext_widget] = w;
			text[ntext_widget] = *result;
			ntext_widget++;
			if (ntext_widget >= MAX_TEXT_WIDGETS)
			    g_error ("MAX_TEXT_WIDGETS exceeded in call to gtk_dialog_cauldron()");
		    }
		    break;
		}
	    case 'F':
	    case 'N':
	    case 'E':
	    case 'P':{
		    gchar **result, *history_id, *dialog_title;
		    next_arg (GTK_CAULDRON_TYPE_CHAR_P_P, user_data, &result);
#ifdef HAVE_GNOME
		    if (option_is_present (p + 1, 'g') && *p != 'P') {
			next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &history_id);
			switch (*p) {
			case 'F':
			    next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &dialog_title);
			    w = gnome_file_entry_new (history_id, dialog_title);
			    if (*result) {
				GnomeFileEntry *gentry;
				GtkWidget *entry;
				gentry = GNOME_FILE_ENTRY (w);
				entry = gnome_file_entry_gtk_entry (gentry);
				gtk_entry_set_text (GTK_ENTRY (entry), *result);
				new_result (r, get_gnome_file_entry_result, w, result);
			    }
			    break;
			case 'N':
			    next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &dialog_title);
			    w = gnome_number_entry_new (history_id, dialog_title);
			    if (*result) {
				GnomeNumberEntry *gentry;
				GtkWidget *entry;
				gentry = GNOME_NUMBER_ENTRY (w);
				entry = gnome_number_entry_gtk_entry (gentry);
				gtk_entry_set_text (GTK_ENTRY (entry), *result);
				new_result (r, get_gnome_number_entry_result, w, result);
			    }
			    break;
			case 'E':
			    w = gnome_entry_new (history_id);
			    if (*result) {
				GnomeEntry *gentry;
				GtkWidget *entry;
				gentry = GNOME_ENTRY (w);
				entry = gnome_entry_gtk_entry (gentry);
				gtk_entry_set_text (GTK_ENTRY (entry), *result);
				new_result (r, get_gnome_entry_result, w, result);
			    }
			    break;
			}
		    } else {
			w = gtk_entry_new ();
			new_result (r, get_entry_result, w, result);
			if (c == 'P')
			    gtk_entry_set_visibility (GTK_ENTRY (w), FALSE);
			if (*result)
			    gtk_entry_set_text (GTK_ENTRY (w), *result);
		    }
#else
		    w = gtk_entry_new ();
		    new_result (r, get_entry_result, w, result);
		    if (c == 'P')
			gtk_entry_set_visibility (GTK_ENTRY (w), FALSE);
		    if (*result)
			gtk_entry_set_text (GTK_ENTRY (w), *result);
#endif
		    user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		    gtk_cauldron_box_add (widget_stack_top (), w, &p, pixels_per_space);
		    gtk_widget_show (w);
		    break;
		}
	    case 'B':{
		    gchar *label;
		    next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &label);
		    if (label) {
#ifdef HAVE_GNOME
			if (option_is_present (p + 1, 'g'))
			    w = gnome_stock_button (label);
			else
#endif
			    w = gtk_button_new_with_label (label);
		    } else
			w = gtk_button_new ();
		    b[ncauldron_button].label = label;
		    b[ncauldron_button].result = &return_result;
		    if (option_is_present (p + 1, 'r'))
			b[ncauldron_button].r = r;
		    else
			b[ncauldron_button].r = 0;
		    if (option_is_present (p + 1, 'q'))
			b[ncauldron_button].window = window;
		    else
			b[ncauldron_button].window = 0;
		    gtk_signal_connect (GTK_OBJECT (w), "clicked",
			      GTK_SIGNAL_FUNC (gtk_cauldron_button_exit),
					(gpointer) & b[ncauldron_button]);
		    user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		    gtk_cauldron_box_add (widget_stack_top (), w, &p, pixels_per_space);
		    gtk_widget_show (w);
		    ncauldron_button++;
		    break;
		}
	    case 'C':{
		    gchar *label;
		    int *result;
		    next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &label);
		    if (label)
			w = gtk_check_button_new_with_label (label);
		    else
			w = gtk_check_button_new ();
		    next_arg (GTK_CAULDRON_TYPE_INT_P, user_data, &result);
		    GTK_TOGGLE_BUTTON (w)->active = *result;
		    new_result (r, get_check_result, w, result);
		    user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		    gtk_cauldron_box_add (widget_stack_top (), w, &p, pixels_per_space);
		    gtk_widget_show (w);
		    break;
		}
	    case 'S':
		if (p[1] == 'B') {
		    gint digits;
		    gdouble climb_rate, *result;
		    p++;
		    next_arg (GTK_CAULDRON_TYPE_DOUBLE, user_data, &climb_rate);
		    next_arg (GTK_CAULDRON_TYPE_INT, user_data, &digits);
		    if (option_is_present (p + 1, 'j')) {
			gdouble lower_bound, upper_bound, step_increment,
			 page_increment, page_size;
			GtkAdjustment *adj;
			next_arg (GTK_CAULDRON_TYPE_DOUBLE_P, user_data, &result);
			next_arg (GTK_CAULDRON_TYPE_DOUBLE, user_data, &lower_bound);
			next_arg (GTK_CAULDRON_TYPE_DOUBLE, user_data, &upper_bound);
			next_arg (GTK_CAULDRON_TYPE_DOUBLE, user_data, &step_increment);
			next_arg (GTK_CAULDRON_TYPE_DOUBLE, user_data, &page_increment);
			next_arg (GTK_CAULDRON_TYPE_DOUBLE, user_data, &page_size);
			adj = (GtkAdjustment *) gtk_adjustment_new (*result, lower_bound, \
								    upper_bound, step_increment, page_increment, page_size);
			w = gtk_spin_button_new (adj, climb_rate, digits);
			new_result (r, get_spin_result, w, result);
		    } else {
			w = gtk_spin_button_new (0, climb_rate, digits);
		    }
		    user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		    gtk_cauldron_box_add (widget_stack_top (), w, &p, pixels_per_space);
		    gtk_widget_show (w);
		} else {
		    if (GTK_IS_HBOX (widget_stack_top ())) {
			w = gtk_vseparator_new ();
		    } else {
			w = gtk_hseparator_new ();
		    }
		    user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		    gtk_cauldron_box_add (widget_stack_top (), w, &p, pixels_per_space);
		    gtk_widget_show (w);
		}
		break;
	    case 'R':{
		    gchar *label;
		    int *result;
		    next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &label);
		    if (label)
			w = gtk_radio_button_new_with_label (radio_group, label);
		    else
			w = gtk_radio_button_new (radio_group);
		    radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (w));
		    next_arg (GTK_CAULDRON_TYPE_INT_P, user_data, &result);
		    gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (w), *result);
		    new_result (r, get_check_result, w, result);
		    user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		    gtk_cauldron_box_add (widget_stack_top (), w, &p, pixels_per_space);
		    gtk_widget_show (w);
		    break;
		}
	    case 'X':{
		    GtkCauldronCustomCallback func;
		    gpointer data;
		    next_arg (GTK_CAULDRON_TYPE_CALLBACK, user_data, &func);
		    next_arg (GTK_CAULDRON_TYPE_USERDATA_P, user_data, &data);
		    w = (*func) (window, data);		/* `window' is just to fill the first arg and shouldn't be needed by the user */
		    if (w) {
			gtk_cauldron_box_add (widget_stack_top (), w, &p, pixels_per_space);
			gtk_widget_show (w);
		    } else {
			g_warning ("user callback GtkCauldronCustomCallback returned null");
		    }
		    break;
		}
	    }
	    break;
	case '}':
	case ')':
	case ']':
	    radio_group = 0;
	    w = widget_stack_pop ();
	    if (option_is_present (p + 1, 'v') || option_is_present (p + 1, 'h')) {
		GtkWidget *scrolled_window;
		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
						option_is_present (p + 1, 'h') ? GTK_POLICY_ALWAYS : GTK_POLICY_AUTOMATIC,
						option_is_present (p + 1, 'v') ? GTK_POLICY_ALWAYS : GTK_POLICY_AUTOMATIC);
		gtk_container_add (GTK_CONTAINER (scrolled_window), w);
		gtk_widget_show (scrolled_window);
		user_callbacks (scrolled_window, p + 1, next_arg, user_data, accel_table);
		gtk_cauldron_box_add (widget_stack_top (), scrolled_window, &p, pixels_per_space);
	    } else {
		user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		gtk_cauldron_box_add (widget_stack_top (), w, &p, pixels_per_space);
	    }
	    gtk_widget_show (w);
	    break;
	default:{
		gchar e[100] = "bad syntax in call to gtk_dialog_cauldron() at: ";
		gint l;
		l = strlen (e);
		strncpy (e + l, p, 30);
		strcpy (e + l + 30, "...");
		g_error ("%s", e);
		return 0;
	    }
	}
    }
    if (widget_stack_pointer != 1) {
	g_error ("bracketing error in call to gtk_dialog_cauldron()");
	return 0;
    }
    w = widget_stack_pop ();
    gtk_widget_show (w);

    while (ntext_widget-- > 0) {
	gtk_text_freeze (GTK_TEXT (text_widget[ntext_widget]));
	gtk_text_insert (GTK_TEXT (text_widget[ntext_widget]), NULL, NULL, NULL, text[ntext_widget], -1);
	gtk_text_thaw (GTK_TEXT (text_widget[ntext_widget]));
    }

    gtk_main ();

    gtk_window_remove_accel_group (GTK_WINDOW (window), accel_table);
    gtk_object_destroy (GTK_OBJECT (window));
    
    if (r) {
	while (r->prev)
	    r = r->prev;
	while (r) {
	    struct cauldron_result *next;
	    next = r->next;
	    free (r);
	    r = next;
	}
    }
    free (fmt);
    return return_result;
}

static void next_arg (gint type, va_list * ap, void *result)
{
    switch (type) {
    case GTK_CAULDRON_TYPE_CHAR_P:{
	    gchar **x = (gchar **) result;
	    *x = va_arg (*ap, gchar *);
	    break;
	}
    case GTK_CAULDRON_TYPE_CHAR_P_P:{
	    gchar ***x = (gchar ***) result;
	    *x = va_arg (*ap, gchar **);
	    break;
	}
    case GTK_CAULDRON_TYPE_INT:{
	    gint *x = (gint *) result;
	    *x = va_arg (*ap, gint);
	    break;
	}
    case GTK_CAULDRON_TYPE_INT_P:{
	    gint **x = (gint **) result;
	    *x = va_arg (*ap, gint *);
	    break;
	}
    case GTK_CAULDRON_TYPE_USERDATA_P:{
	    gpointer *x = (gpointer *) result;
	    *x = va_arg (*ap, gpointer);
	    break;
	}
    case GTK_CAULDRON_TYPE_DOUBLE:{
	    gdouble *x = (gdouble *) result;
	    *x = va_arg (*ap, gdouble);
	    break;
	}
    case GTK_CAULDRON_TYPE_DOUBLE_P:{
	    gdouble **x = (gdouble **) result;
	    *x = va_arg (*ap, gdouble *);
	    break;
	}
    case GTK_CAULDRON_TYPE_CALLBACK:{
	    GtkCauldronNextArgCallback *x = (GtkCauldronNextArgCallback *) result;
	    *x = va_arg (*ap, GtkCauldronNextArgCallback);
	    break;
	}
    }
    return;
}

gchar *gtk_dialog_cauldron (gchar * title, glong options, const gchar * format,...)
{
    gchar *r;
    va_list ap;
    va_start (ap, format);
    r = gtk_dialog_cauldron_parse (title, options, format, (GtkCauldronNextArgCallback) next_arg, &ap);
    va_end (ap);
    return r;
}				/* }}} main */

