/* gnome-dock.c

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

#include <gtk/gtk.h>

#include "gnome-dock.h"
#include "gnome-dock-band.h"
#include "gnome-dock-item.h"



#define noGNOME_DOCK_DEBUG

/* FIXME: To be removed.  */
#if defined GNOME_DOCK_DEBUG && defined __GNUC__
#define DEBUG(x)                                        \
  do                                                    \
    {                                                   \
      printf ("%s.%d: ", __FUNCTION__, __LINE__);       \
      printf x;                                         \
      putchar ('\n');                                   \
    }                                                   \
  while (0)
#else
#define DEBUG(x)
#endif



enum {
  LAYOUT_CHANGED,
  LAST_SIGNAL
};



static void     gnome_dock_class_init          (GnomeDockClass *class);
static void     gnome_dock_init                (GnomeDock *app);
static void     gnome_dock_size_request        (GtkWidget *widget,
                                                GtkRequisition *requisition);
static void     gnome_dock_size_allocate       (GtkWidget *widget,
                                                GtkAllocation *allocation);
static void     gnome_dock_map                 (GtkWidget *widget);
static void     gnome_dock_draw                (GtkWidget *widget,
                                                GdkRectangle *area);
static gint     gnome_dock_expose              (GtkWidget *widget,
                                                GdkEventExpose *event);
static void     gnome_dock_add                 (GtkContainer *container,
                                                GtkWidget *child);
static void     gnome_dock_remove              (GtkContainer *container,
                                                GtkWidget *widget);
static void     gnome_dock_forall              (GtkContainer *container,
                                                gboolean include_internals,
                                                GtkCallback callback,
                                                gpointer callback_data);
          
static void     size_request_v                 (GList *list,
                                                GtkRequisition *requisition);
static void     size_request_h                 (GList *list,
                                                GtkRequisition *requisition);
static gint     size_allocate_v                (GList *list,
                                                gint start_x, gint start_y,
                                                guint width, gint direction);
static gint     size_allocate_h                (GList *list,
                                                gint start_x, gint start_y,
                                                guint width, gint direction);
static void     map_widget                     (GtkWidget *w);
static void     map_widget_foreach             (gpointer data,
                                                gpointer user_data);
static void     map_band_list                  (GList *list);
static void     draw_widget                    (GtkWidget *widget,
                                                GdkRectangle *area);
static void     draw_band_list                 (GList *list,
                                                GdkRectangle *area);
static void     expose_widget                  (GtkWidget *widget,
                                                GdkEventExpose *event);
static void     expose_band_list               (GList *list,
                                                GdkEventExpose *event);
static gboolean remove_from_band_list          (GList **list,
                                                GnomeDockBand *child);
static void     forall_helper                  (GList *list,
                                                GtkCallback callback,
                                                gpointer callback_data);
      
static void     drag_begin_foreach_func        (gpointer data,
                                                gpointer user_data);
static void     drag_begin                     (GtkWidget *widget,
                                                gpointer data);
static void     drag_end_bands                 (GList **list,
                                                GnomeDockItem *item);
static void     drag_end                       (GtkWidget *widget,
                                                gpointer data);
static gboolean drag_new                       (GnomeDock *dock,
                                                GnomeDockItem *item,
                                                GList **area,
                                                GList *where,
                                                gint x, gint y,
                                                gboolean is_vertical);
static gboolean drag_to                        (GnomeDock *dock,
                                                GnomeDockItem *item,
                                                GList *where,
                                                gint x, gint y,
                                                gboolean is_vertical);
static gboolean drag_floating                  (GnomeDock *dock,
                                                GnomeDockItem *item,
                                                gint x, gint y,
                                                gint rel_x, gint rel_y);
static gboolean drag_check                     (GnomeDock *dock,
                                                GnomeDockItem *item,
                                                GList **area,
                                                gint x, gint y,
                                                gboolean is_vertical);
static void     drag_motion                    (GtkWidget *widget,
                                                gint x, gint y,
                                                gpointer data);
      
static GnomeDockItem *get_docked_item_by_name  (GnomeDock *dock,
                                                const gchar *name,
                                                GnomeDockPlacement *placement_return,
                                                guint *num_band_return,
                                                guint *band_position_return,
                                                guint *offset_return);
static GnomeDockItem *get_floating_item_by_name (GnomeDock *dock,
                                                 const gchar *name);

static void           connect_drag_signals      (GnomeDock *dock,
                                                 GtkWidget *item);


static GtkWidgetClass *parent_class = NULL;

static guint dock_signals[LAST_SIGNAL] = { 0 };



static void
gnome_dock_class_init (GnomeDockClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;
  container_class = (GtkContainerClass *) class;

  parent_class = gtk_type_class (gtk_container_get_type ());

  widget_class->size_request = gnome_dock_size_request;
  widget_class->size_allocate = gnome_dock_size_allocate;
  widget_class->map = gnome_dock_map;
  widget_class->draw = gnome_dock_draw;
  widget_class->expose_event = gnome_dock_expose;

  container_class->add = gnome_dock_add;
  container_class->remove = gnome_dock_remove;
  container_class->forall = gnome_dock_forall;

  dock_signals[LAYOUT_CHANGED] =
    gtk_signal_new ("layout_changed",
                    GTK_RUN_LAST,
                    object_class->type,
                    GTK_SIGNAL_OFFSET (GnomeDockClass, layout_changed),
                    gtk_marshal_NONE__NONE,
                    GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, dock_signals, LAST_SIGNAL);
}

