/* gnome-dock.h

   Copyright (C) 1998 Free Software Foundation

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Ettore Perazzoli <ettore@comm2000.it>
*/

#ifndef GNOMEDOCK_H
#define GNOMEDOCK_H

#include <libgnome/gnome-defs.h>

#include "gnome-dock-band.h"

BEGIN_GNOME_DECLS

#define GNOME_DOCK(obj) \
  GTK_CHECK_CAST (obj, gnome_dock_get_type (), GnomeDock)
#define GNOME_DOCK_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, gnome_dock_get_type (), GnomeDockClass)
#define GNOME_IS_DOCK(obj) \
  GTK_CHECK_TYPE (obj, gnome_dock_get_type ())

typedef enum
{
  GNOME_DOCK_POS_LEFT,
  GNOME_DOCK_POS_RIGHT,
  GNOME_DOCK_POS_TOP,
  GNOME_DOCK_POS_BOTTOM,
  GNOME_DOCK_POS_DETACHED
} GnomeDockPositionType;

typedef struct _GnomeDock GnomeDock;
typedef struct _GnomeDockClass GnomeDockClass;
typedef struct _GnomeDockChild GnomeDockChild;

struct _GnomeDockChild
{
  GnomeDockBand *band;

  GtkAllocation drag_allocation;

  gboolean new_for_drag;
};

struct _GnomeDock
{
  GtkContainer container;

  GtkWidget *client_area;

  /* GnomeDockBands associated to this dock.  */
  GList *top_bands;             /* GnomeDockChild */
  GList *bottom_bands;          /* GnomeDockChild */
  GList *right_bands;           /* GnomeDockChild */
  GList *left_bands;            /* GnomeDockChild */

  /* Children that are currently not docked.  */
  GList *undocked_children;     /* GnomeDockChild */

  /* Client rectangle before drag.  */
  GtkAllocation client_rect;
};

struct _GnomeDockClass
{
  GtkContainerClass parent_class;
};

GtkWidget *gnome_dock_new             (void);
guint      gnome_dock_get_type        (void);
void       gnome_dock_prepend_band    (GnomeDock *dock, GtkWidget *band);
void       gnome_dock_add_item        (GnomeDock *dock, GtkWidget *item,
                                       GnomeDockPositionType edge,
                                       guint band_num,
                                       guint offset,
                                       gint position);
void       gnome_dock_set_client_area (GnomeDock *dock, GtkWidget *widget);

END_GNOME_DECLS

#endif
