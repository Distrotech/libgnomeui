/* gnome-dock-band.h

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

#ifndef GNOME_DOCK_BAND_H
#define GNOME_DOCK_BAND_H

#include <libgnome/gnome-defs.h>

#include "gnome-dock-item.h"

BEGIN_GNOME_DECLS

#define GNOME_DOCK_BAND(obj) \
  GTK_CHECK_CAST (obj, gnome_dock_band_get_type (), GnomeDockBand)

#define GNOME_DOCK_BAND_CLASS(class) \
  GTK_CHECK_CLASS_CAST (class, gnome_dock_band_get_type (), GnomeDockBandClass)

#define GNOME_IS_DOCK_BAND(obj) \
  GTK_CHECK_TYPE (obj, gnome_dock_band_get_type ())

typedef struct _GnomeDockBand GnomeDockBand;
typedef struct _GnomeDockBandClass GnomeDockBandClass;
typedef struct _GnomeDockBandChild GnomeDockBandChild;

struct _GnomeDockBand
{
  GtkContainer container;

  GtkOrientation orientation;

  GList *children;              /* GnomeDockBandChild */
  guint num_children;

  GList *floating_child;        /* GnomeDockBandChild */
  gboolean doing_drag;
};

struct _GnomeDockBandClass
{
  GtkContainerClass parent_class;
};

struct _GnomeDockBandChild
{
  GtkWidget *widget;

  /* Maximum (requested) offset from the previous child.  */
  guint offset;

  /* Actual offset.  */
  guint real_offset;

  guint drag_offset;

  GtkAllocation drag_allocation;

  guint prev_space, foll_space;
  guint drag_prev_space, drag_foll_space;
};

GtkWidget  *gnome_dock_band_new              (void);
guint       gnome_dock_band_get_type         (void);

void        gnome_dock_band_set_orientation  (GnomeDockBand *band,
                                              GtkOrientation orientation);
GtkOrientation gnome_dock_band_get_orientation (GnomeDockBand *band);

gboolean    gnome_dock_band_insert           (GnomeDockBand *band,
                                              GtkWidget *child,
                                              guint offset,
                                              gint position);
gboolean    gnome_dock_band_prepend          (GnomeDockBand *band,
                                              GtkWidget *child,
                                              guint offset);
gboolean    gnome_dock_band_append           (GnomeDockBand *band,
                                              GtkWidget *child,
                                              guint offset);
 
void        gnome_dock_band_set_child_offset (GnomeDockBand *band,
                                              GtkWidget *child,
                                              guint offset);
guint       gnome_dock_band_get_child_offset (GnomeDockBand *band,
                                              GtkWidget *child); 
void        gnome_dock_band_move_child       (GnomeDockBand *band,
                                              GList *old_child,
                                              guint new_num);

guint       gnome_dock_band_get_num_children (GnomeDockBand *band);
 
void        gnome_dock_band_drag_begin       (GnomeDockBand *band,
                                              GnomeDockItem *item);
void        gnome_dock_band_drag_to          (GnomeDockBand *band,
                                              GnomeDockItem *item,
                                              gint x, gint y);
void        gnome_dock_band_drag_end         (GnomeDockBand *band,
                                              GnomeDockItem *item);

END_GNOME_DECLS

#endif
