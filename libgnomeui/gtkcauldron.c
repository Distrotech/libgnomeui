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
#include <time.h>
#include "libgnomeui/gnome-preferences.h"
#include "libgnomeui/gnome-file-entry.h"
#include "libgnomeui/gnome-dateedit.h"
#include "libgnomeui/gnome-number-entry.h"
#include "libgnomeui/gnome-stock.h"
#endif
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>


gchar *GTK_CAULDRON_ENTER = "GTK_CAULDRON_ENTER";
gchar *GTK_CAULDRON_ESCAPE = "GTK_CAULDRON_ESCAPE";

static gchar *gtk_dialog_cauldron_error = 0;

gchar *gtk_dialog_cauldron_get_error (void)
{
    return gtk_dialog_cauldron_error;
}

static void gtk_dialog_cauldron_set_error (gchar *s)
{
    gtk_dialog_cauldron_error = s;
}

enum {
    VERTICAL_HOMOGENOUS = 1,
    VERTICAL,
    HORIZONTAL_HOMOGENOUS,
    HORIZONTAL
};

#define CAULDRON_FMT_OPTIONS "xfpsieocrdqjhvaugnt"

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
    if (!r)
	return;
    while (r->prev)
	r = r->prev;
    while (r) {
	(*r->get_result) (r->w, r->result);
	r = r->next;
    }
}

/* when a button is pressed for to quit the dialog, this function is called */
static void gtk_cauldron_button_exit (GtkWidget * w, struct cauldron_button *b)
{
    if (b->r)
	gtk_cauldron_get_results (b->r);
    if (b->window) {
	*(b->result) = b->label;	/* returned from gtk_dialog_cauldron */
	gtk_main_quit ();
    }
}

/* get text of an entry widget */
static void get_entry_result (GtkWidget * w, void *x)
{
    gchar **result = (gchar **) x;
    *result = g_strdup (gtk_entry_get_text (GTK_ENTRY (w)));
}

#ifdef HAVE_GNOME

static void get_gnome_date_edit_result (GtkWidget * w, void *x)
{
    gdouble *result = (gdouble *) x;
    *result = (gdouble) gnome_date_edit_get_date (GNOME_DATE_EDIT (w));
}

/* get text of an gnome entry widget */
static void get_gnome_entry_result (GtkWidget * w, void *x)
{
    gchar **result = (gchar **) x;
    GnomeEntry *gentry;
    GtkWidget *entry;
    gentry = GNOME_ENTRY (w);
    entry = gnome_entry_gtk_entry (gentry);
    *result = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
}

/* get text of an gnome file entry widget */
static void get_gnome_file_entry_result (GtkWidget * w, void *x)
{
    gchar **result = (gchar **) x;
    GnomeFileEntry *gentry;
    GtkWidget *entry;
    gentry = GNOME_FILE_ENTRY (w);
    entry = gnome_file_entry_gtk_entry (gentry);
    *result = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
}

/* get text of an gnome number entry widget */
static void get_gnome_number_entry_result (GtkWidget * w, void *x)
{
    gchar **result = (gchar **) x;
    GnomeNumberEntry *gentry;
    GtkWidget *entry;
    gentry = GNOME_NUMBER_ENTRY (w);
    entry = gnome_number_entry_gtk_entry (gentry);
    *result = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
}

#endif

/* get text of an text widget */
static void get_text_result (GtkWidget * w, void *x)
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
static void get_check_result (GtkWidget * w, void *x)
{
    gint *result = (gint *) x;
    *result = GTK_TOGGLE_BUTTON (w)->active;
}

/* get state of a check box or a radio button */
static void get_spin_result (GtkWidget * w, void *x)
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
    gtk_dialog_cauldron_set_error ("gtk_dialog_cauldron(): bracketing error in format string");
    return 0;
}

static gint space_after (gchar * p)
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

typedef struct _GtkCauldronStack {
    gint widget_stack_pointer;
    GtkWidget *widget_stack[256];
} GtkCauldronStack;

static int widget_stack_push (GtkCauldronStack * s, GtkWidget * x)
{
    if (s->widget_stack_pointer >= 250) {
	gtk_dialog_cauldron_set_error ("gtk_dialog_cauldron(): maximum number of widgets exceeded");
	return 1;
    }
    s->widget_stack[s->widget_stack_pointer++] = x;
    s->widget_stack[s->widget_stack_pointer] = 0;
    return 0;
}

