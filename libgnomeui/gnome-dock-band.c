/* gnome-dock-band.c

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

#include "gnome-dock-band.h"
#include "gnome-dock-item.h"



#define noGNOME_DOCK_BAND_DEBUG

/* FIXME: To be removed.  */
#if defined GNOME_DOCK_BAND_DEBUG && defined __GNUC__
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
     


/* FIXME: check usage of guint instead of gint.  */

static void gnome_dock_band_class_init (GnomeDockBandClass *class);
static void gnome_dock_band_init (GnomeDockBand *app);
static void gnome_dock_band_size_request (GtkWidget *widget,
                                          GtkRequisition *requisition);
static void gnome_dock_band_size_allocate (GtkWidget *widget,
                                           GtkAllocation *allocation);
static void gnome_dock_band_map (GtkWidget *widget);
static void gnome_dock_band_draw (GtkWidget *widget,
                                  GdkRectangle *area);
static gint gnome_dock_band_expose (GtkWidget *widget,
                                    GdkEventExpose *event);
static void gnome_dock_band_add (GtkContainer *container,
                                 GtkWidget *child);
static void gnome_dock_band_remove (GtkContainer *container,
                                    GtkWidget *widget);
static void gnome_dock_band_forall (GtkContainer *container,
                                    gboolean include_internals,
                                    GtkCallback callback,
                                    gpointer callback_data);

static gboolean docking_allowed (GnomeDockBand *band, GnomeDockItem *item);
static GList *find_child (GnomeDockBand *band, GtkWidget *child);
static GList *prev_if_floating (GnomeDockBand *band, GList *c);
static GList *next_if_floating (GnomeDockBand *band, GList *c);
static GList *prev_not_floating (GnomeDockBand *band, GList *c);
static GList *next_not_floating (GnomeDockBand *band, GList *c);
static void calc_prev_and_foll_space (GnomeDockBand *band);
static guint attempt_move_backward (GnomeDockBand *band,
                                    GList *child,
                                    guint amount);
static guint attempt_move_forward (GnomeDockBand *band,
                                   GList *child,
                                   guint amount);
static gboolean dock_nonempty (GnomeDockBand *band,
                               GnomeDockItem *item,
                               GList *where,
                               gint x, gint y);
static gboolean dock_empty (GnomeDockBand *band,
                            GnomeDockItem *item,
                            GList *where,
                            gint x, gint y);
static gboolean dock_empty_right (GnomeDockBand *band,
                                  GnomeDockItem *item,
                                  GList *where,
                                  gint x, gint y);



static GtkContainerClass *parent_class = NULL;



static void
gnome_dock_band_class_init (GnomeDockBandClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;
  container_class = (GtkContainerClass *) class;

  parent_class = gtk_type_class (gtk_container_get_type ());

  /* object_class->destroy = destroy; */

  widget_class->map = gnome_dock_band_map;
  widget_class->draw = gnome_dock_band_draw;
  widget_class->expose_event = gnome_dock_band_expose;
  widget_class->size_request = gnome_dock_band_size_request;
  widget_class->size_allocate = gnome_dock_band_size_allocate;

  container_class->add = gnome_dock_band_add;
  container_class->remove = gnome_dock_band_remove;
  container_class->forall = gnome_dock_band_forall;
}

static void
gnome_dock_band_init (GnomeDockBand *band)
{
  GTK_WIDGET_SET_FLAGS (band, GTK_NO_WINDOW);

  band->orientation = GTK_ORIENTATION_HORIZONTAL;
  band->children = NULL;
  band->floating_child = NULL;

  band->num_children = 0;

  band->doing_drag = FALSE;
}



static void
gnome_dock_band_size_request (GtkWidget *widget,
                              GtkRequisition *requisition)
{
  GnomeDockBand *band;
  gint width, height;
  GList *lp;
  
  band = GNOME_DOCK_BAND (widget);

  width = height = 0;
  for (lp = band->children; lp != NULL; lp = lp->next)
    {
      GnomeDockBandChild *c = lp->data;

      if (GTK_WIDGET_VISIBLE (c->widget))
        {
          GtkRequisition req;

          gtk_widget_size_request (c->widget, &req);

          if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
            {
              height = MAX (height, req.height);
              width += req.width;
            }
          else
            {
              width = MAX (width, req.width);
              height += req.height;
            }

          c->widget->requisition = req;
        }
    }

  requisition->width = width;
  requisition->height = height;

  widget->requisition = *requisition;
}



