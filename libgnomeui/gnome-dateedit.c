/*
 * Date editor widget
 * Copyright (C) 1998 the Free Software Foundation
 *
 * Author: Miguel de Icaza
 */
#include <config.h>
#include <string.h>
#include <time.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "gnome-dateedit.h"
#include <libgnome/gnome-i18n.h>
#include <libgnomeui/gtkcalendar.h>
#include <libgnomeui/gnome-stock.h>


enum {
	DATE_CHANGED,
	TIME_CHANGED,
	LAST_SIGNAL
};

static gint date_edit_signals [LAST_SIGNAL] = { 0 };


static void gnome_date_edit_init         (GnomeDateEdit      *gde);
static void gnome_date_edit_class_init   (GnomeDateEditClass *class);
static void gnome_date_edit_destroy      (GtkObject          *object);


static GtkHBoxClass *parent_class;


guint
gnome_date_edit_get_type (void)
{
	static guint date_edit_type = 0;

	if (!date_edit_type){
		GtkTypeInfo date_edit_info = {
			"GnomeDateEdit",
			sizeof (GnomeDateEdit),
			sizeof (GnomeDateEditClass),
			(GtkClassInitFunc) gnome_date_edit_class_init,
			(GtkObjectInitFunc) gnome_date_edit_init,
			NULL,
			NULL,
		};

		date_edit_type = gtk_type_unique (gtk_hbox_get_type (), &date_edit_info);
	}
	
	return date_edit_type;
}

static void
hide_popup (GnomeDateEdit *gde)
{
	gtk_widget_hide (gde->cal_popup);
	gtk_grab_remove (gde->cal_popup);
	gdk_pointer_ungrab (GDK_CURRENT_TIME);
}

static void
day_selected (GtkCalendar *calendar, GnomeDateEdit *gde)
{
	char buffer [40];
	gint year, month, day;

	hide_popup (gde);

	gtk_calendar_get_date (calendar, &year, &month, &day);

	sprintf (buffer, "%d/%d/%d", month + 1, day, year); /* FIXME: internationalize this - strftime()*/
	gtk_entry_set_text (GTK_ENTRY (gde->date_entry), buffer);
	gtk_signal_emit (GTK_OBJECT (gde), date_edit_signals [DATE_CHANGED]);
}

static gint
delete_popup (GtkWidget *widget, gpointer data)
{
	GnomeDateEdit *gde;

	gde = data;
	hide_popup (gde);

	return TRUE;
}

static gint
key_press_popup (GtkWidget *widget, GdkEventKey *event, gpointer data)
{
	GnomeDateEdit *gde;

	if (event->keyval != GDK_Escape)
		return FALSE;

	gde = data;
	gtk_signal_emit_stop_by_name (GTK_OBJECT (widget), "key_press_event");
	hide_popup (gde);

	return TRUE;
}

/* This function is yanked from gtkcombo.c */
static gint
button_press_popup (GtkWidget *widget, GdkEventButton *event, gpointer data)
{
	GnomeDateEdit *gde;
	GtkWidget *child;

	gde = data;

	child = gtk_get_event_widget ((GdkEvent *) event);

	/* We don't ask for button press events on the grab widget, so
	 *  if an event is reported directly to the grab widget, it must
	 *  be on a window outside the application (and thus we remove
	 *  the popup window). Otherwise, we check if the widget is a child
	 *  of the grab widget, and only remove the popup window if it
	 *  is not.
	 */
	if (child != widget) {
		while (child) {
			if (child == widget)
				return FALSE;
			child = child->parent;
		}
	}

	hide_popup (gde);

	return TRUE;
}

static void
position_popup (GnomeDateEdit *gde)
{
	gint x, y;
	gint bwidth, bheight;

	gtk_widget_size_request (gde->cal_popup, &gde->cal_popup->requisition);

	gdk_window_get_origin (gde->date_button->window, &x, &y);
	gdk_window_get_size (gde->date_button->window, &bwidth, &bheight);

	x += bwidth - gde->cal_popup->requisition.width;
	y += bheight;

	if (x < 0)
		x = 0;

	if (y < 0)
		y = 0;

	gtk_widget_set_uposition (gde->cal_popup, x, y);
}

static void
select_clicked (GtkWidget *widget, GnomeDateEdit *gde)
{
	struct tm *mtm;
	GdkCursor *cursor;

	mtm = localtime (&gde->initial_time);

	/* FIXME: this would be better if we set the date from what is displayed */
	gtk_calendar_select_month (GTK_CALENDAR (gde->calendar), mtm->tm_mon, 1900 + mtm->tm_year);

	position_popup (gde);

	gtk_widget_realize (gde->cal_popup);
	gtk_widget_show (gde->cal_popup);
	gtk_widget_grab_focus (gde->cal_popup);
	gtk_grab_add (gde->cal_popup);

	cursor = gdk_cursor_new (GDK_ARROW);

	gdk_pointer_grab (gde->cal_popup->window, TRUE,
			  (GDK_BUTTON_PRESS_MASK
			   | GDK_BUTTON_RELEASE_MASK
			   | GDK_POINTER_MOTION_MASK),
			  NULL, cursor, GDK_CURRENT_TIME);

	gdk_cursor_destroy (cursor);
}