static GtkWidget *widget_stack_pop (GtkCauldronStack * s)
{
    if (s->widget_stack_pointer <= 0) {
	gtk_dialog_cauldron_set_error ("gtk_dialog_cauldron(): bracketing error in format string");
	return 0;
    }
    return s->widget_stack[--(s->widget_stack_pointer)];
}

static GtkWidget *widget_stack_top (GtkCauldronStack * s)
{
    if (s->widget_stack_pointer <= 0) {
	gtk_dialog_cauldron_set_error ("gtk_dialog_cauldron(): bracketing error in format string");
	return 0;
    }
    s->widget_stack[s->widget_stack_pointer] = 0;
    return s->widget_stack[s->widget_stack_pointer - 1];
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

static int gtk_cauldron_box_add (GtkWidget * b, GtkWidget * w, gchar ** p, gint pixels_per_space)
{
    gint expand = FALSE, fill = FALSE, padding = 0, shadow = -1;
    if (option_is_present (1 + *p, 't'))
	gtk_widget_set_sensitive (w, 0);
    if (option_is_present (1 + *p, 'd')) {	/* 'd' for 'd'efault values */
	gtk_container_add (GTK_CONTAINER (b), w);
	while ((*p)[1] && strchr (CAULDRON_FMT_OPTIONS, (*p)[1]))
	    (*p)++;
	return 0;
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
		gtk_dialog_cauldron_set_error ("gtk_dialog_cauldron(): widget doesn't support setting of shadow type");
		return 1;
	    }
	    break;
	default:
	    break;
	}
    }
    if (GTK_IS_BOX (b)) {
/* special hack for notebooks */
	if (GTK_IS_NOTEBOOK (w))
	    expand = fill = TRUE;
	gtk_box_pack_start (GTK_BOX (b), w, expand, fill, padding);
    } else if (GTK_IS_CONTAINER (b))
	gtk_container_add (GTK_CONTAINER (b), w);
    return 0;
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
	    gtk_main_quit ();
	    return TRUE;
	}
    }
    if (!(d->options & GTK_CAULDRON_IGNOREESCAPE)) {
	if (event->keyval == GDK_Escape) {
	    *d->result = GTK_CAULDRON_ESCAPE;
	    gtk_main_quit ();
	    return TRUE;
	}
    }
    return FALSE;
}

/* }}} key event handler */


/* {{{ main */

#define new_result(l,f,w,d)					\
	if (l) {						\
	    (l)->next = malloc (sizeof (*(l)));			\
	    (l)->next->prev = (l);				\
	    (l) = (l)->next;					\
	    (l)->next = 0;					\
	} else {						\
	    (l) = malloc (sizeof (*(l)));			\
	    (l)->next = 0;					\
	    (l)->prev = 0;					\
	}							\
	(l)->get_result = (f);					\
	(l)->w = (w);						\
	(l)->result = (void *) (d);

static gchar *convert_label_with_ampersand (const gchar * label, gint * accelerator_key, gint * underbar_pos);

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
    npresent = option_is_present (p, 'u');
    for (i = 0; i < npresent; i++) {
	gchar *signal, *label;
	gint mods;
	gint underbar_pos = -1, accelerator_key = 0;
	next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &signal);
	next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &label);
	next_arg (GTK_CAULDRON_TYPE_INT, user_data, &mods);
	label = convert_label_with_ampersand (label, &accelerator_key, &underbar_pos);
	gtk_widget_add_accelerator (w, signal, accel_table, accelerator_key, mods, GTK_ACCEL_VISIBLE);
	g_free (label);
    }
    if (option_is_present (p, 'c')) {
	GtkCauldronCustomCallback fud;
	gpointer fud_user_data;
	next_arg (GTK_CAULDRON_TYPE_CALLBACK, user_data, &fud);
	next_arg (GTK_CAULDRON_TYPE_USERDATA_P, user_data, &fud_user_data);
	(*fud) ((w), fud_user_data);
    }
}

/* result must be g_free'd */
static gchar *convert_label_with_ampersand (const gchar * _label, gint * accelerator_key, gint * underbar_pos)
{
    gchar *p;
    gchar *label = g_strdup (_label);
    for (p = label;; p++) {
	if (!*p)
	    break;
	if (!p[1])
	    break;
	if (*p == '&') {
	    memcpy (p, p + 1, strlen (p));
	    if (*p == '&')	/* use && for an actual & */
		continue;
	    *underbar_pos = (unsigned long) p - (unsigned long) label;
	    *accelerator_key = *p;
	    return label;
	}
    }
    return label;
}