static void
gnome_dock_band_size_allocate (GtkWidget *widget,
                               GtkAllocation *allocation)
{
  GnomeDockBand *band;
  GList *lp;
  guint w, uw;
  guint tot_offsets;

  band = GNOME_DOCK_BAND (widget);

  widget->allocation = *allocation;

  /* Check if we have a single exclusive item.  If so, allocate the
     whole space to it.  */
  if (band->num_children == 1)
    {
      GnomeDockBandChild *c;

      c = (GnomeDockBandChild *) band->children->data;
      if (GNOME_IS_DOCK_ITEM (c->widget))
        {
          GnomeDockItemBehavior behavior;
          GnomeDockItem *item;

          item = GNOME_DOCK_ITEM (c->widget);
          behavior = gnome_dock_item_get_behavior (item);
          if (behavior & GNOME_DOCK_ITEM_BEH_EXCLUSIVE)
            {
              gtk_widget_size_allocate (c->widget, allocation);
              return;
            }
        }
    }

  if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      uw = allocation->width;
      w = widget->requisition.width;
    }
  else
    {
      uw = allocation->height;
      w = widget->requisition.height;
    }

  /* FIXME: This could be cached somewhere.  */
  for (lp = band->children, tot_offsets = 0; lp != NULL; lp = lp->next)
    {
      GnomeDockBandChild *c;

      c = lp->data;
      tot_offsets += c->offset;
    }

  w += tot_offsets;

  if (uw >= w)
    {
      /* The allocation is large enough to keep all the requested offsets.  */
      for (lp = band->children; lp != NULL; lp = lp->next)
        {
          GnomeDockBandChild *c;

          c = lp->data;
          c->real_offset = c->offset;
        }
    }
  else
    {
      gfloat delta = (float) (w - uw);

      /* Shrink the offsets proportionally.  */
      for (lp = band->children; lp != NULL; lp = lp->next)
        {
          GnomeDockBandChild *c;

          c = lp->data;
          c->real_offset = (c->offset
                            - (guint) (((float) delta / (float) tot_offsets)
                                       * c->offset + .5));
        }
    }

  /* Allocate children size.  */
  /* FIXME: We could avoid one scan and make things faster.  */

  {
    GtkAllocation child_allocation;
    
    child_allocation.x = widget->allocation.x;
    child_allocation.y = widget->allocation.y;

    if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
      child_allocation.height = widget->allocation.height;
    else
      child_allocation.width = widget->allocation.width;

    for (lp = band->children; lp != NULL; lp = lp->next)
      {
        GnomeDockBandChild *c;

        c = lp->data;

        if (GTK_WIDGET_VISIBLE (c->widget))
          {
            if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
              {
                child_allocation.width = c->widget->requisition.width;
                child_allocation.x += c->real_offset;
              }
            else
              {
                child_allocation.height = c->widget->requisition.height;
                child_allocation.y += c->real_offset;
              }

            gtk_widget_size_allocate (c->widget, &child_allocation);

            if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
              child_allocation.x += child_allocation.width;
            else
              child_allocation.y += child_allocation.height;
          }
      }
  }

  calc_prev_and_foll_space (band);
}



void
gnome_dock_band_map (GtkWidget *widget)
{
  GnomeDockBand *band = GNOME_DOCK_BAND (widget);
  GList *lp;

  GTK_WIDGET_SET_FLAGS (widget, GTK_MAPPED);

  for (lp = band->children; lp != NULL; lp = lp->next)
    {
      GnomeDockBandChild *c;

      c = lp->data;
      if (GTK_WIDGET_VISIBLE (c->widget) && ! GTK_WIDGET_MAPPED (c->widget))
        gtk_widget_map (c->widget);
    }
}

/* FIXME: unmap */

void
gnome_dock_band_draw (GtkWidget *widget, GdkRectangle *area)
{
  g_return_if_fail (widget != NULL);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GList *lp;
      GnomeDockBand *band;

      band = GNOME_DOCK_BAND (widget);

      for (lp = band->children; lp != NULL; lp = lp->next)
        {
          GdkRectangle child_area;
          GnomeDockBandChild *c;
          GtkWidget *w;

          c = lp->data;
          w = c->widget;
          if (gtk_widget_intersect (w, area, &child_area))
            gtk_widget_draw (w, &child_area);
        }
    }
}

static gint
gnome_dock_band_expose (GtkWidget *widget, GdkEventExpose *event)
{
  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GList *lp;
      GnomeDockBand *band;
      GdkEventExpose child_event;

      band = GNOME_DOCK_BAND (widget);
      child_event = *event;

      for (lp = band->children; lp != NULL; lp = lp->next)
        {
          GnomeDockBandChild *c;
          GtkWidget *w;

          c = lp->data;

          w = c->widget;
          if (GTK_WIDGET_NO_WINDOW (w)
              && gtk_widget_intersect (w, &event->area, &child_event.area))
            gtk_widget_event (w, (GdkEvent *) &child_event);
        }
    }

  return FALSE;
}


/* GtkContainer methods.  */

