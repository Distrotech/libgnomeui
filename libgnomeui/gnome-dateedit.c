/*
 * Date editor widget
 * Copyright (C) 1998 the Free Software Foundation
 *
 * Author: Miguel de Icaza
 */
#include <config.h>
#include <ctype.h> /* isdigit */
#include <string.h>
#include <stdlib.h> /* atoi */
#include <stdio.h>
#include <time.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "gnome-dateedit.h"
#include "libgnome/gnome-i18nP.h"
#include "libgnomeui/gnome-stock.h"
#include "libgnomeui/gnome-window-icon.h"

enum {
	DATE_CHANGED,
	TIME_CHANGED,
	LAST_SIGNAL
};

static gint date_edit_signals [LAST_SIGNAL] = { 0 };


static void gnome_date_edit_init         (GnomeDateEdit      *gde);
static void gnome_date_edit_class_init   (GnomeDateEditClass *class);
static void gnome_date_edit_destroy      (GtkObject          *object);
static void gnome_date_edit_forall       (GtkContainer       *container,
					  gboolean	      include_internals,
					  GtkCallback	      callback,
					  gpointer	      callbabck_data);

static GtkHBoxClass *parent_class;

/**
 * gnome_date_edit_get_type:
 *
 * Returns the GtkType for the GnomeDateEdit widget
 */
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

	gtk_calendar_get_date (calendar, &year, &month, &day);

	/* FIXME: internationalize this - strftime()*/
	g_snprintf (buffer, sizeof(buffer), "%d/%d/%d", month + 1, day, year);
	gtk_entry_set_text (GTK_ENTRY (gde->date_entry), buffer);
	gtk_signal_emit (GTK_OBJECT (gde), date_edit_signals [DATE_CHANGED]);
}

static void
day_selected_double_click (GtkCalendar *calendar, GnomeDateEdit *gde)
{
	hide_popup (gde);
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
	GtkRequisition requisition;

	gtk_widget_size_request (gde->cal_popup, &requisition);

	gdk_window_get_origin (gde->date_button->window, &x, &y);
	gdk_window_get_size (gde->date_button->window, &bwidth, &bheight);

	x += bwidth - requisition.width;
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
	struct tm mtm = {0};
	GdkCursor *cursor;

        /* This code is pretty much just copied from gtk_date_edit_get_date */
      	sscanf (gtk_entry_get_text (GTK_ENTRY (gde->date_entry)), "%d/%d/%d",
		&mtm.tm_mon, &mtm.tm_mday, &mtm.tm_year); /* FIXME: internationalize this - strptime()*/

	mtm.tm_mon = CLAMP (mtm.tm_mon, 1, 12);
	mtm.tm_mday = CLAMP (mtm.tm_mday, 1, 31);

        mtm.tm_mon--;

	/* Hope the user does not actually mean years early in the A.D. days...
	 * This date widget will obviously not work for a history program :-)
	 */
	if (mtm.tm_year >= 1900)
		mtm.tm_year -= 1900;

	gtk_calendar_select_month (GTK_CALENDAR (gde->calendar), mtm.tm_mon, 1900 + mtm.tm_year);
        gtk_calendar_select_day (GTK_CALENDAR (gde->calendar), mtm.tm_mday);

        position_popup (gde);
       
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

		if (gde->flags & GNOME_DATE_EDIT_24_HR)
			strftime (buffer, sizeof (buffer), "%H:00", mtm);
		else
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
			if (gde->flags & GNOME_DATE_EDIT_24_HR)
				strftime (buffer, sizeof (buffer), "%H:%M", mtm);
			else
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
	GtkContainerClass *container_class = (GtkContainerClass *) class;

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

	container_class->forall = gnome_date_edit_forall;

	object_class->destroy = gnome_date_edit_destroy;

	class->date_changed = NULL;
	class->time_changed = NULL;
}