static void
gnome_dock_init (GnomeDock *dock)
{
  GTK_WIDGET_SET_FLAGS (GTK_WIDGET (dock), GTK_NO_WINDOW);

  dock->client_area = NULL;

  dock->top_bands = NULL;
  dock->bottom_bands = NULL;
  dock->right_bands = NULL;
  dock->left_bands = NULL;

  dock->floating_children = NULL;
}



static void
size_request_v (GList *list, GtkRequisition *requisition)
{
  for (; list != NULL; list = list->next)
    {
      GnomeDockChild *child;
      GtkWidget *w;

      child = list->data;
      w = GTK_WIDGET (child->band);
      gtk_widget_size_request (w, &w->requisition);
      requisition->width += w->requisition.width;
      requisition->height = MAX (requisition->height, w->requisition.height);
    }
}

static void
size_request_h (GList *list, GtkRequisition *requisition)
{
  for (list = list; list != NULL; list = list->next)
    {
      GnomeDockChild *child;
      GtkWidget *w;

      child = (GnomeDockChild *) list->data;
      w = GTK_WIDGET (child->band);
      gtk_widget_size_request (w, &w->requisition);
      requisition->height += w->requisition.height;
      requisition->width = MAX (requisition->width, w->requisition.width);
    }
}

static void
gnome_dock_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  GnomeDock *dock;
  GtkContainer *container;

  dock = GNOME_DOCK (widget);

  if (dock->client_area != NULL && GTK_WIDGET_VISIBLE (dock->client_area))
    {
      gtk_widget_size_request (dock->client_area,
                               &dock->client_area->requisition);
      requisition->width = dock->client_area->requisition.width;
      requisition->height = dock->client_area->requisition.height;
    }
  else
    {
      requisition->width = 0;
      requisition->height = 0;
    }

  size_request_v (dock->left_bands, requisition);
  size_request_v (dock->right_bands, requisition);
  size_request_h (dock->top_bands, requisition);
  size_request_h (dock->bottom_bands, requisition);
}



static gint
size_allocate_h (GList *list, gint start_x, gint start_y, guint width,
                 gint direction)
{
  GtkAllocation allocation;

  allocation.x = start_x;
  allocation.y = start_y;
  allocation.width = width;

  if (direction < 0)
    list = g_list_last (list);
  while (list != NULL)
    {
      GnomeDockChild *child;
      GtkWidget *w;

      child = (GnomeDockChild *) list->data;
      w = GTK_WIDGET (child->band);
      allocation.height = w->requisition.height;

      if (direction > 0)
        {
          gtk_widget_size_allocate (w, &allocation);
          allocation.y += allocation.height;
          list = list->next;
        }
      else
        {
          allocation.y -= allocation.height;
          gtk_widget_size_allocate (w, &allocation);
          list = list->prev;
        }
    }

  return allocation.y;
}

static gint
size_allocate_v (GList *list, gint start_x, gint start_y, guint height,
                 gint direction)
{
  GtkAllocation allocation;

  allocation.x = start_x;
  allocation.y = start_y;
  allocation.height = height;

  if (direction < 0)
    list = g_list_last (list);

  while (list != NULL)
    {
      GnomeDockChild *child;
      GtkWidget *w;

      child = (GnomeDockChild *) list->data;
      w = GTK_WIDGET (child->band);
      allocation.width = w->requisition.width;

      if (direction > 0)
        {
          gtk_widget_size_allocate (w, &allocation);
          allocation.x += allocation.width;
          list = list->next;
        }
      else
        {
          allocation.x -= allocation.width;
          gtk_widget_size_allocate (w, &allocation);
          list = list->prev;
        }
    }

  return allocation.x;
}

static void
gnome_dock_size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  GnomeDock *dock;
  gint top_bands_y, bottom_bands_y;
  gint left_bands_x, right_bands_x;
  GtkAllocation child_allocation;

  dock = GNOME_DOCK (widget);

  widget->allocation = *allocation;

  top_bands_y = size_allocate_h (dock->top_bands,
                                 allocation->x,
                                 allocation->y,
                                 allocation->width,
                                 +1);

  bottom_bands_y = size_allocate_h (dock->bottom_bands,
                                    allocation->x,
                                    allocation->y + allocation->height,
                                    allocation->width,
                                    -1);

  child_allocation.height = MAX (bottom_bands_y - top_bands_y, 0);

  left_bands_x = size_allocate_v (dock->left_bands,
                                  allocation->x,
                                  top_bands_y,
                                  child_allocation.height,
                                  +1);

  right_bands_x = size_allocate_v (dock->right_bands,
                                   allocation->x + allocation->width,
                                   top_bands_y,
                                   child_allocation.height,
                                   -1);

  child_allocation.width = MAX (right_bands_x - left_bands_x, 0);

  child_allocation.x = left_bands_x;
  child_allocation.y = top_bands_y;

  if (dock->client_area != NULL && GTK_WIDGET_VISIBLE (dock->client_area))
    gtk_widget_size_allocate (dock->client_area, &child_allocation);
}