static void
gnome_dock_band_add (GtkContainer *container, GtkWidget *child)
{
  GnomeDockBand *band = GNOME_DOCK_BAND (container);

  g_return_if_fail (gnome_dock_band_prepend (band, child, 0));
}

static void
gnome_dock_band_remove (GtkContainer *container, GtkWidget *widget)
{
  GnomeDockBand *band;
  GList *child;

  band = GNOME_DOCK_BAND (container);
  if (band->num_children == 0)
    return;

  child = find_child (band, widget);
  if (child != NULL)
    {
      gboolean was_visible;

      if (child == band->floating_child)
        band->floating_child = NULL;

      was_visible = GTK_WIDGET_VISIBLE (widget);
      widget->parent = NULL;    /* FIXME? */

      band->children = g_list_remove_link (band->children, child);
      g_free (child->data);
      g_list_free (child);

      if (band->doing_drag)
        {
          GList *p;

          for (p = band->children; p != NULL; p = p->next)
            {
              GnomeDockBandChild *c;

              c = (GnomeDockBandChild *) p->data;
              c->offset = c->real_offset = c->drag_offset;
            }
        }

      gtk_widget_queue_resize (GTK_WIDGET (band));

      band->num_children--;
      DEBUG (("now num_children = %d", band->num_children));
    }
}

static void
gnome_dock_band_forall (GtkContainer *container,
                        gboolean include_internals,
                        GtkCallback callback,
                        gpointer callback_data)
{
  GnomeDockBand *band;
  GnomeDockBandChild *child;
  GList *children;

  band = GNOME_DOCK_BAND (container);

  children = band->children;
  while (children)
    {
      child = children->data;
      children = children->next;
      (* callback) (child->widget, callback_data);
    }
}


/* Utility functions.  */

static gboolean
docking_allowed (GnomeDockBand *band, GnomeDockItem *item)
{
  GnomeDockItemBehavior behavior;
  GnomeDockBandChild *c;

  behavior = gnome_dock_item_get_behavior (item);

  if (band->num_children == 0 || !(behavior & GNOME_DOCK_ITEM_BEH_EXCLUSIVE))
    return TRUE;

  if (band->num_children > 1)
    return FALSE;

  c = (GnomeDockBandChild *) band->children->data;
  if (c->widget != GTK_WIDGET (item))
    return FALSE;

  return TRUE;
}

static GList *
find_child (GnomeDockBand *band, GtkWidget *child)
{
  GList *children;

  children = band->children;

  while (children != NULL)
    {
      GnomeDockBandChild *c;

      c = (GnomeDockBandChild *) children->data;
      if (c->widget == child)
        return children;

      children = children->next;
    }

  return NULL;
}

static GList *
next_if_floating (GnomeDockBand *band, GList *c)
{
  if (c != NULL && c == band->floating_child)
    return c->next;
  else
    return c;
}

static GList *
prev_if_floating (GnomeDockBand *band, GList *c)
{
  if (c != NULL && c == band->floating_child)
    return c->prev;
  else
    return c;
}

static GList *
next_not_floating (GnomeDockBand *band, GList *c)
{
  if (c == NULL)
    return NULL;
  else
    return next_if_floating (band, c->next);
}

static GList *
prev_not_floating (GnomeDockBand *band, GList *c)
{
  if (c == NULL)
    return NULL;
  else
    return prev_if_floating (band, c->prev);
}



static GList *
find_where (GnomeDockBand *band, gint offset, gboolean *is_empty)
{
  guint count;                  /* FIXME: used for debugging only */
  gint offs;
  GList *lp;

  if (offset < 0)
    offset = 0;

  offs = 0;
  count = 0;                    /* FIXME */
  for (lp = band->children; lp != NULL; lp = lp->next)
    {
      GnomeDockBandChild *child;

      child = lp->data;

      if (lp == band->floating_child)
        {
          if (lp->next == NULL)
            {
              DEBUG (("empty last %d", count));
              *is_empty = TRUE;

              return lp == band->floating_child ? lp->prev : lp;
            }
          DEBUG (("%d: is floating or dragged.", count++));
          continue;
        }

      DEBUG (("%d: Checking for x %d, width %d, offs %d (%d)",
              count, child->drag_allocation.x,
              child->drag_allocation.width, offs, offset));

      if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
        {
          if (offset >= offs && offset <= child->drag_allocation.x)
            {
              *is_empty = TRUE;
              DEBUG (("empty %d (allocation.x %d)",
                      count, child->drag_allocation.x));

              return prev_if_floating (band, lp->prev);
            }

          offs = child->drag_allocation.x + child->drag_allocation.width;
          if (offset > child->drag_allocation.x && offset < offs)
            {
              *is_empty = FALSE;
              DEBUG (("%d", count));
              return lp->prev;
            }
        }
      else
        {
          if (offset >= offs && offset <= child->drag_allocation.y)
            {
              *is_empty = TRUE;
              DEBUG (("empty %d (allocation.y %d)",
                      count, child->drag_allocation.y));

              return prev_if_floating (band, lp->prev);
            }

          offs = child->drag_allocation.y + child->drag_allocation.height;
          if (offset > child->drag_allocation.y && offset < offs)
            {
              *is_empty = FALSE;
              DEBUG (("%d", count));
              return lp->prev;
            }
        }

      if (lp->next == NULL)
        {
          DEBUG (("empty last %d", count));
          *is_empty = TRUE;
          return lp;
        }

      count++;                  /* FIXME */
    }

  DEBUG (("nothing done."));

  /* Make compiler happy.  */
  *is_empty = TRUE;
  return lp;
}