typedef struct {
	char *hour;
	GnomeDateEdit *gde;
} hour_info_t;

static void
set_time (GtkWidget *widget, hour_info_t *hit)
{
	gtk_entry_set_text (GTK_ENTRY (hit->gde->time_entry), hit->hour);
	gtk_signal_emit (GTK_OBJECT (hit->gde), date_edit_signals [TIME_CHANGED]);
}

static void
free_resources (GtkWidget *widget, hour_info_t *hit)
{
	g_free (hit->hour);
	g_free (hit);
}

static void
fill_time_popup (GtkWidget *widget, GnomeDateEdit *gde)
{
	GtkWidget *menu;
	struct tm *mtm;
	time_t current_time;
	int i, j;

	if (gde->lower_hour > gde->upper_hour)
		return;

	menu = gtk_menu_new ();
	gtk_option_menu_set_menu (GTK_OPTION_MENU (gde->time_popup), menu);

	time (&current_time);
	mtm = localtime (&current_time);
	
	for (i = gde->lower_hour; i <= gde->upper_hour; i++){
		GtkWidget *item, *submenu;
		hour_info_t *hit;
		char buffer [40];

		mtm->tm_hour = i;
		mtm->tm_min  = 0;
		hit = g_new (hour_info_t, 1);
		
		strftime (buffer, sizeof (buffer), "%I:00 %p ", mtm);
		hit->hour = g_strdup (buffer);
		hit->gde  = gde;

		item = gtk_menu_item_new_with_label (buffer);
		gtk_menu_append (GTK_MENU (menu), item);
#if 0
		gtk_signal_connect (GTK_OBJECT (item), "activate",
				    GTK_SIGNAL_FUNC (set_time), hit);
#endif
		gtk_signal_connect (GTK_OBJECT (item), "destroy",
				    GTK_SIGNAL_FUNC (free_resources), hit);
		gtk_widget_show (item);

		submenu = gtk_menu_new ();
		gtk_menu_item_set_submenu (GTK_MENU_ITEM (item), submenu);
		for (j = 0; j < 60; j += 15){
			GtkWidget *mins;

			mtm->tm_min = j;
			hit = g_new (hour_info_t, 1);
			strftime (buffer, sizeof (buffer), "%I:%M %p", mtm);
			hit->hour = g_strdup (buffer);
			hit->gde  = gde;

			mins = gtk_menu_item_new_with_label (buffer);
			gtk_menu_append (GTK_MENU (submenu), mins);
			gtk_signal_connect (GTK_OBJECT (mins), "activate",
					    GTK_SIGNAL_FUNC (set_time), hit);
			gtk_signal_connect (GTK_OBJECT (item), "destroy",
					    GTK_SIGNAL_FUNC (free_resources), hit);
			gtk_widget_show (mins);
		}
	}
}

