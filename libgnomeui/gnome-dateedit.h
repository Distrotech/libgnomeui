/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
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

#ifndef __GNOME_DATE_EDIT_H_
#define __GNOME_DATE_EDIT_H_ 

#include <gtk/gtkhbox.h>
#include <libgnome/gnome-defs.h>
 
BEGIN_GNOME_DECLS


typedef enum {
	GNOME_DATE_EDIT_SHOW_TIME             = 1 << 0,
	GNOME_DATE_EDIT_24_HR                 = 1 << 1,
	GNOME_DATE_EDIT_WEEK_STARTS_ON_MONDAY = 1 << 2
} GnomeDateEditFlags;


#define GNOME_TYPE_DATE_EDIT            (gnome_date_edit_get_type ())
#define GNOME_DATE_EDIT(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_DATE_EDIT, GnomeDateEdit))
#define GNOME_DATE_EDIT_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_DATE_EDIT, GnomeDateEditClass))
#define GNOME_IS_DATE_EDIT(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_DATE_EDIT))
#define GNOME_IS_DATE_EDIT_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_DATE_EDIT))
#define GNOME_DATE_EDIT_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_DATE_EDIT, GnomeDateEditClass))

typedef struct {
	GtkHBox hbox;

	GtkWidget *date_entry;
	GtkWidget *date_button;
	
	GtkWidget *time_entry;
	GtkWidget *time_popup;

	GtkWidget *cal_label;
	GtkWidget *cal_popup;
	GtkWidget *calendar;

	time_t    initial_time;

	int       lower_hour;
	int       upper_hour;
	
	int       flags;
} GnomeDateEdit;

typedef struct {
	GtkHBoxClass parent_class;
	void (*date_changed) (GnomeDateEdit *gde);
	void (*time_changed) (GnomeDateEdit *gde);
} GnomeDateEditClass;

guint     gnome_date_edit_get_type        (void);
GtkWidget *gnome_date_edit_new            (time_t the_time, int show_time, int use_24_format);
GtkWidget *gnome_date_edit_new_flags      (time_t the_time, GnomeDateEditFlags flags);

void      gnome_date_edit_set_time        (GnomeDateEdit *gde, time_t the_time);
void      gnome_date_edit_set_popup_range (GnomeDateEdit *gde, int low_hour, int up_hour);
time_t    gnome_date_edit_get_date        (GnomeDateEdit *gde);
void      gnome_date_edit_set_flags       (GnomeDateEdit *gde, GnomeDateEditFlags flags);
int       gnome_date_edit_get_flags       (GnomeDateEdit *gde);


END_GNOME_DECLS

#endif