static void
calc_prev_and_foll_space (GnomeDockBand *band)
{
  GtkWidget *widget;
  GList *lp;

  if (band->children == NULL)
    return;

  widget = GTK_WIDGET (band);

  lp = next_if_floating (band, band->children);
  if (lp != NULL)
    {
      GnomeDockBandChild *c;
      guint prev_space, foll_space;

      prev_space = 0;

      while (1)
        {
          GList *next;

          c = lp->data;
          prev_space += c->real_offset;
          c->prev_space = prev_space;

          next = next_not_floating (band, lp);
          if (next == NULL)
            break;

          lp = next;
        }

      if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
        foll_space = (widget->allocation.x + widget->allocation.width
                      - (c->widget->allocation.x
                         + c->widget->allocation.width));
      else
        foll_space = (widget->allocation.y + widget->allocation.height
                      - (c->widget->allocation.y
                         + c->widget->allocation.height));

      DEBUG(("foll_space %d", foll_space));

      for (; lp != NULL; lp = prev_not_floating (band, lp))
        {
          c = lp->data;
          c->foll_space = foll_space;
          foll_space += c->real_offset;
        }
    }
}



static guint
attempt_move_backward (GnomeDockBand *band, GList *child, guint amount)
{
  GList *lp;
  guint effective_amount;

  effective_amount = 0;

  for (lp = prev_if_floating (band, child);
       lp != NULL && amount > 0;
       lp = prev_not_floating (band, lp))
    {
      GnomeDockBandChild *c;

      c = lp->data;

      if (c->drag_offset > amount)
        {
          c->real_offset = c->drag_offset - amount;
          effective_amount += amount;
          amount = 0;
        }
      else
        {
          c->real_offset = 0;
          effective_amount += c->drag_offset;
          amount -= c->drag_offset;
        }
      c->offset = c->real_offset;
    }

  return effective_amount;
}

static guint
attempt_move_forward (GnomeDockBand *band, GList *child, guint requirement)
{
  GList *lp;
  guint effective_amount;

  effective_amount = 0;
  for (lp = next_if_floating (band, child);
       lp != NULL && requirement > 0;
       lp = next_not_floating (band, lp))
    {
      GnomeDockBandChild *c;

      c = lp->data;

      DEBUG (("requirement = %d", requirement));
      if (c->drag_offset > requirement)
        {
          c->real_offset = c->drag_offset - requirement;
          effective_amount += requirement;
          requirement = 0;
        }
      else
        {
          c->real_offset = 0;
          effective_amount += c->drag_offset;
          requirement -= c->drag_offset;
        }
      c->offset = c->real_offset;
    }

  return effective_amount;
}



static void
reparent_if_needed (GnomeDockBand *band,
                    GnomeDockItem *item,
                    gint x, gint y)
{
  if (GTK_WIDGET (item)->parent != GTK_WIDGET (band))
    {
      gnome_dock_item_attach (item, GTK_WIDGET (band), x, y);

      /* Reparenting causes the new floating child to be the first
         item on the child list (see the `remove' method).  */
      band->floating_child = band->children;

      /* Reparenting will remove the grab, so we need to redo it.  */
      gnome_dock_item_grab_pointer (item);
    }
}