static void
map_widget (GtkWidget *w)
{
  if (w != NULL && GTK_WIDGET_VISIBLE (w) && ! GTK_WIDGET_MAPPED (w))
    gtk_widget_map (w);
}

static void
map_widget_foreach (gpointer data,
                    gpointer user_data)
{
  map_widget (GTK_WIDGET (data));
}

static void
map_band_list (GList *list)
{
  while (list != NULL)
    {
      GnomeDockChild *child;
      GtkWidget *w;

      child = (GnomeDockChild *) list->data;
      w = GTK_WIDGET (child->band);
      map_widget (w);

      list = list->next;
    }
}

static void
gnome_dock_map (GtkWidget *widget)
{
  GnomeDock *dock;

  if (GTK_WIDGET_CLASS (parent_class)->map != NULL)
    (* GTK_WIDGET_CLASS (parent_class)->map) (widget);

  dock = GNOME_DOCK (widget);

  map_widget (dock->client_area);

  map_band_list (dock->top_bands);
  map_band_list (dock->bottom_bands);
  map_band_list (dock->left_bands);
  map_band_list (dock->right_bands);

  g_list_foreach (dock->floating_children, map_widget_foreach, NULL);
}



static void
draw_widget (GtkWidget *widget, GdkRectangle *area)
{
  GdkRectangle d_area;

  if (widget != NULL && gtk_widget_intersect (widget, area, &d_area))
    gtk_widget_draw (widget, &d_area);
}

static void
draw_band_list (GList *list, GdkRectangle *area)
{
  while (list != NULL)
    {
      GnomeDockChild *child;
      GtkWidget *w;

      child = (GnomeDockChild *) list->data;
      w = GTK_WIDGET (child->band);
      draw_widget (w, area);

      list = list->next;
    }
}

static void
gnome_dock_draw (GtkWidget *widget, GdkRectangle *area)
{
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GnomeDock *dock;

      dock = GNOME_DOCK (widget);

      draw_widget (dock->client_area, area);

      draw_band_list (dock->top_bands, area);
      draw_band_list (dock->bottom_bands, area);
      draw_band_list (dock->left_bands, area);
      draw_band_list (dock->right_bands, area);
    }
}



static void
expose_widget (GtkWidget *widget, GdkEventExpose *event)
{
  if (widget != NULL && GTK_WIDGET_NO_WINDOW (widget))
    {
      GdkEventExpose child_event;

      child_event = *event;
      if (gtk_widget_intersect (widget, &event->area, &child_event.area))
        gtk_widget_event (widget, (GdkEvent *) &child_event);
    }
}

static void
expose_band_list (GList *list, GdkEventExpose *event)
{
  while (list != NULL)
    {
      GnomeDockChild *child;

      child = (GnomeDockChild *) list->data;
      expose_widget (GTK_WIDGET (child->band), event);

      list = list->next;
    }
}

static gint
gnome_dock_expose (GtkWidget *widget, GdkEventExpose *event)
{
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GnomeDock *dock;

      dock = GNOME_DOCK (widget);

      expose_widget (dock->client_area, event);

      expose_band_list (dock->top_bands, event);
      expose_band_list (dock->bottom_bands, event);
      expose_band_list (dock->left_bands, event);
      expose_band_list (dock->right_bands, event);
    }

  return FALSE;
}



/* GtkContainer methods.  */

static void
gnome_dock_add (GtkContainer *container, GtkWidget *child)
{
  GnomeDock *dock;

  dock = GNOME_DOCK (container);
  gnome_dock_add_item (dock, GNOME_DOCK_ITEM(child), GNOME_DOCK_TOP, 0, 0, 0, TRUE);
}

static gboolean
remove_from_band_list (GList **list, GnomeDockBand *child)
{
  GList *lp;

  for (lp = *list; lp != NULL; lp = lp->next)
    {
      GnomeDockChild *c;

      c = (GnomeDockChild *) lp->data;
      if (c->band == child)
        {
          gtk_widget_unparent (GTK_WIDGET (child));
          *list = g_list_remove_link (*list, lp);
          g_free (c);
          g_list_free (lp);

          return TRUE;
        }
    }

  return FALSE;
}

static void
gnome_dock_remove (GtkContainer *container, GtkWidget *widget)
{
  GnomeDock *dock;

  dock = GNOME_DOCK (container);

  if (dock->client_area == widget)
    {
      gtk_widget_unparent (widget);
      dock->client_area = NULL;
      gtk_widget_queue_resize (GTK_WIDGET (dock));
    }
  else
    {
      /* Check if it's a floating child.  */
      {
        GList *lp;

        lp = dock->floating_children;
        while (lp != NULL)
          {
            GtkWidget *w;

            w = lp->data;
            if (w == widget)
              {
                gtk_widget_unparent (w);
                dock->floating_children
                  = g_list_remove_link (dock->floating_children, lp);
                g_list_free (lp);
                return;
              }

            lp = lp->next;
          }
      }

      /* Then it must be one of the bands.  */
      {
        GnomeDockBand *band;

        g_return_if_fail (GNOME_IS_DOCK_BAND (widget));

        band = GNOME_DOCK_BAND (widget);
        if (remove_from_band_list (&dock->top_bands, band)
            || remove_from_band_list (&dock->bottom_bands, band)
            || remove_from_band_list (&dock->left_bands, band)
            || remove_from_band_list (&dock->right_bands, band))
          {
            gtk_widget_queue_resize (GTK_WIDGET (dock));
            return;
          }
      }
    }
}

