/*
 * Copyright (C) 1998, 1999, 2000 Free Software Foundation
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

/* Author: Dietmar Maurer <dm@vlsivie.tuwien.ac.at> */

#include "gtkpixmapmenuitem.h"
#include <gtk/gtkaccellabel.h>
#include <gtk/gtksignal.h>
#include <gtk/gtkmenuitem.h>
#include <gtk/gtkmenu.h>
#include <gtk/gtkcontainer.h>

static void gtk_pixmap_menu_item_class_init    (GtkPixmapMenuItemClass *klass);
static void gtk_pixmap_menu_item_init          (GtkPixmapMenuItem      *menu_item);
static gint gtk_pixmap_menu_item_expose        (GtkWidget              *widget,
					        GdkEventExpose         *event);

/* we must override the following functions */

static void gtk_pixmap_menu_item_map           (GtkWidget        *widget);
static void gtk_pixmap_menu_item_size_allocate (GtkWidget        *widget,
						GtkAllocation    *allocation);
static void gtk_pixmap_menu_item_forall        (GtkContainer    *container,
						gboolean         include_internals,
						GtkCallback      callback,
						gpointer         callback_data);
static void gtk_pixmap_menu_item_size_request  (GtkWidget        *widget,
						GtkRequisition   *requisition);
static void gtk_pixmap_menu_item_remove        (GtkContainer *container,
						GtkWidget    *child);

static void changed_have_pixmap_status         (GtkPixmapMenuItem *menu_item);

static GtkMenuItemClass *parent_class = NULL;

#define BORDER_SPACING  3
#define PMAP_WIDTH 20

GtkType
gtk_pixmap_menu_item_get_type (void)
{
  static GtkType pixmap_menu_item_type = 0;

  if (!pixmap_menu_item_type)
    {
      GtkTypeInfo pixmap_menu_item_info =
      {
        "GtkPixmapMenuItem",
        sizeof (GtkPixmapMenuItem),
        sizeof (GtkPixmapMenuItemClass),
        (GtkClassInitFunc) gtk_pixmap_menu_item_class_init,
        (GtkObjectInitFunc) gtk_pixmap_menu_item_init,
        /* reserved_1 */ NULL,
        /* reserved_2 */ NULL,
        (GtkClassInitFunc) NULL,
      };

      pixmap_menu_item_type = gtk_type_unique (gtk_menu_item_get_type (), 
					       &pixmap_menu_item_info);
    }

  return pixmap_menu_item_type;
}

/**
 * gtk_pixmap_menu_item_new
 *
 * Creates a new pixmap menu item. Use gtk_pixmap_menu_item_set_pixmap() 
 * to set the pixmap wich is displayed at the left side.
 *
 * Returns:
 * &GtkWidget pointer to new menu item
 **/

GtkWidget*
gtk_pixmap_menu_item_new (void)
{
  return GTK_WIDGET (gtk_type_new (gtk_pixmap_menu_item_get_type ()));
}