struct find_label_data {
    gchar *label, *pattern;
};

/* recurse into widget to find a label. when found, set the
   pattern to one with an underbar underneath the hotkey. */
static void find_label (GtkWidget * widget, gpointer data)
{
    struct find_label_data *d;
    d = (struct find_label_data *) data;
    if (GTK_IS_LABEL (widget)) {
	GtkLabel *label;
	label = GTK_LABEL (widget);
	if (!strcmp (label->label, d->label))
	    if (strchr (d->pattern, '_'))
		gtk_label_set_pattern (label, d->pattern);
    } else if (GTK_IS_CONTAINER (widget)) {
	gtk_container_forall (GTK_CONTAINER (widget), find_label, data);
    }
}

static void get_child_entry (GtkWidget * widget, gpointer data)
{
    GtkWidget **d = (GtkWidget **) data;
    if (GTK_IS_ENTRY (widget)) {
	*d = widget;
    } else if (GTK_IS_CONTAINER (widget)) {
	gtk_container_forall (GTK_CONTAINER (widget), get_child_entry, data);
    }
}

static gchar *create_label_pattern (gchar * label, gint underbar_pos)
{
    gchar *pattern;
    pattern = g_strdup (label);
    memset (pattern, ' ', strlen (label));
    if (underbar_pos < strlen (pattern) && underbar_pos >= 0)
	pattern[underbar_pos] = '_';
    return pattern;
}

static void add_accelerator_with_underbar (GtkWidget * widget, GtkAccelGroup * accel_table, gint accelerator_key, gchar * label, gint underbar_pos)
{
    struct find_label_data d;
    d.label = label;
    d.pattern = create_label_pattern (label, underbar_pos);
    find_label (widget, (void *) &d);
    gtk_widget_add_accelerator (widget, "clicked",
	 accel_table, accelerator_key, GDK_MOD1_MASK, GTK_ACCEL_VISIBLE);
    g_free (d.pattern);
}

static GtkWidget *gtk_label_new_with_ampersand (const gchar * _label)
{
    GtkWidget *widget;
    gchar *pattern;
    gint accelerator_key = 0, underbar_pos = -1;
    gchar *label = convert_label_with_ampersand (_label, &accelerator_key,
    						 &underbar_pos);
    pattern = create_label_pattern (label, underbar_pos);
    widget = gtk_label_new (label);
    if (underbar_pos != -1)
	gtk_label_set_pattern (GTK_LABEL (widget), pattern);
    g_free (label);
    return widget;
}

#define MAX_TEXT_WIDGETS 64

/**
 * gtk_dialog_cauldron_parse:
 * @title: dialog title
 * @options: dialog options, see the macro definitions
 * @format: format string that describes the dialog
 * @next_arg: user function.
 * @user_data: data to pass to user function
 *
 * This function parses a format
 * string exactly like gtk_dialog_cauldron(), however it derives
 * arguments for the @format string from a user function @next_arg.
 * gtk_dialog_cauldron_parse() is primarily used for creating
 * wrappers for interpreted languages.
 *
 * Each subsequent call to @next_arg must assign to <type>*result</type> a
 * pointer to data of the type specified by <type>cauldron_type</type>. (An
 * example can be found in <filename>gtk_dialog_cauldron.c</filename> and the
 * pygnome package.) The \fIcauldron_type\fP's are a small set of
 * types used for specifying and returning widget data. They are
 * enumerated as <type>GTK_CAULDRON_TYPE_*</type> in the header file gtkcauldron.h.
 *
 * NULL is returned if the dialog is
 * cancelled. GTK_CAULDRON_ENTER is returned if the user pressed
 * enter (return-on-enter can be overridden - see global options
 * below), and GTK_CAULDRON_ESCAPE is returned if the user
 * pressed escape. GTK_CAULDRON_ERROR is returned by 
 * gtk_dialog_cauldron_parse() if an error
 * occurred (like a malformed format string). The error message can be
 * retrieved by gtk_dialog_cauldron_get_error().
 * Otherwise the label of the widget that was used to
 * exit the dialog is returned.
 *
 */