static void
forall_helper (GList *list,
               GtkCallback callback,
               gpointer callback_data)
{
  while (list != NULL)
    {
      GnomeDockChild *c;

      c = list->data;
      list = list->next;
      (* callback) (GTK_WIDGET (c->band), callback_data);
    }
}

static void
gnome_dock_forall (GtkContainer *container,
                   gboolean include_internals,
                   GtkCallback callback,
                   gpointer callback_data)
{
  GnomeDock *dock;
  GList *lp;

  g_return_if_fail (container != NULL);
  g_return_if_fail (GNOME_IS_DOCK (container));
  g_return_if_fail (callback != NULL);

  dock = GNOME_DOCK (container);

  forall_helper (dock->top_bands, callback, callback_data);
  forall_helper (dock->bottom_bands, callback, callback_data);
  forall_helper (dock->left_bands, callback, callback_data);
  forall_helper (dock->right_bands, callback, callback_data);

  lp = dock->floating_children;
  while (lp != NULL)
    {
      GtkWidget *w;

      w = lp->data;
      lp = lp->next;
      (* callback) (w, callback_data);
    }

  if (dock->client_area != NULL)
    (* callback) (dock->client_area, callback_data);
}



/* When an item is being dragged, there can be 3 situations:

   (I)   A new band is created and the item is docked to it.

   (II)  The item is docked to an existing band.

   (III) The item must be floating, so it has to be detached if
         currently not floating, and moved around in its own window.  */

/* Case (I): Dock `item' into a new band next to `where' in the
   docking area `area'.  If `where' is NULL, the band becomes the
   first one in `area'.  */
static gboolean
drag_new (GnomeDock *dock,
          GnomeDockItem *item,
          GList **area,
          GList *where,
          gint x, gint y,
          gboolean is_vertical)
{
  GnomeDockChild *new;
  GList *next;

  DEBUG (("entering function"));

  new = NULL;

  /* We need a new band next to `where', but we try to re-use the band
     next to it if either it contains no children, or it only contains
     `item'.  */

  next = NULL;
  if (where == NULL && area != NULL)
    next = *area;
  else
    next = where->next;
  if (next != NULL)
    {
      GnomeDockChild *c;
      guint num_children;

      c = (GnomeDockChild *) next->data;

      num_children = gnome_dock_band_get_num_children (c->band);

      if (num_children == 0
          || (num_children == 1
              && GTK_WIDGET (c->band) == GTK_WIDGET (item)->parent))
        new = (GnomeDockChild *) next->data;
    }

  /* Create the new band and make it our child if we cannot re-use an
     existing one.  */
  if (new == NULL)
    {
      new = g_new (GnomeDockChild, 1);

      new->band = GNOME_DOCK_BAND (gnome_dock_band_new ());
      gnome_dock_band_set_orientation (new->band,
                                       (is_vertical
                                        ? GTK_ORIENTATION_VERTICAL
                                        : GTK_ORIENTATION_HORIZONTAL));

      /* This is mostly to remember that `drag_allocation' for this
         child is bogus, as it was not previously allocated.  */
      new->new_for_drag = TRUE;

      if (where == NULL)
        *area = where = g_list_prepend (*area, new);
      else if (where->next == NULL)
        g_list_append (where, new);
      else
        g_list_prepend (where->next, new);

      gtk_widget_set_parent (GTK_WIDGET (new->band), GTK_WIDGET (dock));
      gtk_widget_queue_resize (GTK_WIDGET (new->band));
      gtk_widget_show (GTK_WIDGET (new->band));
    }

  /* Move the item to the new band.  (This is a no-op if we are using
     `where->next' and it already contains `item'.)  */
  gnome_dock_item_attach (item, GTK_WIDGET (new->band), x, y);

  /* Prepare the band for dragging of `item'.  */
  gnome_dock_band_drag_begin (new->band, item);

  /* Reparenting will remove the grab, so we need to redo it.  */
  gnome_dock_item_grab_pointer (item);

  /* Set the offset of `item' in the band.  */
  if (is_vertical)
    gnome_dock_band_set_child_offset (new->band, GTK_WIDGET (item),
                                      y - dock->client_area->allocation.y);
  else
    gnome_dock_band_set_child_offset (new->band, GTK_WIDGET (item),
                                      x - GTK_WIDGET (dock)->allocation.x);

  return TRUE;
}

/* Case (II): Drag into an existing band.  */
static gboolean
drag_to (GnomeDock *dock,
         GnomeDockItem *item,
         GList *where,
         gint x, gint y,
         gboolean is_vertical)
{
  GnomeDockChild *child;

  DEBUG (("x %d y %d", x, y));
  child = (GnomeDockChild *) where->data;

  return gnome_dock_band_drag_to (child->band, item, x, y);
}