static void
gnome_date_edit_init (GnomeDateEdit *gde)
{
	gde->lower_hour = 7;
	gde->upper_hour = 19;
	gde->flags = GNOME_DATE_EDIT_SHOW_TIME;
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

static void
gnome_date_edit_forall (GtkContainer *container, gboolean include_internals,
			GtkCallback callback, gpointer callback_data)
{
	g_return_if_fail (container != NULL);
	g_return_if_fail (GNOME_IS_DATE_EDIT (container));
	g_return_if_fail (callback != NULL);

	/* Let GtkBox handle things only if the internal widgets need to be
	 * poked.
	 */
	if (include_internals)
		if (GTK_CONTAINER_CLASS (parent_class)->forall)
			(* GTK_CONTAINER_CLASS (parent_class)->forall) (container,
									include_internals,
									callback,
									callback_data);
}

/**
 * gnome_date_edit_set_time:
 * @gde: the GnomeDateEdit widget
 * @the_time: The time and date that should be set on the widget
 *
 * Changes the displayed date and time in the GnomeDateEdit widget
 * to be the one represented by @the_time.
 */
void
gnome_date_edit_set_time (GnomeDateEdit *gde, time_t the_time)
{
	struct tm *mytm;
	char buffer [40];

	g_return_if_fail(gde != NULL);

	if (the_time == 0)
		the_time = time (NULL);
	gde->initial_time = the_time;

	mytm = localtime (&the_time);

	/* Set the date */
	g_snprintf (buffer, sizeof(buffer), "%d/%d/%d",
		    mytm->tm_mon + 1,
		    mytm->tm_mday,
		    1900 + mytm->tm_year);
	gtk_entry_set_text (GTK_ENTRY (gde->date_entry), buffer);

	/* Set the time */
	if (gde->flags & GNOME_DATE_EDIT_24_HR)
		strftime (buffer, sizeof (buffer), "%H:%M", mytm);
	else
		strftime (buffer, sizeof (buffer), "%I:%M %p", mytm);
	gtk_entry_set_text (GTK_ENTRY (gde->time_entry), buffer);
}

/**
 * gnome_date_edit_set_popup_range:
 * @gde: The GnomeDateEdit widget
 * @low_hour: low boundary for the time-range display popup.
 * @up_hour:  upper boundary for the time-range display popup.
 *
 * Sets the range of times that will be provide by the time popup
 * selectors.
 */
void
gnome_date_edit_set_popup_range (GnomeDateEdit *gde, int low_hour, int up_hour)
{
        g_return_if_fail(gde != NULL);

	gde->lower_hour = low_hour;
	gde->upper_hour = up_hour;

        fill_time_popup(NULL, gde);
}

static void
create_children (GnomeDateEdit *gde)
{
	GtkWidget *frame;
	GtkWidget *hbox;
	GtkWidget *arrow;

	gde->date_entry  = gtk_entry_new ();
	gtk_widget_set_usize (gde->date_entry, 90, 0);
	gtk_box_pack_start (GTK_BOX (gde), gde->date_entry, TRUE, TRUE, 0);
	gtk_widget_show (gde->date_entry);
	
	gde->date_button = gtk_button_new ();
	gtk_signal_connect (GTK_OBJECT (gde->date_button), "clicked",
			    GTK_SIGNAL_FUNC (select_clicked), gde);
	gtk_box_pack_start (GTK_BOX (gde), gde->date_button, FALSE, FALSE, 0);

	hbox = gtk_hbox_new (FALSE, 3);
	gtk_container_add (GTK_CONTAINER (gde->date_button), hbox);
	gtk_widget_show (hbox);

	/* Calendar label, only shown if the date editor has a time field */

	gde->cal_label = gtk_label_new (_("Calendar"));
	gtk_misc_set_alignment (GTK_MISC (gde->cal_label), 0.0, 0.5);
	gtk_box_pack_start (GTK_BOX (hbox), gde->cal_label, TRUE, TRUE, 0);
	if (gde->flags & GNOME_DATE_EDIT_SHOW_TIME)
		gtk_widget_show (gde->cal_label);

	arrow = gtk_arrow_new (GTK_ARROW_DOWN, GTK_SHADOW_OUT);
	gtk_box_pack_start (GTK_BOX (hbox), arrow, FALSE, FALSE, 0);
	gtk_widget_show (arrow);

	gtk_widget_show (gde->date_button);

	gde->time_entry = gtk_entry_new_with_max_length (12);
	gtk_widget_set_usize (gde->time_entry, 88, 0);
	gtk_box_pack_start (GTK_BOX (gde), gde->time_entry, TRUE, TRUE, 0);

	gde->time_popup = gtk_option_menu_new ();
	gtk_box_pack_start (GTK_BOX (gde), gde->time_popup, FALSE, FALSE, 0);

	/* We do not create the popup menu with the hour range until we are
	 * realized, so that it uses the values that the user might supply in a
	 * future call to gnome_date_edit_set_popup_range
	 */
	gtk_signal_connect (GTK_OBJECT (gde), "realize",
			    GTK_SIGNAL_FUNC (fill_time_popup), gde);

	if (gde->flags & GNOME_DATE_EDIT_SHOW_TIME) {
		gtk_widget_show (gde->time_entry);
		gtk_widget_show (gde->time_popup);
	}

	gde->cal_popup = gtk_window_new (GTK_WINDOW_POPUP);
	gnome_window_icon_set_from_default (GTK_WINDOW (gde->cal_popup));
	gtk_widget_set_events (gde->cal_popup,
			       gtk_widget_get_events (gde->cal_popup) | GDK_KEY_PRESS_MASK);
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
				      (GTK_CALENDAR_SHOW_DAY_NAMES
				       | GTK_CALENDAR_SHOW_HEADING
				       | ((gde->flags & GNOME_DATE_EDIT_WEEK_STARTS_ON_MONDAY)
					  ? GTK_CALENDAR_WEEK_START_MONDAY : 0)));
	gtk_signal_connect (GTK_OBJECT (gde->calendar), "day_selected",
			    GTK_SIGNAL_FUNC (day_selected), gde);
	gtk_signal_connect (GTK_OBJECT (gde->calendar), "day_selected_double_click",
			    GTK_SIGNAL_FUNC (day_selected_double_click), gde);
	gtk_container_add (GTK_CONTAINER (frame), gde->calendar);
        gtk_widget_show (gde->calendar);
}