static gboolean
dock_nonempty (GnomeDockBand *band,
               GnomeDockItem *item,
               GList *where,
               gint x, gint y)
{
  GnomeDockBandChild *c, *floating_child;
  GtkOrientation orig_item_orientation;
  GtkRequisition item_requisition;
  GList *lp, *next;
  guint amount;
  guint requirement;

  DEBUG (("entering function"));

  if (! docking_allowed (band, item))
    return FALSE;

  if (where == NULL)
    lp = band->children;
  else
    lp = next_not_floating (band, where);

  c = lp->data;

  orig_item_orientation = gnome_dock_item_get_orientation (item);
  if (orig_item_orientation != band->orientation
      && ! gnome_dock_item_set_orientation (item, band->orientation))
    return FALSE;

  gtk_widget_size_request (GTK_WIDGET (item), &item_requisition);

  if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
    requirement = item_requisition.width;
  else
    requirement = item_requisition.height;

  if (c->drag_prev_space + c->drag_foll_space < requirement)
    {
      DEBUG (("not enough space %d %d",
              c->drag_prev_space + c->drag_foll_space,
              requirement));

      /* Restore original orientation.  */
      if (orig_item_orientation != band->orientation)
        gnome_dock_item_set_orientation (item, orig_item_orientation);

      return FALSE;
    }

  if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
    amount = c->drag_allocation.x + c->drag_allocation.width - x;
  else
    amount = c->drag_allocation.y + c->drag_allocation.height - y;

  DEBUG (("amount %d requirement %d", amount, requirement));
  amount = attempt_move_backward (band, lp, amount);

  if (requirement < amount)
    requirement = 0;
  else
    {
      requirement -= amount;
      next = next_not_floating (band, lp);
      if (next != NULL)
        attempt_move_forward (band, next, requirement);
    }

  if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
    reparent_if_needed (band, item, x, GTK_WIDGET (band)->allocation.y);
  else
    reparent_if_needed (band, item, GTK_WIDGET (band)->allocation.x, y);

  floating_child = band->floating_child->data;
  floating_child->offset = floating_child->real_offset = 0;

  if (band->floating_child->prev != lp)
    {
      DEBUG (("moving"));
      band->children = g_list_remove_link (band->children,
                                           band->floating_child);
      band->floating_child->next = lp->next;
      if (band->floating_child->next != NULL)
        band->floating_child->next->prev = band->floating_child;
      band->floating_child->prev = lp;
      lp->next = band->floating_child;
    }

  gtk_widget_queue_resize (floating_child->widget);

  return TRUE;
}

static gboolean
dock_empty (GnomeDockBand *band,
            GnomeDockItem *item,
            GList *where,
            gint x, gint y)
{
  GnomeDockBandChild *floating_child;
  GnomeDockBandChild *c1, *c2;
  GnomeDockItemBehavior behavior;
  GtkOrientation orig_item_orientation;
  GtkRequisition item_requisition;
  GList *lp;
  guint new_offset;
  GtkWidget *item_widget;

  DEBUG (("entering function"));

  if (! docking_allowed (band, item))
    return FALSE;

  if (where != NULL)
    {
      lp = next_not_floating (band, where);

      if (lp == NULL)
        /* Extreme right is a special case.  */
        return dock_empty_right (band, item, where, x, y);
      c1 = where->data;
    }
  else
    {
      c1 = NULL;
      lp = next_if_floating (band, band->children);

      if (lp == NULL)
        {
          /* Only one floating element.  Easy.  */
          GnomeDockBandChild *c;

          if (! gnome_dock_item_set_orientation (item, band->orientation))
            return FALSE;

          if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
            reparent_if_needed (band, item,
                                x, GTK_WIDGET (band)->allocation.y);
          else
            reparent_if_needed (band, item,
                                GTK_WIDGET (band)->allocation.x, y);

          c = band->floating_child->data;

          if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
            c->real_offset = x - GTK_WIDGET (band)->allocation.x;
          else
            c->real_offset = y - GTK_WIDGET (band)->allocation.y;

          c->offset = c->real_offset;

          DEBUG (("simple case"));

          gtk_widget_queue_resize (c->widget);

          return TRUE;
        }
    }

  c2 = lp->data;

  item_widget = GTK_WIDGET (item);

  orig_item_orientation = gnome_dock_item_get_orientation (item);
  if (! gnome_dock_item_set_orientation (item, band->orientation))
    return FALSE;

  /* Check whether there is enough space for the widget.  */
  {
    gint space;

    if (c1 != NULL)
      space = c1->drag_foll_space;
    else
      space = c2->real_offset + c2->drag_foll_space;

    gtk_widget_size_request (item_widget, &item_requisition);

    if (space < (band->orientation == GTK_ORIENTATION_HORIZONTAL
                 ? item_requisition.width
                 : item_requisition.height))
      {
        DEBUG (("not enough space %d", space));

        /* Restore original orientation.  */
        if (orig_item_orientation != band->orientation)
          gnome_dock_item_set_orientation (item, orig_item_orientation);

        return FALSE;
      }
  }

  if (c1 == NULL)
    {
      if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
        new_offset = x - GTK_WIDGET (band)->allocation.x;
      else
        new_offset = y - GTK_WIDGET (band)->allocation.y;
    }
  else
    {
      if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
        new_offset = x - (c1->drag_allocation.x + c1->drag_allocation.width);
      else
        new_offset = y - (c1->drag_allocation.y + c1->drag_allocation.height);
    }

  DEBUG (("new_offset %d", new_offset));

  if (c2->drag_offset >= (new_offset
                          + (band->orientation == GTK_ORIENTATION_HORIZONTAL
                             ? item_requisition.width
                             : item_requisition.height)))
    {
      c2->real_offset = (c2->drag_offset
                         - (new_offset
                            + (band->orientation == GTK_ORIENTATION_HORIZONTAL
                               ? item_requisition.width
                               : item_requisition.height)));
      c2->offset = c2->real_offset;
    }
  else
    {
      guint requisition;
      GList *lp1;

      requisition = new_offset + (band->orientation == GTK_ORIENTATION_HORIZONTAL
                                  ? item_requisition.width
                                  : item_requisition.height);

      DEBUG (("Moving forward %d!", requisition));

      for (lp1 = lp; lp1 != NULL && requisition > 0; )
        {
          GnomeDockBandChild *tmp = lp1->data;
          GList *lp1next;

          if (tmp->drag_offset > requisition)
            {
              tmp->real_offset = tmp->drag_offset - requisition;
              requisition = 0;
            }
          else
            {
              requisition -= tmp->drag_offset;
              tmp->real_offset = 0;
            }
          tmp->offset = tmp->real_offset;

          DEBUG (("Offset %d (drag %d)", tmp->real_offset, tmp->drag_offset));
          lp1next = next_not_floating (band, lp1);
          if (lp1next == NULL)
            {
              if (tmp->drag_foll_space > requisition)
                requisition = 0;
              else
                requisition -= tmp->drag_foll_space;
            }

          lp1 = lp1next;
        }

      if (requisition > 0)
        new_offset -= requisition;
    }

  if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
    reparent_if_needed (band, item, x, GTK_WIDGET (band)->allocation.y);
  else
    reparent_if_needed (band, item, GTK_WIDGET (band)->allocation.x, y);

  floating_child = (GnomeDockBandChild *) band->floating_child->data;
  floating_child->real_offset = floating_child->offset = new_offset;

  band->children = g_list_remove_link (band->children, band->floating_child);

  if (where == NULL)
    {
      band->floating_child->next = band->children;
      band->children->prev = band->floating_child;
      band->children = band->floating_child;
    }
  else
    {
      band->floating_child->next = where->next;
      band->floating_child->prev = where;
      if (where->next != NULL)
        where->next->prev = band->floating_child;
      where->next = band->floating_child;
    }

  gtk_widget_queue_resize (((GnomeDockBandChild *) band->floating_child->data)->widget);

  return TRUE;
}