/* Case (III): Move a floating (i.e. floating) item.  */
static gboolean
drag_floating (GnomeDock *dock,
               GnomeDockItem *item,
               gint x,
               gint y,
               gint rel_x,
               gint rel_y)
{
  GtkWidget *item_widget, *dock_widget;

  item_widget = GTK_WIDGET (item);
  dock_widget = GTK_WIDGET (dock);

  if (item_widget->parent != dock_widget)
    {
      GtkAllocation *dock_allocation;
      GtkAllocation *client_allocation;

      /* The item is currently not floating (so it is not our child).
         Make it so if we are outside the docking areas.  */

      dock_allocation = &dock_widget->allocation;
      client_allocation = &dock->client_area->allocation;
      if (rel_x < dock_allocation->x
          || rel_x >= dock_allocation->x + dock_allocation->width
          || rel_y < dock_allocation->y
          || rel_y >= dock_allocation->y + dock_allocation->height
          || (rel_x >= client_allocation->x
              && rel_x < client_allocation->x + client_allocation->width
              && rel_y >= client_allocation->y
              && rel_y < client_allocation->y + client_allocation->height))
        {
          GnomeDockChild *new;

          gtk_widget_ref (item_widget);

          gnome_dock_item_detach (item, x, y);
          gnome_dock_item_grab_pointer (item);

          gtk_container_remove (GTK_CONTAINER (item_widget->parent),
                                item_widget);
          gtk_widget_set_parent (item_widget, dock_widget);
          dock->floating_children = g_list_prepend (dock->floating_children,
                                                    item);

          gtk_widget_unref (item_widget);
        }
    }
  else
    {
      /* The item is already floating; all we have to do is move it to
         the current dragging position.  */
      gnome_dock_item_drag_floating (item, x, y);
    }

  return TRUE;
}



/* Check if `item' can be docked to any of the DockBands of the dock
   area `area'.  If so, dock it and return TRUE; otherwise, return
   FALSE.  */
static gboolean
drag_check (GnomeDock *dock,
            GnomeDockItem *item,
            GList **area,
            gint x, gint y,
            gboolean is_vertical)
{
  GList *lp;
  GnomeDockChild *child;
  GtkAllocation *alloc;

  for (lp = *area; lp != NULL; lp = lp->next)
    {
      child = (GnomeDockChild *) lp->data;

      if (! child->new_for_drag)
        {
          alloc = &child->drag_allocation;

          if (x >= alloc->x && x < alloc->x + alloc->width
              && y >= alloc->y && y < alloc->y + alloc->height)
            {
              if (is_vertical)
                {
                  if (x > alloc->x + alloc->width / 2)
                    return drag_new (dock, item, area, lp, x, y, TRUE);
                  else
                    return drag_to (dock, item, lp, x, y, TRUE);
                }
              else
                {
                  if (y > alloc->y + alloc->height / 2)
                    return drag_new (dock, item, area, lp, x, y, FALSE);
                  else
                    return drag_to (dock, item, lp, x, y, FALSE);
                }
            }
        }
    }

  return FALSE;
}



/* "drag_begin" signal handling.  */

static void
drag_begin_foreach_func (gpointer data, gpointer user_data)
{
  GnomeDockChild *child;
  GtkWidget *widget;

  child = (GnomeDockChild *) data;
  widget = GTK_WIDGET (child->band);

  /* Remember the allocation before the drag begin: this is necessary
     because we actually decide what docking action happens depending
     on it, instead of using the current allocation (which might be
     constantly changing while the user drags things around).  */
  child->drag_allocation = widget->allocation;

  /* Tell the band that this child is being dragged.  The function
     will simply ignore the information if the item is not the band's
     child.  */
  gnome_dock_band_drag_begin (child->band, GNOME_DOCK_ITEM (user_data));
}

static void
drag_begin (GtkWidget *widget, gpointer data)
{
  GnomeDock *dock;
  GnomeDockItem *item;

  DEBUG (("entering function"));

  dock = GNOME_DOCK (data);
  item = GNOME_DOCK_ITEM (widget);

  /* Communicate all the bands that `widget' is currently being
     dragged.  */
  g_list_foreach (dock->top_bands, drag_begin_foreach_func, item);
  g_list_foreach (dock->bottom_bands, drag_begin_foreach_func, item);
  g_list_foreach (dock->right_bands, drag_begin_foreach_func, item);
  g_list_foreach (dock->left_bands, drag_begin_foreach_func, item);

  dock->client_rect = dock->client_area->allocation;
}



/* "drag_end" signal handling.  */

static void
drag_end_bands (GList **list, GnomeDockItem *item)
{
  GList *lp;

  lp = *list;
  while (lp != NULL)
    {
      GnomeDockChild *child;

      child = (GnomeDockChild *) lp->data;

      gnome_dock_band_drag_end (child->band, item);

      child->new_for_drag = FALSE;

      if (gnome_dock_band_get_num_children (child->band) == 0)
        {
          GList *next;

          next = lp->next;

          /* This will remove this link, too.  */
          gtk_widget_destroy (GTK_WIDGET (child->band));

          lp = next;
        }
      else
        lp = lp->next;
    }
}