/**
 * gnome_date_edit_new:
 * @the_time: date and time to be displayed on the widget
 * @show_time: whether time should be displayed
 * @use_24_format: whether 24-hour format is desired for the time display.
 *
 * Creates a new GnomeDateEdit widget which can be used to provide
 * an easy to use way for entering dates and times.
 * 
 * Returns a GnomeDateEdit widget.
 */
GtkWidget *
gnome_date_edit_new (time_t the_time, int show_time, int use_24_format)
{
	return gnome_date_edit_new_flags (the_time,
					  ((show_time ? GNOME_DATE_EDIT_SHOW_TIME : 0)
					   | (use_24_format ? GNOME_DATE_EDIT_24_HR : 0)));
}

/**
 * gnome_date_edit_new_flags:
 * @the_time: The initial time for the date editor.
 * @flags: A bitmask of GnomeDateEditFlags values.
 * 
 * Creates a new GnomeDateEdit widget with the specified flags.
 * 
 * Return value: the newly-created date editor widget.
 **/
GtkWidget *
gnome_date_edit_new_flags (time_t the_time, GnomeDateEditFlags flags)
{
	GnomeDateEdit *gde;

	gde = gtk_type_new (gnome_date_edit_get_type ());

	gde->flags = flags;
	create_children (gde);
	gnome_date_edit_set_time (gde, the_time);

	return GTK_WIDGET (gde);
}

/**
 * gnome_date_edit_get_date:
 * @gde: The GnomeDateEdit widget
 *
 * Returns the time entered in the GnomeDateEdit widget
 */