static gboolean
dock_empty_right (GnomeDockBand *band,
                  GnomeDockItem *item,
                  GList *where,
                  gint x, gint y)
{
  GnomeDockBandChild *c, *floating_child;
  GtkOrientation orig_item_orientation;
  GtkRequisition item_requisition;
  GtkWidget *item_widget;
  gint new_offset;

  g_return_val_if_fail (next_not_floating (band, where) == NULL, FALSE);
  g_return_val_if_fail (band->floating_child != where, FALSE);

  DEBUG (("entering function"));

  if (! docking_allowed (band, item))
    return FALSE;

  item_widget = GTK_WIDGET (item);

  c = where->data;

  orig_item_orientation = gnome_dock_item_get_orientation (item);
  if (orig_item_orientation != band->orientation
      && ! gnome_dock_item_set_orientation (item, band->orientation))
    return FALSE;

  gtk_widget_size_request (item_widget, &item_requisition);

  if (c->drag_prev_space + c->drag_foll_space
      < (guint) (band->orientation == GTK_ORIENTATION_HORIZONTAL
                 ? item_requisition.width
                 : item_requisition.height))
    {
      DEBUG (("not enough space %d ", c->drag_prev_space+ c->drag_foll_space));

      /* Restore original orientation.  */
      if (orig_item_orientation != band->orientation)
        gnome_dock_item_set_orientation (item, orig_item_orientation);

      return FALSE;
    }

  if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
    new_offset = x - (c->widget->allocation.x + c->widget->allocation.width);
  else
    new_offset = y - (c->widget->allocation.y + c->widget->allocation.height);

  DEBUG (("x %d y %d new_offset %d width %d foll_space %d",
          x, y, new_offset, item_widget->allocation.width,
          c->drag_foll_space));

  if ((guint) (new_offset
               + (band->orientation == GTK_ORIENTATION_HORIZONTAL
                  ? item_requisition.width
                  : item_requisition.height)) > c->drag_foll_space)
    {
      gint excess = (new_offset
                     + (band->orientation == GTK_ORIENTATION_HORIZONTAL
                        ? item_requisition.width
                        : item_requisition.height)
                     - c->drag_foll_space);

      DEBUG (("excess %d new_offset %d", excess, new_offset));
      if (excess < new_offset)
        new_offset -= excess;
      else
        {
          attempt_move_backward (band, where, excess - new_offset);
          new_offset = 0; 
        }
    }

  if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
    reparent_if_needed (band, item,
                        x, GTK_WIDGET (band)->allocation.y);
  else
    reparent_if_needed (band, item,
                        GTK_WIDGET (band)->allocation.x, y);

  floating_child = band->floating_child->data;
  floating_child->offset = floating_child->real_offset = new_offset;

  band->children = g_list_remove_link (band->children, band->floating_child);
  where->next = band->floating_child;
  band->floating_child->prev = where;

  gtk_widget_queue_resize (floating_child->widget);

  return TRUE;
}