static void
gtk_pixmap_menu_item_class_init (GtkPixmapMenuItemClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkMenuItemClass *menu_item_class;
  GtkContainerClass *container_class;

  object_class = (GtkObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;
  menu_item_class = (GtkMenuItemClass*) klass;
  container_class = (GtkContainerClass*) klass;

  parent_class = gtk_type_class (gtk_menu_item_get_type ());

  widget_class->expose_event = gtk_pixmap_menu_item_expose;
  widget_class->map = gtk_pixmap_menu_item_map;
  widget_class->size_allocate = gtk_pixmap_menu_item_size_allocate;
  widget_class->size_request = gtk_pixmap_menu_item_size_request;

  container_class->forall = gtk_pixmap_menu_item_forall;
  container_class->remove = gtk_pixmap_menu_item_remove;

#if 0 /* FIXME */
  klass->orig_toggle_size = menu_item_class->toggle_size;
#endif
  klass->have_pixmap_count = 0;
}

static void
gtk_pixmap_menu_item_init (GtkPixmapMenuItem *menu_item)
{
  GtkMenuItem *mi;

  mi = GTK_MENU_ITEM (menu_item);

  menu_item->pixmap = NULL;
}

static gint
gtk_pixmap_menu_item_expose (GtkWidget      *widget,
			     GdkEventExpose *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GTK_IS_PIXMAP_MENU_ITEM (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (GTK_WIDGET_CLASS (parent_class)->expose_event)
    (* GTK_WIDGET_CLASS (parent_class)->expose_event) (widget, event);

  if (GTK_WIDGET_DRAWABLE (widget) && 
      GTK_PIXMAP_MENU_ITEM(widget)->pixmap) {
    gtk_widget_draw(GTK_WIDGET(GTK_PIXMAP_MENU_ITEM(widget)->pixmap),NULL);
  }

  return FALSE;
}

/**
 * gtk_pixmap_menu_item_set_pixmap
 * @menu_item: Pointer to the pixmap menu item
 * @pixmap: Pointer to a pixmap widget
 *
 * Set the pixmap of the menu item.
 *
 **/

void
gtk_pixmap_menu_item_set_pixmap (GtkPixmapMenuItem *menu_item,
				 GtkWidget         *pixmap)
{
  g_return_if_fail (menu_item != NULL);
  g_return_if_fail (pixmap != NULL);
  g_return_if_fail (GTK_IS_PIXMAP_MENU_ITEM (menu_item));
  g_return_if_fail (GTK_IS_WIDGET (pixmap));
  g_return_if_fail (menu_item->pixmap == NULL);

  gtk_widget_set_parent (pixmap, GTK_WIDGET (menu_item));
  menu_item->pixmap = pixmap;

  if (GTK_WIDGET_REALIZED (pixmap->parent) &&
      !GTK_WIDGET_REALIZED (pixmap))
    gtk_widget_realize (pixmap);
  
  if (GTK_WIDGET_VISIBLE (pixmap->parent)) {      
    if (GTK_WIDGET_MAPPED (pixmap->parent) &&
	GTK_WIDGET_VISIBLE(pixmap) &&
        !GTK_WIDGET_MAPPED (pixmap))
      gtk_widget_map (pixmap);
  }

  changed_have_pixmap_status(menu_item);
  
  if (GTK_WIDGET_VISIBLE (pixmap) && GTK_WIDGET_VISIBLE (menu_item))
    gtk_widget_queue_resize (pixmap);
}

static void
gtk_pixmap_menu_item_map (GtkWidget *widget)
{
  GtkPixmapMenuItem *menu_item;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_PIXMAP_MENU_ITEM (widget));

  menu_item = GTK_PIXMAP_MENU_ITEM(widget);

  GTK_WIDGET_CLASS(parent_class)->map(widget);

  if (menu_item->pixmap &&
      GTK_WIDGET_VISIBLE (menu_item->pixmap) &&
      !GTK_WIDGET_MAPPED (menu_item->pixmap))
    gtk_widget_map (menu_item->pixmap);
}

static void
gtk_pixmap_menu_item_size_allocate (GtkWidget        *widget,
				    GtkAllocation    *allocation)
{
  GtkPixmapMenuItem *pmenu_item;

  pmenu_item = GTK_PIXMAP_MENU_ITEM(widget);

  if (pmenu_item->pixmap && GTK_WIDGET_VISIBLE(pmenu_item))
    {
      GtkAllocation child_allocation;
      int border_width;

      border_width = GTK_CONTAINER (widget)->border_width;

      child_allocation.width = pmenu_item->pixmap->requisition.width;
      child_allocation.height = pmenu_item->pixmap->requisition.height;
      child_allocation.x = border_width + BORDER_SPACING;
      child_allocation.y = (border_width + BORDER_SPACING
			    + (((allocation->height - child_allocation.height) - child_allocation.x)
			       / 2)); /* center pixmaps vertically */
      gtk_widget_size_allocate (pmenu_item->pixmap, &child_allocation);
    }

  if (GTK_WIDGET_CLASS (parent_class)->size_allocate)
    GTK_WIDGET_CLASS(parent_class)->size_allocate (widget, allocation);
}

static void
gtk_pixmap_menu_item_forall (GtkContainer    *container,
			     gboolean         include_internals,
			     GtkCallback      callback,
			     gpointer         callback_data)
{
  GtkPixmapMenuItem *menu_item;

  g_return_if_fail (container != NULL);
  g_return_if_fail (GTK_IS_PIXMAP_MENU_ITEM (container));
  g_return_if_fail (callback != NULL);

  menu_item = GTK_PIXMAP_MENU_ITEM (container);

  if (menu_item->pixmap)
    (* callback) (menu_item->pixmap, callback_data);

  GTK_CONTAINER_CLASS(parent_class)->forall(container,include_internals,
					    callback,callback_data);
}

static void
gtk_pixmap_menu_item_size_request (GtkWidget      *widget,
				   GtkRequisition *requisition)
{
  GtkPixmapMenuItem *menu_item;
  GtkRequisition req = {0, 0};

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GTK_IS_MENU_ITEM (widget));
  g_return_if_fail (requisition != NULL);

  GTK_WIDGET_CLASS(parent_class)->size_request(widget,requisition);

  menu_item = GTK_PIXMAP_MENU_ITEM (widget);
  
  if (menu_item->pixmap)
    gtk_widget_size_request(menu_item->pixmap, &req);

  requisition->height = MAX(req.height + GTK_CONTAINER(widget)->border_width + BORDER_SPACING, requisition->height);
  requisition->width += (req.width + GTK_CONTAINER(widget)->border_width + BORDER_SPACING);
}

