/* GNOME Unit spinner
 * Copyright (C) 1999 James Henstridge <james@daa.com.au>
 * All rights reserved.
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/
#ifndef GNOME_UNIT_SPINNER_H
#define GNOME_UNIT_SPINNER_H

#include <gtk/gtk.h>

#include <libgnome/gnome-paper.h>

G_BEGIN_DECLS

#define GNOME_TYPE_UNIT_SPINNER            (gnome_unit_spinner_get_type ())
#define GNOME_UNIT_SPINNER(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_UNIT_SPINNER, GnomeUnitSpinner))
#define GNOME_UNIT_SPINNER_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_UNIT_SPINNER, GnomeUnitSpinnerClass))
#define GNOME_IS_UNIT_SPINNER(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_UNIT_SPINNER))
#define GNOME_IS_UNIT_SPINNER_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_UNIT_SPINNER))
#define GNOME_UNIT_SPINNER_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_UNIT_SPINNER, GnomeUnitSpinnerClass))

typedef struct _GnomeUnitSpinner GnomeUnitSpinner;
typedef struct _GnomeUnitSpinnerClass GnomeUnitSpinnerClass;

struct _GnomeUnitSpinner {
  GtkSpinButton parent;

  /*< public >*/
  const GnomeUnit *adj_unit;

  /*< private >*/
  gpointer _priv; /* reserved for a future private pointer */
};

struct _GnomeUnitSpinnerClass {
  GtkSpinButtonClass parent_class;
};

GtkType    gnome_unit_spinner_get_type     (void) G_GNUC_CONST;
GtkWidget *gnome_unit_spinner_new          (GtkAdjustment *adjustment,
					    guint digits,
					    const GnomeUnit *adj_unit);
void       gnome_unit_spinner_construct    (GnomeUnitSpinner *self,
					    GtkAdjustment *adjustment,
					    guint digits,
					    const GnomeUnit *adj_unit);
void       gnome_unit_spinner_set_value    (GnomeUnitSpinner *self,
					    gfloat val,
					    const GnomeUnit *unit);
gfloat     gnome_unit_spinner_get_value    (GnomeUnitSpinner *self,
					    const GnomeUnit *unit);
void       gnome_unit_spinner_change_units (GnomeUnitSpinner *self,
					    const GnomeUnit *unit);

G_END_DECLS

#endif