/* Exported interface.  */

GtkWidget *
gnome_dock_band_new (void)
{
  GnomeDockBand *band;
  GtkWidget *widget;

  band = gtk_type_new (gnome_dock_band_get_type ());
  widget = GTK_WIDGET (band);

  if (GTK_WIDGET_VISIBLE (widget))
    gtk_widget_queue_resize (widget);

  return widget;
}

GtkType
gnome_dock_band_get_type (void)
{
  static GtkType band_type = 0;

  if (band_type == 0)
    {
      GtkTypeInfo band_info =
      {
	"GnomeDockBand",
	sizeof (GnomeDockBand),
	sizeof (GnomeDockBandClass),
	(GtkClassInitFunc) gnome_dock_band_class_init,
	(GtkObjectInitFunc) gnome_dock_band_init,
        /* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };

      band_type = gtk_type_unique (gtk_container_get_type (), &band_info);
    }

  return band_type;
}

void
gnome_dock_band_set_orientation (GnomeDockBand *band,
                                 GtkOrientation orientation)
{
  g_return_if_fail (orientation == GTK_ORIENTATION_HORIZONTAL
                    || orientation == GTK_ORIENTATION_VERTICAL);

  band->orientation = orientation;
}

GtkOrientation
gnome_dock_band_get_orientation (GnomeDockBand *band)
{
  return band->orientation;
}

gboolean
gnome_dock_band_insert (GnomeDockBand *band,
                        GtkWidget *child,
                        guint offset,
                        gint position)
{
  GnomeDockBandChild *band_child;
  guint nchildren;

  DEBUG (("%08x", (unsigned int) band));

  if (GNOME_IS_DOCK_ITEM (child)
      && !docking_allowed (band, GNOME_DOCK_ITEM (child)))
    return FALSE;

  if (position < 0 || position > (gint) band->num_children)
    position = band->num_children;

  band_child = g_new (GnomeDockBandChild, 1);
  band_child->widget = child;
  band_child->offset = offset;

  if (position == 0)
    band->children = g_list_prepend (band->children, band_child);
  else if ((guint) position == band->num_children)
    band->children = g_list_append (band->children, band_child);
  else
    {
      GList *p;

      p = g_list_nth (band->children, position);
      g_list_prepend (p, band_child);
    }

  if (GNOME_IS_DOCK_ITEM (child)
      && ! gnome_dock_item_set_orientation (GNOME_DOCK_ITEM (child),
                                            band->orientation))
      return FALSE;

  gtk_widget_set_parent (child, GTK_WIDGET (band));

  if (GTK_WIDGET_VISIBLE (GTK_WIDGET (band)))
    {
      if (GTK_WIDGET_REALIZED (GTK_WIDGET (band))
          && !GTK_WIDGET_REALIZED (child))
        gtk_widget_realize (child);

      if (GTK_WIDGET_MAPPED (GTK_WIDGET (band))
          && !GTK_WIDGET_MAPPED (child)
          && GTK_WIDGET_VISIBLE (child)) /* as stated by `widget_system.txt' */
        gtk_widget_map (child);
    }

  if (GTK_WIDGET_VISIBLE (band))
    {
      /* gtk_widget_queue_resize (GTK_WIDGET (band)); */
      if (GTK_WIDGET_VISIBLE (child))
        gtk_widget_queue_resize (GTK_WIDGET (child));
    }

  band->num_children++;
  DEBUG (("now num_children = %d", band->num_children));

  return TRUE;
}

void
gnome_dock_band_move_child (GnomeDockBand *band,
                            GList *old_child,
                            guint new_num)
{
  GList *children;
  GList *lp;

  children = band->children;

  lp = old_child;

  children = g_list_remove_link (children, lp);

  children = g_list_insert (children, lp->data, new_num);

  g_list_free (lp);

  band->children = children;

  /* FIXME */
  gtk_widget_queue_resize (GTK_WIDGET (band));
}

gboolean
gnome_dock_band_prepend (GnomeDockBand *band,
                         GtkWidget *child,
                         guint offset)
{
  return gnome_dock_band_insert (band, child, offset, 0);
}

gboolean
gnome_dock_band_append (GnomeDockBand *band,
                        GtkWidget *child,
                        guint offset)
{
  return gnome_dock_band_insert (band, child, offset, -1);
}

void
gnome_dock_band_set_child_offset (GnomeDockBand *band,
                                  GtkWidget *child,
                                  guint offset)
{
  GList *p;

  p = find_child (band, child);
  if (p != NULL)
    {
      GnomeDockBandChild *c;

      c = (GnomeDockBandChild *) p->data;
      c->offset = offset;
      gtk_widget_queue_resize (c->widget);
    }
}

guint
gnome_dock_band_get_child_offset (GnomeDockBand *band,
                                  GtkWidget *child)
{
  GList *p;

  p = find_child (band, child);
  if (p != NULL)
    {
      GnomeDockBandChild *c;

      c = (GnomeDockBandChild *) p->data;
      return c->offset;
    }

  return 0;
}

guint
gnome_dock_band_get_num_children (GnomeDockBand *band)
{
  return band->num_children;
}



void
gnome_dock_band_drag_begin (GnomeDockBand *band, GnomeDockItem *item)
{
  GList *lp;
  GtkWidget *floating_widget;
  GtkWidget *item_widget;
  guint extra_offset = 0;

  DEBUG (("entering function"));

  item_widget = GTK_WIDGET (item);
  floating_widget = NULL;

  for (lp = band->children; lp != NULL;)
    {
      GnomeDockBandChild *c;

      c = lp->data;

      c->drag_allocation = c->widget->allocation;
      c->drag_offset = c->real_offset + extra_offset;
      c->drag_prev_space = c->prev_space;
      c->drag_foll_space = c->foll_space;

      c->offset = c->real_offset;

      if (c->widget == item_widget)
        {
          band->floating_child = lp;
          floating_widget = item_widget;
          if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
            extra_offset = c->widget->allocation.width + c->real_offset;
          else
            extra_offset = c->widget->allocation.height + c->real_offset;
        }
      else
        extra_offset = 0;

      if (lp->next == NULL)
        break;

      lp = lp->next;
    }

  if (floating_widget != NULL)
    {
      for (lp = band->floating_child->prev; lp != NULL; lp = lp->prev)
        {
          GnomeDockBandChild *c;

          c = lp->data;
          if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
            c->drag_foll_space += item_widget->allocation.width;
          else
            c->drag_foll_space += item_widget->allocation.height;
        }
      for (lp = band->floating_child->next; lp != NULL; lp = lp->next)
        {
          GnomeDockBandChild *c;

          c = lp->data;
          if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
            c->drag_prev_space += item_widget->allocation.width;
          else
            c->drag_prev_space += item_widget->allocation.height;
        }
    }

  band->doing_drag = TRUE;
}

void
gnome_dock_band_drag_to (GnomeDockBand *band,
                         GnomeDockItem *item,
                         gint x, gint y)
{
  GnomeDockBandChild *next_child;
  GtkAllocation *allocation;
  GList *where;
  gint x_offs;
  gboolean is_empty;

  g_return_if_fail (band->doing_drag);

  DEBUG (("%d %d", x, y));

  allocation = & GTK_WIDGET (band)->allocation;

  if (band->orientation == GTK_ORIENTATION_HORIZONTAL)
    {
      if (x < allocation->x)
        x = allocation->x;
      if (x >= allocation->x + allocation->width)
        x = allocation->x + allocation->width - 1;
      where = find_where (band, x, &is_empty);
    }
  else
    {
      if (y < allocation->y)
        y = allocation->y;
      if (y >= allocation->y + allocation->height)
        y = allocation->y + allocation->height - 1;
      where = find_where (band, y, &is_empty);
    }

  {
    GList *p;

    for (p = next_if_floating (band, band->children);
         p != NULL;
         p = next_not_floating (band, p))
      {
        GnomeDockBandChild *c = p->data;

        c->real_offset = c->offset = c->drag_offset;
      }
  }

  if (is_empty)
    dock_empty (band, item, where, x, y);
  else
    dock_nonempty (band, item, where, x, y);
}

void
gnome_dock_band_drag_end (GnomeDockBand *band, GnomeDockItem *item)
{
  g_return_if_fail (band->doing_drag);

  DEBUG (("entering function"));

  if (band->floating_child != NULL)
    {
      GnomeDockBandChild *f;
      
      /* Minimal sanity check.  */
      f = (GnomeDockBandChild *) band->floating_child->data;
      g_return_if_fail (f->widget == GTK_WIDGET (item));

      gtk_widget_queue_resize (f->widget);
      band->floating_child = NULL;
    }

  band->doing_drag = FALSE;
}