static void
drag_end (GtkWidget *widget, gpointer data)
{
  GnomeDockItem *item;
  GnomeDock *dock;

  DEBUG (("entering function"));

  item = GNOME_DOCK_ITEM (widget);
  dock = GNOME_DOCK (data);

  /* Communicate to all the bands that `item' is no longer being
     dragged.  */
  drag_end_bands (&dock->top_bands, item);
  drag_end_bands (&dock->bottom_bands, item);
  drag_end_bands (&dock->left_bands, item);
  drag_end_bands (&dock->right_bands, item);

  gtk_signal_emit (GTK_OBJECT (data), dock_signals[LAYOUT_CHANGED]);
}



/* "drag_motion" signal handling.  */

/* Handle a drag motion on the GnomeDockItem `widget'.  This is
   connected to the "drag_motion" of all the children being added to
   the GnomeDock, and tries to dock the dragged item at the current
   (`x', `y') position of the pointer.  */
static void
drag_motion (GtkWidget *widget,
             gint x, gint y,
             gpointer data)
{
#define SNAP 20
  GnomeDockItem *item;
  GnomeDockItemBehavior item_behavior;
  GnomeDock *dock;
  gint win_x, win_y;
  gint rel_x, rel_y;
  gboolean item_allows_horizontal, item_allows_vertical;

  dock = GNOME_DOCK (data);
  item = GNOME_DOCK_ITEM (widget);

  item_behavior = gnome_dock_item_get_behavior (item);
  item_allows_horizontal = ! (item_behavior
                              & GNOME_DOCK_ITEM_BEH_NEVER_HORIZONTAL);
  item_allows_vertical = ! (item_behavior
                            & GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL);

  gdk_window_get_origin (GTK_WIDGET (dock)->window, &win_x, &win_y);
  rel_x = x - win_x;
  rel_y = y - win_y;

  DEBUG (("(%d,%d)", x, y));
  DEBUG (("relative (%d,%d)", rel_x, rel_y));

  if (item_allows_horizontal
      && rel_x >= 0 && rel_x < GTK_WIDGET (dock)->allocation.width)
    {
      /* Check prepending to top/bottom bands.  */
      if (rel_y < 0 && rel_y >= -SNAP
          && drag_new (dock, item, &dock->top_bands, NULL,
                       rel_x, rel_y, FALSE))
        return;
      else if (rel_y >= dock->client_rect.y + dock->client_rect.height - SNAP
               && rel_y < dock->client_rect.y + dock->client_rect.height
               && drag_new (dock, item, &dock->bottom_bands, NULL,
                            rel_x, rel_y, FALSE))
        return;
    }

  if (item_allows_vertical
      && rel_y >= dock->client_rect.y
      && rel_y < dock->client_rect.y + dock->client_rect.height)
    {
      /* Check prepending to left/right bands.  */
      if (rel_x < 0 && rel_x >= -SNAP
          && drag_new (dock, item, &dock->left_bands, NULL,
                       rel_x, rel_y, TRUE))
        return;
      else if (rel_x >= dock->client_rect.x + dock->client_rect.width - SNAP
               && rel_x < dock->client_rect.x + dock->client_rect.width
               && drag_new (dock, item, &dock->right_bands, NULL,
                            rel_x, rel_y, TRUE))
        return;
    }

  /* Check dragging into bands.  */
  if (item_allows_horizontal
      && drag_check (dock, item, &dock->top_bands, rel_x, rel_y, FALSE))
    return;
  else if (item_allows_horizontal
           && drag_check (dock, item, &dock->bottom_bands, rel_x, rel_y, FALSE))
    return;
  else if (item_allows_vertical
           && drag_check (dock, item, &dock->left_bands, rel_x, rel_y, TRUE))
    return;
  else if (item_allows_vertical
           && drag_check (dock, item, &dock->right_bands, rel_x, rel_y, TRUE))
    return;

  /* We are not in any "interesting" area: the item must be floating
     if allowed to.  */
  if (! (item_behavior & GNOME_DOCK_ITEM_BEH_NEVER_DETACH))
    drag_floating (dock, item, x, y, rel_x, rel_y);
}



static GnomeDockItem *
get_docked_item_by_name (GnomeDock *dock,
                         const gchar *name,
                         GnomeDockPlacement *placement_return,
                         guint *num_band_return,
                         guint *band_position_return,
                         guint *offset_return)
{
  {
    struct
    {
      GList *band_list;
      GnomeDockPlacement placement;
    }
    areas[] =
    {
      { dock->top_bands, GNOME_DOCK_TOP },
      { dock->bottom_bands, GNOME_DOCK_BOTTOM },
      { dock->left_bands, GNOME_DOCK_LEFT },
      { dock->right_bands, GNOME_DOCK_RIGHT },
      { NULL, GNOME_DOCK_FLOATING },
    }, *p;
    GtkWidget *item;
    GList *lp;

    for (p = areas; p->band_list != NULL; p++)
      {
        guint num_band;

        for (lp = p->band_list, num_band = 0;
             lp != NULL;
             lp = lp->next, num_band++)
          {
            GnomeDockChild *child;
            GnomeDockItem *item;

            child = lp->data;
            item = gnome_dock_band_get_item_by_name (child->band,
                                                     name,
                                                     band_position_return,
                                                     offset_return);
            if (item != NULL)
              {
                if (num_band_return != NULL)
                  *num_band_return = num_band;
                if (placement_return != NULL)
                  *placement_return = areas->placement;

                return item;
              }
          }
      }
  }

  return NULL;
}