time_t
gnome_date_edit_get_date (GnomeDateEdit *gde)
{
	struct tm tm = {0};
	char *str, *flags = NULL;

	/* Assert, because we're just hosed if it's NULL */
	g_assert(gde != NULL);
	g_assert(GNOME_IS_DATE_EDIT(gde));
	
	sscanf (gtk_entry_get_text (GTK_ENTRY (gde->date_entry)), "%d/%d/%d",
		&tm.tm_mon, &tm.tm_mday, &tm.tm_year); /* FIXME: internationalize this - strptime()*/

	tm.tm_mon = CLAMP (tm.tm_mon, 1, 12);
	tm.tm_mday = CLAMP (tm.tm_mday, 1, 31);

	tm.tm_mon--;

	/* Hope the user does not actually mean years early in the A.D. days...
	 * This date widget will obviously not work for a history program :-)
	 */
	if (tm.tm_year >= 1900)
		tm.tm_year -= 1900;

	if (gde->flags & GNOME_DATE_EDIT_SHOW_TIME) {
		char *tokp, *temp;

		str = g_strdup (gtk_entry_get_text (GTK_ENTRY (gde->time_entry)));
		temp = strtok_r (str, ": ", &tokp);
		if (temp) {
			tm.tm_hour = atoi (temp);
			temp = strtok_r (NULL, ": ", &tokp);
			if (temp) {
				if (isdigit (*temp)) {
					tm.tm_min = atoi (temp);
					flags = strtok_r (NULL, ": ", &tokp);
					if (flags && isdigit (*flags)) {
						tm.tm_sec = atoi (flags);
						flags = strtok_r (NULL, ": ", &tokp);
					}
				} else
					flags = temp;
			}
		}

		if (flags && (strcasecmp (flags, "PM") == 0)){
			if (tm.tm_hour < 12)
				tm.tm_hour += 12;
		}
		g_free (str);
	}

	tm.tm_isdst = -1;

	return mktime (&tm);
}

/**
 * gnome_date_edit_set_flags:
 * @gde: The date editor widget whose flags should be changed.
 * @flags: The new bitmask of GnomeDateEditFlags values.
 * 
 * Changes the display flags on an existing date editor widget.
 **/
void
gnome_date_edit_set_flags (GnomeDateEdit *gde, GnomeDateEditFlags flags)
{
        GnomeDateEditFlags old_flags;
        
	g_return_if_fail (gde != NULL);
	g_return_if_fail (GNOME_IS_DATE_EDIT (gde));

        old_flags = gde->flags;
        gde->flags = flags;
        
	if ((flags & GNOME_DATE_EDIT_SHOW_TIME) != (old_flags & GNOME_DATE_EDIT_SHOW_TIME)) {
		if (flags & GNOME_DATE_EDIT_SHOW_TIME) {
			gtk_widget_show (gde->cal_label);
			gtk_widget_show (gde->time_entry);
			gtk_widget_show (gde->time_popup);
		} else {
			gtk_widget_hide (gde->cal_label);
			gtk_widget_hide (gde->time_entry);
			gtk_widget_hide (gde->time_popup);
		}
	}

	if ((flags & GNOME_DATE_EDIT_24_HR) != (old_flags & GNOME_DATE_EDIT_24_HR))
		fill_time_popup (GTK_WIDGET (gde), gde); /* This will destroy the old menu properly */

	if ((flags & GNOME_DATE_EDIT_WEEK_STARTS_ON_MONDAY)
	    != (old_flags & GNOME_DATE_EDIT_WEEK_STARTS_ON_MONDAY)) {
		if (flags & GNOME_DATE_EDIT_WEEK_STARTS_ON_MONDAY)
			gtk_calendar_display_options (GTK_CALENDAR (gde->calendar),
						      (GTK_CALENDAR (gde->calendar)->display_flags
						       | GTK_CALENDAR_WEEK_START_MONDAY));
		else
			gtk_calendar_display_options (GTK_CALENDAR (gde->calendar),
						      (GTK_CALENDAR (gde->calendar)->display_flags
						       & ~GTK_CALENDAR_WEEK_START_MONDAY));
	}
}

/**
 * gnome_date_edit_get_flags:
 * @gde: The date editor whose flags should be queried.
 * 
 * Queries the display flags on a date editor widget.
 * 
 * Return value: The current display flags for the specified date editor widget.
 **/
int
gnome_date_edit_get_flags (GnomeDateEdit *gde)
{
	g_return_val_if_fail (gde != NULL, 0);
	g_return_val_if_fail (GNOME_IS_DATE_EDIT (gde), 0);

	return gde->flags;
}
