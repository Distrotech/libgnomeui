/* WARNING ____ IMMATURE API ____ liable to change */

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

#ifndef _GNOME_DOCK_H
#define _GNOME_DOCK_H

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

#define GNOME_TYPE_DOCK            (gnome_dock_get_type ())
#define GNOME_DOCK(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_DOCK, GnomeDock))
#define GNOME_DOCK_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_DOCK, GnomeDockClass))
#define GNOME_IS_DOCK(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_DOCK))
#define GNOME_IS_DOCK_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_DOCK))

typedef enum
{
  GNOME_DOCK_TOP,
  GNOME_DOCK_RIGHT,
  GNOME_DOCK_BOTTOM,
  GNOME_DOCK_LEFT,
  GNOME_DOCK_FLOATING
} GnomeDockPlacement;

typedef struct _GnomeDock GnomeDock;
typedef struct _GnomeDockClass GnomeDockClass;

#include "gnome-dock-band.h"
#include "gnome-dock-layout.h"

struct _GnomeDock
{
  GtkContainer container;

  GtkWidget *client_area;

  /* GnomeDockBands associated with this dock.  */
  GList *top_bands;
  GList *bottom_bands;
  GList *right_bands;
  GList *left_bands;

  /* Children that are currently not docked.  */
  GList *floating_children;     /* GtkWidget */

  /* Client rectangle before drag.  */
  GtkAllocation client_rect;

  gboolean floating_items_allowed : 1;
};

struct _GnomeDockClass
{
  GtkContainerClass parent_class;

  void (* layout_changed) (GnomeDock *dock);
};

GtkWidget     *gnome_dock_new               (void);
guint          gnome_dock_get_type          (void);

void           gnome_dock_allow_floating_items
                                            (GnomeDock *dock,
                                             gboolean enable);
                                            
void           gnome_dock_add_item          (GnomeDock             *dock,
                                             GnomeDockItem         *item,
                                             GnomeDockPlacement  placement,
                                             guint                  band_num,
                                             gint                   position,
                                             guint                  offset,
                                             gboolean               in_new_band);

void           gnome_dock_add_floating_item (GnomeDock *dock,
                                             GnomeDockItem *widget,
                                             gint x, gint y,
                                             GtkOrientation orientation);
          
void           gnome_dock_set_client_area   (GnomeDock             *dock,
                                             GtkWidget             *widget);

GtkWidget     *gnome_dock_get_client_area   (GnomeDock             *dock);
  
GnomeDockItem *gnome_dock_get_item_by_name  (GnomeDock *dock,
                                             const gchar *name,
                                             GnomeDockPlacement *placement_return,
                                             guint *num_band_return,
                                             guint *band_position_return,
                                             guint *offset_return);
 
GnomeDockLayout *gnome_dock_get_layout      (GnomeDock *dock);

gboolean       gnome_dock_add_from_layout   (GnomeDock *dock,
                                             GnomeDockLayout *layout);

END_GNOME_DECLS

#endif