gchar *gtk_dialog_cauldron_parse (const gchar * title, glong options, const gchar * format,
		 GtkCauldronNextArgCallback next_arg, gpointer user_data, GtkWidget *parent_)
{
    GtkWidget *parent = 0;
    GtkWidget *w = 0, *top, *window, *f = 0;
    GSList *radio_group = 0;
    gchar *p, *return_result = 0, *fmt;
    gint pixels_per_space = 3;
    struct cauldron_button b[256];
    gint ncauldron_button = 0;
    gint how;
    struct cauldron_result *r = 0;
    struct key_press_event_data d;
    GtkAccelGroup *accel_table;
    GtkCauldronStack *stack;

/* text widgets may only add text after the widget is realized,
   so we store text to be added in these, and then add it 
   at the end. */
    GtkWidget *text_widget[MAX_TEXT_WIDGETS];
    gint ntext_widget = 0;
    gchar *text[MAX_TEXT_WIDGETS];

/* this ensure that if we link with an old code, we don't get segfaults */
    if (options & GTK_CAULDRON_PARENT)
	parent = parent_;

    stack = malloc (sizeof (*stack));
    memset (stack, 0, sizeof (*stack));

    if (options & GTK_CAULDRON_SPACE_MASK)
	pixels_per_space = (options & GTK_CAULDRON_SPACE_MASK) >> GTK_CAULDRON_SPACE_SHIFT;

#ifdef HAVE_GNOME
    how = gnome_preferences_get_dialog_type ();
#else
    how = GTK_WINDOW_DIALOG;
#endif
    if (options & GTK_CAULDRON_TOPLEVEL)
	how = GTK_WINDOW_TOPLEVEL;
    else if (options & GTK_CAULDRON_DIALOG)
	how = GTK_WINDOW_DIALOG;
    else if (options & GTK_CAULDRON_POPUP)
	how = GTK_WINDOW_POPUP;
    window = gtk_window_new (how);
#ifdef HAVE_GNOME
    gtk_window_set_position (GTK_WINDOW (window), gnome_preferences_get_dialog_position ());
#else
    gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_MOUSE);