static GnomeDockItem *
get_floating_item_by_name (GnomeDock *dock,
                           const gchar *name)
{
  GList *lp;
  GnomeDockItem *item;

  for (lp = dock->floating_children; lp != NULL; lp = lp->next)
    {
      item = lp->data;
      if (strcmp (item->name, name) == 0)
        return item;
    }

  return NULL;
}

static void
connect_drag_signals (GnomeDock *dock,
                      GtkWidget *item)
{
  if (GNOME_IS_DOCK_ITEM (item))
    {
      DEBUG (("here"));
      gtk_signal_connect (GTK_OBJECT (item), "dock_drag_begin",
                          GTK_SIGNAL_FUNC (drag_begin), (gpointer) dock);
      gtk_signal_connect (GTK_OBJECT (item), "dock_drag_motion",
                          GTK_SIGNAL_FUNC (drag_motion), (gpointer) dock);
      gtk_signal_connect (GTK_OBJECT (item), "dock_drag_end",
                          GTK_SIGNAL_FUNC (drag_end), (gpointer) dock);
    }
}



GtkType
gnome_dock_get_type (void)
{
  static GtkType dock_type = 0;

  if (dock_type == 0)
    {
      GtkTypeInfo dock_info =
      {
	"GnomeDock",
	sizeof (GnomeDock),
	sizeof (GnomeDockClass),
	(GtkClassInitFunc) gnome_dock_class_init,
	(GtkObjectInitFunc) gnome_dock_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      dock_type = gtk_type_unique (gtk_container_get_type (), &dock_info);
    }

  return dock_type;
}

GtkWidget *
gnome_dock_new (void)
{
  GnomeDock *dock;
  GtkWidget *widget;

  dock = gtk_type_new (gnome_dock_get_type ());
  widget = GTK_WIDGET (dock);

#if 0                           /* FIXME: should I? */
  if (GTK_WIDGET_VISIBLE (widget))
    gtk_widget_queue_resize (widget);
#endif

  return widget;
}

void
gnome_dock_add_item (GnomeDock *dock,
                     GnomeDockItem *item,
                     GnomeDockPlacement placement,
                     guint band_num,
                     gint position,
                     guint offset,
                     gboolean in_new_band)
{
  GnomeDockChild *c;
  GList **band_ptr;
  GList *p;

  DEBUG (("side %d band_num %d offset %d position %d in_new_band %d",
          side, band_num, offset, position, in_new_band));

  switch (placement)
    {
    case GNOME_DOCK_TOP:
      band_ptr = &dock->top_bands;
      break;
    case GNOME_DOCK_BOTTOM:
      band_ptr = &dock->bottom_bands;
      break;
    case GNOME_DOCK_LEFT:
      band_ptr = &dock->left_bands;
      break;
    case GNOME_DOCK_RIGHT:
      band_ptr = &dock->right_bands;
      break;
    case GNOME_DOCK_FLOATING:
      g_warning ("Floating dock items not supported by `gnome_dock_add_item'.");
      return;
    default:
      g_error ("Unknown dock placement.");
      return;
    }

  p = g_list_nth (*band_ptr, band_num);
  if (in_new_band || p == NULL)
    {
      GtkWidget *new_band;
      GnomeDockChild *new_child;

      new_band = gnome_dock_band_new ();

      new_child = g_new (GnomeDockChild, 1);
      new_child->band = GNOME_DOCK_BAND (new_band);
      new_child->new_for_drag = FALSE;

      /* FIXME: slow.  */
      if (in_new_band)
        {
          *band_ptr = g_list_insert (*band_ptr, new_child, band_num);
          p = g_list_nth (*band_ptr, band_num);
          if (p == NULL)
            p = g_list_last (*band_ptr);
        }
      else
        {
          *band_ptr = g_list_append (*band_ptr, new_child);
          p = g_list_last (*band_ptr);
        }

      if (placement == GNOME_DOCK_TOP
          || placement == GNOME_DOCK_BOTTOM)
        gnome_dock_band_set_orientation (GNOME_DOCK_BAND (new_band),
                                         GTK_ORIENTATION_HORIZONTAL);
      else
        gnome_dock_band_set_orientation (GNOME_DOCK_BAND (new_band),
                                         GTK_ORIENTATION_VERTICAL);

      gtk_widget_set_parent (new_band, GTK_WIDGET (dock));
      gtk_widget_show (new_band);
      gtk_widget_queue_resize (GTK_WIDGET (dock));
    }

  c = (GnomeDockChild *) p->data;
  gnome_dock_band_insert (c->band, GTK_WIDGET(item), offset, position);

  connect_drag_signals (dock, GTK_WIDGET(item));

  gtk_signal_emit (GTK_OBJECT (dock), dock_signals[LAYOUT_CHANGED]);
}

void
gnome_dock_add_floating_item (GnomeDock *dock,
                              GnomeDockItem *item,
                              gint x, gint y,
                              GtkOrientation orientation)
{
  GtkWidget *widget;

  g_return_if_fail (GNOME_IS_DOCK_ITEM (item));

  widget = GTK_WIDGET(item);

  gnome_dock_item_set_orientation (GNOME_DOCK_ITEM (widget), orientation);

  gtk_widget_ref (widget);
  if (widget->parent != NULL)
    gtk_container_remove (GTK_CONTAINER (widget->parent), widget);
  gtk_widget_set_parent (widget, GTK_WIDGET (dock));

  gnome_dock_item_detach (GNOME_DOCK_ITEM (widget), x, y);
  dock->floating_children = g_list_prepend (dock->floating_children, widget);

  connect_drag_signals (dock, widget);

  gtk_widget_unref (widget);

  gtk_signal_emit (GTK_OBJECT (dock), dock_signals[LAYOUT_CHANGED]);
}

void
gnome_dock_set_client_area (GnomeDock *dock, GtkWidget *widget)
{
  g_return_if_fail (dock != NULL);

  if (widget != NULL)
      gtk_widget_ref (widget);

  if (dock->client_area != NULL)
    gtk_widget_unparent (dock->client_area);

  if (widget != NULL)
    {
      gtk_widget_set_parent (widget, GTK_WIDGET (dock));
      dock->client_area = widget;

      if (GTK_WIDGET_VISIBLE (widget->parent))
        {
          if (GTK_WIDGET_REALIZED (widget->parent) &&
              !GTK_WIDGET_REALIZED (widget))
            gtk_widget_realize (widget);
      
          if (GTK_WIDGET_MAPPED (widget->parent) &&
              !GTK_WIDGET_MAPPED (widget))
            gtk_widget_map (widget);
        }
  
      if (GTK_WIDGET_VISIBLE (widget) && GTK_WIDGET_VISIBLE (dock))
        gtk_widget_queue_resize (widget);
    }
  else
    {
      if (dock->client_area != NULL && GTK_WIDGET_VISIBLE (dock))
        gtk_widget_queue_resize (GTK_WIDGET (dock));
      dock->client_area = NULL;
    }

  if (widget != NULL)
    gtk_widget_unref (widget);
}

GtkWidget *
gnome_dock_get_client_area (GnomeDock *dock)
{
  return dock->client_area;
}

GnomeDockItem *
gnome_dock_get_item_by_name (GnomeDock *dock,
                             const gchar *name,
                             GnomeDockPlacement *placement_return,
                             guint *num_band_return,
                             guint *band_position_return,
                             guint *offset_return)
{
  GnomeDockItem *item;

  item = get_docked_item_by_name (dock,
                                  name,
                                  placement_return,
                                  num_band_return,
                                  band_position_return,
                                  offset_return);
  if (item != NULL)
    return item;

  item = get_floating_item_by_name (dock, name);
  if (item != NULL)
    {
      if (placement_return != NULL)
        *placement_return = GNOME_DOCK_FLOATING;
      return item;
    }

  return NULL;
}



/* Layout functions.  */

static void
layout_add_floating (GnomeDock *dock,
                     GnomeDockLayout *layout)
{
  GList *lp;

  for (lp = dock->floating_children; lp != NULL; lp = lp->next)
    {
      GtkOrientation orientation;
      gint x, y;
      GnomeDockItem *item;

      item = GNOME_DOCK_ITEM (lp->data);

      orientation = gnome_dock_item_get_orientation (item);
      gnome_dock_item_get_floating_position (item, &x, &y);

      gnome_dock_layout_add_floating_item (layout, item,
                                           x, y,
                                           orientation);
    }
}

static void
layout_add_bands (GnomeDock *dock,
                  GnomeDockLayout *layout,
                  GnomeDockPlacement placement,
                  GList *band_list)
{
  guint band_num;
  GList *lp;

  for (lp = band_list, band_num = 0;
       lp != NULL;
       lp = lp->next, band_num++)
    {
      GnomeDockChild *child;

      child = lp->data;
      gnome_dock_band_layout_add (child->band, layout, placement, band_num);
    }
}

GnomeDockLayout *
gnome_dock_get_layout (GnomeDock *dock)
{
  GnomeDockLayout *layout;

  layout = gnome_dock_layout_new ();

  layout_add_bands (dock, layout, GNOME_DOCK_TOP, dock->top_bands);
  layout_add_bands (dock, layout, GNOME_DOCK_BOTTOM, dock->bottom_bands);
  layout_add_bands (dock, layout, GNOME_DOCK_LEFT, dock->left_bands);
  layout_add_bands (dock, layout, GNOME_DOCK_RIGHT, dock->right_bands);

  layout_add_floating (dock, layout);

  return layout;
}

gboolean
gnome_dock_add_from_layout (GnomeDock *dock,
                            GnomeDockLayout *layout)
{
  return gnome_dock_layout_add_to_dock (layout, dock);
}