static void
gtk_pixmap_menu_item_remove (GtkContainer *container,
			     GtkWidget    *child)
{
  GtkBin *bin;
  gboolean widget_was_visible;

  g_return_if_fail (container != NULL);
  g_return_if_fail (GTK_IS_PIXMAP_MENU_ITEM (container));
  g_return_if_fail (child != NULL);
  g_return_if_fail (GTK_IS_WIDGET (child));

  bin = GTK_BIN (container);
  g_return_if_fail ((bin->child == child || 
		     (GTK_PIXMAP_MENU_ITEM(container)->pixmap == child)));

  widget_was_visible = GTK_WIDGET_VISIBLE (child);
  
  gtk_widget_unparent (child);
  if (bin->child == child)
    bin->child = NULL; 
  else {
    GTK_PIXMAP_MENU_ITEM(container)->pixmap = NULL;
    changed_have_pixmap_status(GTK_PIXMAP_MENU_ITEM(container));
  }
    
  if (widget_was_visible)
    gtk_widget_queue_resize (GTK_WIDGET (container));
}


/* important to only call this if there was actually a _change_ in pixmap == NULL */
static void
changed_have_pixmap_status (GtkPixmapMenuItem *menu_item)
{
#if 0 /* FIXME */
  if (menu_item->pixmap != NULL) {
    GTK_PIXMAP_MENU_ITEM_GET_CLASS(menu_item)->have_pixmap_count += 1;

    if (GTK_PIXMAP_MENU_ITEM_GET_CLASS(menu_item)->have_pixmap_count == 1) {
      /* Install pixmap toggle size */
      GTK_MENU_ITEM_GET_CLASS(menu_item)->toggle_size = MAX(GTK_PIXMAP_MENU_ITEM_GET_CLASS(menu_item)->orig_toggle_size, PMAP_WIDTH);
    }
  } else {
    GTK_PIXMAP_MENU_ITEM_GET_CLASS(menu_item)->have_pixmap_count -= 1;

    if (GTK_PIXMAP_MENU_ITEM_GET_CLASS(menu_item)->have_pixmap_count == 0) {
      /* Install normal toggle size */
      GTK_MENU_ITEM_GET_CLASS(menu_item)->toggle_size = GTK_PIXMAP_MENU_ITEM_GET_CLASS(menu_item)->orig_toggle_size;    
    }
  }
#endif

  /* Note that we actually need to do this for _all_ GtkPixmapMenuItem
     whenever the klass->toggle_size changes; but by doing it anytime
     this function is called, we get the same effect, just because of
     how the preferences option to show pixmaps works. Bogus, broken.
  */
  if (GTK_WIDGET_VISIBLE(GTK_WIDGET(menu_item))) 
    gtk_widget_queue_resize(GTK_WIDGET(menu_item));
}