#endif
    if (title)
	gtk_window_set_title (GTK_WINDOW (window), title);
    accel_table = gtk_accel_group_new ();
    gtk_window_add_accel_group (GTK_WINDOW (window), accel_table);

    d.result = &return_result;
    d.options = options;
    d.r = &r;

    gtk_signal_connect (GTK_OBJECT (window), "delete_event",
			GTK_SIGNAL_FUNC (gtk_main_quit),
			NULL);

    gtk_signal_connect (GTK_OBJECT (window), "key_press_event",
			(GtkSignalFunc) key_press_event, (gpointer) & d);

    if (widget_stack_push (stack, window))
	return GTK_CAULDRON_ERROR;

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
		if (widget_stack_push (stack, gtk_vpaned_new ()))
		    return GTK_CAULDRON_ERROR;
		break;
	    case HORIZONTAL:
	    case HORIZONTAL_HOMOGENOUS:
		if (widget_stack_push (stack, gtk_hpaned_new ()))
		    return GTK_CAULDRON_ERROR;
		break;
	    default:
		return GTK_CAULDRON_ERROR;
	    }
	    if (!(top = widget_stack_top (stack)))
		return GTK_CAULDRON_ERROR;
	    gtk_container_set_border_width (GTK_CONTAINER (top), pixels_per_space * space_after (p));
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
		    if (widget_stack_push (stack, gtk_vbox_new (FALSE, s)))
			return GTK_CAULDRON_ERROR;
		    break;
		case VERTICAL_HOMOGENOUS:
		    if (widget_stack_push (stack, gtk_vbox_new (TRUE, s)))
			return GTK_CAULDRON_ERROR;
		    break;
		case HORIZONTAL:
		    if (widget_stack_push (stack, gtk_hbox_new (FALSE, s)))
			return GTK_CAULDRON_ERROR;
		    break;
		case HORIZONTAL_HOMOGENOUS:
		    if (widget_stack_push (stack, gtk_hbox_new (TRUE, s)))
			return GTK_CAULDRON_ERROR;
		    break;
		default:
		    return GTK_CAULDRON_ERROR;
		}
	    } else {
/* place a label right in here */
		gchar *label;
		label = g_strdup (p + 1);
		p = strchr (p, ')');
		if (!p) {
		    gtk_dialog_cauldron_set_error ("gtk_dialog_cauldron(): bracketing error in format string");
		    return GTK_CAULDRON_ERROR;
		}
		*(strchr (label, ')')) = '\0';
		w = gtk_label_new_with_ampersand (label);
		g_free (label);
		user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		if (!(top = widget_stack_top (stack)))
		    return GTK_CAULDRON_ERROR;
		if (gtk_cauldron_box_add (top, w, &p, pixels_per_space))
		    return GTK_CAULDRON_ERROR;
		gtk_widget_show (w);
		break;
	    }
	    if (!(top = widget_stack_top (stack)))
		return GTK_CAULDRON_ERROR;
	    gtk_container_set_border_width (GTK_CONTAINER (top), pixels_per_space * space_after (p));
	    break;
	case '[':
	    if (widget_stack_push (stack, gtk_frame_new (NULL)))
		return GTK_CAULDRON_ERROR;
	    break;
	case '%':
	    switch (c = *(++p)) {
	    case '[':{
		    char *label;
		    next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &label);
		    if (widget_stack_push (stack, gtk_frame_new (label)))
			return GTK_CAULDRON_ERROR;
		    break;
		}
	    case 'L':{
		    gchar *label;
		    next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &label);
		    w = gtk_label_new_with_ampersand (label);
		    user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		    if (!(top = widget_stack_top (stack)))
			return GTK_CAULDRON_ERROR;
		    if (gtk_cauldron_box_add (top, w, &p, pixels_per_space))
			return GTK_CAULDRON_ERROR;
		    gtk_widget_show (w);
		    break;
		}
	    case 'T':{
		    gchar **result;
		    GtkWidget *hscrollbar = 0, *vscrollbar = 0, *table = 0;
		    int cols = 1, rows = 1;
		    w = gtk_text_new (NULL, NULL);
		    if (option_is_present (p + 1, 'o'))
			f = w;
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
			table = gtk_table_new (rows, cols, FALSE);
			if (rows >= 2)
			    gtk_table_set_row_spacing (GTK_TABLE (table), 0, 2);
			if (cols >= 2)
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
			if (!(top = widget_stack_top (stack)))
			    return GTK_CAULDRON_ERROR;
			if (gtk_cauldron_box_add (top, table, &p, pixels_per_space))
			    return GTK_CAULDRON_ERROR;
			gtk_table_attach (GTK_TABLE (table), w, 0, 1, 0, 1,
				      GTK_EXPAND | GTK_SHRINK | GTK_FILL,
			       GTK_EXPAND | GTK_SHRINK | GTK_FILL, 0, 0);
			gtk_widget_show (table);
		    } else {
			if (!(top = widget_stack_top (stack)))
			    return GTK_CAULDRON_ERROR;
			if (gtk_cauldron_box_add (top, w, &p, pixels_per_space))
			    return GTK_CAULDRON_ERROR;
		    }
		    gtk_widget_show (w);
		    if (*result) {
			text_widget[ntext_widget] = w;
			text[ntext_widget] = *result;
			ntext_widget++;
			if (ntext_widget >= MAX_TEXT_WIDGETS) {
			    gtk_dialog_cauldron_set_error ("gtk_dialog_cauldron(): maximum number of text widgets exceeded");
			    return GTK_CAULDRON_ERROR;
			}
		    }
		    break;
		}
#ifdef HAVE_GNOME
	    case 'D':{
		gdouble *result;
		gint flags;
		next_arg (GTK_CAULDRON_TYPE_DOUBLE_P, user_data, &result);
		next_arg (GTK_CAULDRON_TYPE_INT, user_data, &flags);
		w = gnome_date_edit_new_flags ((time_t) *result, flags);
		if (!result) {
		    gtk_dialog_cauldron_set_error ("gtk_dialog_cauldron(): date edit widget: place to store result = NULL");
		    return GTK_CAULDRON_ERROR;
		}
		new_result (r, get_gnome_date_edit_result, w, result);
		if (option_is_present (p + 1, 'o')) {
		    f = 0;
		    get_child_entry (w, &f);
		}
		user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		if (!(top = widget_stack_top (stack)))
		    return GTK_CAULDRON_ERROR;
		if (gtk_cauldron_box_add (top, w, &p, pixels_per_space))
		    return GTK_CAULDRON_ERROR;
		gtk_widget_show (w);
		break;
	    }
