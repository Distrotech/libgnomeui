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

#include <time.h>
#include <gtk/gtk.h>

 
G_BEGIN_DECLS


typedef enum {
	GNOME_DATE_EDIT_SHOW_TIME             = 1 << 0,
	GNOME_DATE_EDIT_24_HR                 = 1 << 1,
	GNOME_DATE_EDIT_WEEK_STARTS_ON_MONDAY = 1 << 2,
	GNOME_DATE_EDIT_DISPLAY_SECONDS       = 1 << 3
} GnomeDateEditFlags;


#define GNOME_TYPE_DATE_EDIT            (gnome_date_edit_get_type ())
#define GNOME_DATE_EDIT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_DATE_EDIT, GnomeDateEdit))
#define GNOME_DATE_EDIT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_DATE_EDIT, GnomeDateEditClass))
#define GNOME_IS_DATE_EDIT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_DATE_EDIT))
#define GNOME_IS_DATE_EDIT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_DATE_EDIT))
#define GNOME_DATE_EDIT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_DATE_EDIT, GnomeDateEditClass))

typedef struct _GnomeDateEdit        GnomeDateEdit;
typedef struct _GnomeDateEditPrivate GnomeDateEditPrivate;
typedef struct _GnomeDateEditClass   GnomeDateEditClass;

struct _GnomeDateEdit {
	GtkHBox hbox;

	/*< private >*/
	GnomeDateEditPrivate *_priv;
};

struct _GnomeDateEditClass {
	GtkHBoxClass parent_class;

	void (*date_changed) (GnomeDateEdit *gde);
	void (*time_changed) (GnomeDateEdit *gde);

	/* Padding for possible expansion */
	gpointer padding1;
	gpointer padding2;
};

GType      gnome_date_edit_get_type        (void) G_GNUC_CONST;
GtkWidget *gnome_date_edit_new            (time_t the_time,
					   gboolean show_time,
					   gboolean use_24_format);
GtkWidget *gnome_date_edit_new_flags      (time_t the_time,
					   GnomeDateEditFlags flags);

/* Note that everything that can be achieved with gnome_date_edit_new can
 * be achieved with gnome_date_edit_new_flags, so that's why this call 
 * is like the _new_flags call */
void      gnome_date_edit_construct	  (GnomeDateEdit *gde,
					   time_t the_time,
					   GnomeDateEditFlags flags);

void      gnome_date_edit_set_time        (GnomeDateEdit *gde, time_t the_time);
time_t    gnome_date_edit_get_time        (GnomeDateEdit *gde);
void      gnome_date_edit_set_popup_range (GnomeDateEdit *gde, int low_hour, int up_hour);
void      gnome_date_edit_set_flags       (GnomeDateEdit *gde, GnomeDateEditFlags flags);
int       gnome_date_edit_get_flags       (GnomeDateEdit *gde);

time_t    gnome_date_edit_get_initial_time(GnomeDateEdit *gde);

#ifndef GNOME_DISABLE_DEPRECATED
time_t    gnome_date_edit_get_date        (GnomeDateEdit *gde);
#endif /* GNOME_DISABLE_DEPRECATED */

G_END_DECLS

#endif