static void
gnome_date_edit_class_init (GnomeDateEditClass *class)
{
	GtkObjectClass *object_class = (GtkObjectClass *) class;

	object_class = (GtkObjectClass*) class;

	parent_class = gtk_type_class (gtk_hbox_get_type ());
	
	date_edit_signals [TIME_CHANGED] =
		gtk_signal_new ("time_changed",
				GTK_RUN_FIRST, object_class->type, 
				GTK_SIGNAL_OFFSET (GnomeDateEditClass, time_changed),
				gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
	
	date_edit_signals [DATE_CHANGED] =
		gtk_signal_new ("date_changed",
				GTK_RUN_FIRST, object_class->type, 
				GTK_SIGNAL_OFFSET (GnomeDateEditClass, date_changed),
				gtk_signal_default_marshaller, GTK_TYPE_NONE, 0);
	
	gtk_object_class_add_signals (object_class, date_edit_signals, LAST_SIGNAL);

	object_class->destroy = gnome_date_edit_destroy;

	class->date_changed = NULL;
	class->time_changed = NULL;
}

static void
gnome_date_edit_init (GnomeDateEdit *gde)
{
	GtkWidget *frame;

	gde->lower_hour = 7;
	gde->upper_hour = 19;
	
	gde->date_entry  = gtk_entry_new ();
	gtk_widget_set_usize (gde->date_entry, 90, 0);
	gtk_box_pack_start (GTK_BOX (gde), gde->date_entry, TRUE, TRUE, 0);
	gtk_widget_show (gde->date_entry);
	
	gde->date_button = gtk_button_new_with_label (_("Calendar..."));
	gtk_signal_connect (GTK_OBJECT (gde->date_button), "clicked",
			    GTK_SIGNAL_FUNC (select_clicked), gde);
	gtk_box_pack_start (GTK_BOX (gde), gde->date_button, FALSE, FALSE, 0);
	gtk_widget_show (gde->date_button);

	gde->time_entry = gtk_entry_new_with_max_length (9);
	gtk_widget_set_usize (gde->time_entry, 88, 0);
	gtk_box_pack_start (GTK_BOX (gde), gde->time_entry, TRUE, TRUE, 0);
	gtk_widget_show (gde->date_entry);

	gde->time_popup = gtk_option_menu_new ();
	gtk_box_pack_start (GTK_BOX (gde), gde->time_popup, FALSE, FALSE, 0);
	gtk_widget_show (gde->time_popup);

	/* We do not create the popup menu with the hour range until we are
	 * realized, so that it uses the values that the user might supply
	 * in a future call to gnome_date_edit_set_popup_range
	 */
	gtk_signal_connect (GTK_OBJECT (gde), "realize",
			    GTK_SIGNAL_FUNC (fill_time_popup), gde);

	gde->cal_popup = gtk_window_new (GTK_WINDOW_POPUP);
	gtk_widget_set_events (gde->cal_popup, gtk_widget_get_events (gde->cal_popup) | GDK_KEY_PRESS_MASK);
	gtk_signal_connect (GTK_OBJECT (gde->cal_popup), "delete_event",
			    (GtkSignalFunc) delete_popup,
			    gde);
	gtk_signal_connect (GTK_OBJECT (gde->cal_popup), "key_press_event",
			    (GtkSignalFunc) key_press_popup,
			    gde);
	gtk_signal_connect (GTK_OBJECT (gde->cal_popup), "button_press_event",
			    (GtkSignalFunc) button_press_popup,
			    gde);
	gtk_window_set_policy (GTK_WINDOW (gde->cal_popup), FALSE, FALSE, TRUE);

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (gde->cal_popup), frame);
	gtk_widget_show (frame);

	gde->calendar = gtk_calendar_new ();
	gtk_calendar_display_options (GTK_CALENDAR (gde->calendar),
				      GTK_CALENDAR_SHOW_DAY_NAMES | GTK_CALENDAR_SHOW_HEADING);
	gtk_signal_connect (GTK_OBJECT (gde->calendar), "day_selected",
			    GTK_SIGNAL_FUNC (day_selected), gde);
	gtk_container_add (GTK_CONTAINER (frame), gde->calendar);
	gtk_widget_show (gde->calendar);
}

static void
gnome_date_edit_destroy (GtkObject *object)
{
	GnomeDateEdit *gde;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_DATE_EDIT (object));

	gde = GNOME_DATE_EDIT (object);

	gtk_widget_destroy (gde->cal_popup);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

void
gnome_date_edit_set_time (GnomeDateEdit *gde, time_t the_time)
{
	struct tm *mytm;
	char buffer [40];

	if (the_time == 0)
		the_time = time (NULL);
	gde->initial_time = the_time;

	mytm = localtime (&the_time);

	/* Set the date */
	sprintf (buffer, "%d/%d/%d", mytm->tm_mon + 1, mytm->tm_mday, 1900 + mytm->tm_year);
	gtk_entry_set_text (GTK_ENTRY (gde->date_entry), buffer);

	/* Set the time */
	strftime (buffer, sizeof (buffer), "%I:00 %p", mytm);
	gtk_entry_set_text (GTK_ENTRY (gde->time_entry), buffer);
}

void
gnome_date_edit_set_popup_range (GnomeDateEdit *gde, int low_hour, int up_hour)
{
	gde->lower_hour = low_hour;
	gde->upper_hour = up_hour;
}

GtkWidget *
gnome_date_edit_new (time_t the_time)
{
	GnomeDateEdit *gde;

	gde = gtk_type_new (gnome_date_edit_get_type ());
	gnome_date_edit_set_time (gde, the_time);

	return GTK_WIDGET (gde);
}

time_t
gnome_date_edit_get_date (GnomeDateEdit *gde)
{
	struct tm tm;
	char *str, *flags;
	
	sscanf (gtk_entry_get_text (GTK_ENTRY (gde->date_entry)), "%d/%d/%d",
		&tm.tm_mon, &tm.tm_mday, &tm.tm_year); /* FIXME: internationalize this - strptime()*/

	tm.tm_mon--;

	/* Hope the user does not actually mean years early in the A.D. days...
	 * This date widget will obviously not work for a history program :-)
	 */
	if (tm.tm_year >= 1900)
		tm.tm_year -= 1900;

	str = g_strdup (gtk_entry_get_text (GTK_ENTRY (gde->time_entry)));
	tm.tm_hour = atoi (strtok (str, ":"));
	tm.tm_min  = atoi (strtok (NULL, ": "));
	flags = strtok (NULL, ":");

	if (flags && (strcasecmp (flags, "PM") == 0)){
		if (tm.tm_hour < 12)
			tm.tm_hour += 12;
	}
	g_free (str);
	tm.tm_sec = 0;
	tm.tm_isdst = -1;

	return mktime (&tm);
}