#endif
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
		    if (option_is_present (p + 1, 'o')) {
			f = 0;
			get_child_entry (w, &f);
		    }
		    user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		    if (!(top = widget_stack_top (stack)))
			return GTK_CAULDRON_ERROR;
		    if (gtk_cauldron_box_add (top, w, &p, pixels_per_space))
			return GTK_CAULDRON_ERROR;
		    gtk_widget_show (w);
		    break;
		}
	    case 'B':{
		    gchar *label;
		    gint accelerator_key = 0, underbar_pos = -1;
		    next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &label);
		    b[ncauldron_button].label = label;
		    b[ncauldron_button].result = &return_result;
		    if (label) {
			label = convert_label_with_ampersand (label, &accelerator_key, &underbar_pos);
#ifdef HAVE_GNOME
			if (option_is_present (p + 1, 'g'))
			    w = gnome_stock_button (label);
			else
#endif
			    w = gtk_button_new_with_label (label);
		    } else
			w = gtk_button_new ();
		    if (option_is_present (p + 1, 'o'))
			f = w;
		    if (accelerator_key)
			add_accelerator_with_underbar (w, accel_table, accelerator_key, label, underbar_pos);
		    if (label)
			g_free (label);
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
		    if (!(top = widget_stack_top (stack)))
			return GTK_CAULDRON_ERROR;
		    if (gtk_cauldron_box_add (top, w, &p, pixels_per_space))
			return GTK_CAULDRON_ERROR;
		    gtk_widget_show (w);
		    ncauldron_button++;
		    break;
		}
	    case 'C':{
		    gchar *label;
		    gint accelerator_key = 0, underbar_pos = -1;
		    int *result;
		    next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &label);
		    if (label) {
			label = convert_label_with_ampersand (label, &accelerator_key, &underbar_pos);
			w = gtk_check_button_new_with_label (label);
		    } else
			w = gtk_check_button_new ();
		    if (option_is_present (p + 1, 'o'))
			f = w;
		    if (accelerator_key)
			add_accelerator_with_underbar (w, accel_table, accelerator_key, label, underbar_pos);
		    next_arg (GTK_CAULDRON_TYPE_INT_P, user_data, &result);
		    GTK_TOGGLE_BUTTON (w)->active = *result;
		    new_result (r, get_check_result, w, result);
		    user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		    if (!(top = widget_stack_top (stack)))
			return GTK_CAULDRON_ERROR;
		    if (gtk_cauldron_box_add (top, w, &p, pixels_per_space))
			return GTK_CAULDRON_ERROR;
		    gtk_widget_show (w);
		    if (label)
			g_free (label);
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
		    if (option_is_present (p + 1, 'o'))
			f = w;
		    user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		    if (!(top = widget_stack_top (stack)))
			return GTK_CAULDRON_ERROR;
		    if (gtk_cauldron_box_add (top, w, &p, pixels_per_space))
			return GTK_CAULDRON_ERROR;
		    gtk_widget_show (w);
		} else {
		    if (!(top = widget_stack_top (stack)))
			return GTK_CAULDRON_ERROR;
		    if (GTK_IS_HBOX (top)) {
			w = gtk_vseparator_new ();
		    } else {
			w = gtk_hseparator_new ();
		    }
		    user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		    if (!(top = widget_stack_top (stack)))
			return GTK_CAULDRON_ERROR;
		    if (gtk_cauldron_box_add (top, w, &p, pixels_per_space))
			return GTK_CAULDRON_ERROR;
		    gtk_widget_show (w);
		}
		break;
	    case 'R':{
		    gchar *label;
		    gint *result;
		    gint accelerator_key = 0, underbar_pos = -1;
		    next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &label);
		    if (label) {
			label = convert_label_with_ampersand (label, &accelerator_key, &underbar_pos);
			w = gtk_radio_button_new_with_label (radio_group, label);
		    } else
			w = gtk_radio_button_new (radio_group);
		    if (option_is_present (p + 1, 'o'))
			f = w;
		    if (accelerator_key)
			add_accelerator_with_underbar (w, accel_table, accelerator_key, label, underbar_pos);
		    radio_group = gtk_radio_button_group (GTK_RADIO_BUTTON (w));
		    next_arg (GTK_CAULDRON_TYPE_INT_P, user_data, &result);
		    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (w), *result);
		    new_result (r, get_check_result, w, result);
		    user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		    if (!(top = widget_stack_top (stack)))
			return GTK_CAULDRON_ERROR;
		    if (gtk_cauldron_box_add (top, w, &p, pixels_per_space))
			return GTK_CAULDRON_ERROR;
		    gtk_widget_show (w);
		    if (label)
			g_free (label);
		    break;
		}
	    case 'X':{
		    GtkCauldronCustomCallback func;
		    gpointer data;
		    next_arg (GTK_CAULDRON_TYPE_CALLBACK, user_data, &func);
		    next_arg (GTK_CAULDRON_TYPE_USERDATA_P, user_data, &data);
		    w = (*func) (window, data);		/* `window' is just to fill the first arg and shouldn't be needed by the user */
		    if (option_is_present (p + 1, 'o'))
			f = w;
		    if (w) {
			if (!(top = widget_stack_top (stack)))
			    return GTK_CAULDRON_ERROR;
			if (gtk_cauldron_box_add (top, w, &p, pixels_per_space))
			    return GTK_CAULDRON_ERROR;
			gtk_widget_show (w);
		    } else {
			gtk_dialog_cauldron_set_error ("gtk_dialog_cauldron(): user callback GtkCauldronCustomCallback returned null");
			return GTK_CAULDRON_ERROR;
		    }
		    break;
		}
	    }
	    break;
	case '}':
	case ')':
	case ']':
	    radio_group = 0;
	    w = widget_stack_pop (stack);
	    if (!w)
		return GTK_CAULDRON_ERROR;
	    if (GTK_IS_NOTEBOOK(w)) {
		gtk_notebook_set_page (GTK_NOTEBOOK (w), 0);
		w = widget_stack_pop (stack);
		if (!w)
		    return GTK_CAULDRON_ERROR;
	    }
	    if (option_is_present (p + 1, 'n')) {
		gchar *label;
/* adding to a notebook requires special handling : */
		if (!(top = widget_stack_top (stack)))
		    return GTK_CAULDRON_ERROR;
		if (!GTK_IS_NOTEBOOK(top)) {
		    GtkWidget *notebook;
		    notebook = gtk_notebook_new ();
		    if (option_is_present (p + 1, 'v'))
			gtk_notebook_set_tab_pos (GTK_NOTEBOOK (notebook), GTK_POS_LEFT);
		    while (p[1] && strchr (CAULDRON_FMT_OPTIONS, p[1]))
			p++;
		    if (!(top = widget_stack_top (stack)))
			return GTK_CAULDRON_ERROR;
		    if (gtk_cauldron_box_add (top, notebook, &p, pixels_per_space))
			return GTK_CAULDRON_ERROR;
		    gtk_widget_show (notebook);
		    if (widget_stack_push (stack, notebook))
			return GTK_CAULDRON_ERROR;
		} else {
		    while (p[1] && strchr (CAULDRON_FMT_OPTIONS, p[1]))
			p++;
		}
		next_arg (GTK_CAULDRON_TYPE_CHAR_P, user_data, &label);
		user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		if (!(top = widget_stack_top (stack)))
		    return GTK_CAULDRON_ERROR;
		gtk_notebook_append_page (GTK_NOTEBOOK (top), w, gtk_label_new_with_ampersand (label));
	    } else if (option_is_present (p + 1, 'v') || option_is_present (p + 1, 'h')) {
		GtkWidget *scrolled_window;
		scrolled_window = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolled_window),
						option_is_present (p + 1, 'h') ? GTK_POLICY_ALWAYS : GTK_POLICY_AUTOMATIC,
						option_is_present (p + 1, 'v') ? GTK_POLICY_ALWAYS : GTK_POLICY_AUTOMATIC);
		gtk_scrolled_window_add_with_viewport (GTK_SCROLLED_WINDOW (scrolled_window), w);
		gtk_widget_show (scrolled_window);
		user_callbacks (scrolled_window, p + 1, next_arg, user_data, accel_table);
		if (!(top = widget_stack_top (stack)))
		    return GTK_CAULDRON_ERROR;
		if (gtk_cauldron_box_add (top, scrolled_window, &p, pixels_per_space))
		    return GTK_CAULDRON_ERROR;
	    } else {
		user_callbacks (w, p + 1, next_arg, user_data, accel_table);
		if (!(top = widget_stack_top (stack)))
		    return GTK_CAULDRON_ERROR;
		if (gtk_cauldron_box_add (top, w, &p, pixels_per_space))
		    return GTK_CAULDRON_ERROR;
	    }
	    gtk_widget_show (w);
	    break;
	default:{
		gchar e[100] = "gtk_dialog_cauldron(): bad syntax in format string at: ";
		gint l;
		l = strlen (e);
		strncpy (e + l, p, 30);
		strcpy (e + l + 30, "...");
		gtk_dialog_cauldron_set_error (e);
		return GTK_CAULDRON_ERROR;
	    }
	}
    }
    if (stack->widget_stack_pointer != 1) {
	gtk_dialog_cauldron_set_error ("gtk_dialog_cauldron(): bracketing error in format string");
	return GTK_CAULDRON_ERROR;
    }
    w = widget_stack_pop (stack);
    if (!w)
	return GTK_CAULDRON_ERROR;
    gtk_widget_show (w);

    while (ntext_widget-- > 0) {
	gtk_text_freeze (GTK_TEXT (text_widget[ntext_widget]));
	gtk_text_insert (GTK_TEXT (text_widget[ntext_widget]), NULL, NULL, NULL, text[ntext_widget], -1);
	gtk_text_thaw (GTK_TEXT (text_widget[ntext_widget]));
    }

    if (options & GTK_CAULDRON_GRAB)
	gtk_grab_add (window);

    if (f)
	gtk_widget_grab_focus (f);
    if (parent) {
	if (GTK_IS_WINDOW (parent)) {
	    if ((unsigned long) parent != (unsigned long) window) {
		gtk_window_set_transient_for (GTK_WINDOW (window), GTK_WINDOW (parent));
#ifdef HAVE_GNOME
		/* This code is duplicated in gnome-file-entry.c:browse-clicked.  If
		 * a change is made here, update it there too. */
		/* This code is duplicated in gnome-dialog.c:gnome_dialog_set_parent().  If
		 * a change is made here, update it there too. */
		if (gnome_preferences_get_dialog_centered ()) {
		    gint x, y, w, h, window_x, window_y;
		    if (GTK_WIDGET_VISIBLE (parent)) {
			gtk_window_set_position (GTK_WINDOW (window), GTK_WIN_POS_NONE);
			gdk_window_get_origin (GTK_WIDGET (parent)->window, &x, &y);
			gdk_window_get_size (GTK_WIDGET (parent)->window, &w, &h);
			window_x = x + w / 4;
			window_y = y + h / 4;
			gtk_widget_set_uposition (GTK_WIDGET (window), window_x, window_y);
		    }
		}
#endif
	    }
	}
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
    free (stack);
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

/**
 * gtk_dialog_cauldron:
 * @title: dialog title
 * @options: dialog options, see the macro definitions
 * @format: dialog layout format string
 *
 * This function parses a @format string with
 * a variable length list of arguments. The @format string describes a
 * dialog box and has intuitive tokens to represent different frames and
 * widgets. The dialog box is drawn whereupon gtk_dialog_cauldron()
 * blocks until closed or until an appropriate button is pushed. Results
 * from the widgets are then stored into appropriate variables passed in
 * the argument list in order to be retrieved by the caller.
 *
 * Retuns NULL is returned if the dialog is
 * cancelled. GTK_CAULDRON_ENTERP is returned if the user pressed
 * enter (return-on-enter can be overridden - see global options
 * below), and GTK_CAULDRON_ESCAPEP is returned if the user
 * pressed escape. Otherwise the label of the widget that was used to
 * exit the dialog is returned.
 * 
 */
gchar *gtk_dialog_cauldron (const gchar * title, glong options,...)
{
    gchar *r;
    GtkWidget *parent = 0;
    char *format;
    va_list ap;
    va_start (ap, options);
    if (options & GTK_CAULDRON_PARENT)
	parent = va_arg (ap, GtkWidget *);
    format = va_arg (ap, char *);
    r = gtk_dialog_cauldron_parse (title, options, format, (GtkCauldronNextArgCallback) next_arg, &ap, parent);
    va_end (ap);
    if (r == GTK_CAULDRON_ERROR)
	g_error (gtk_dialog_cauldron_get_error ());
    return r;
}				/* }}} main */

