/* GNOME GUI Library
 * Copyright (C) 1998 the Free Software Foundation
 *
 * Author: Ettore Perazzoli (ettore@comm2000.it)
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

/* This is a simple widget for adding animations to Gnome
   applications.  It is updated using double buffering for maximum
   smoothness.
   Notice that, although its window is not shaped, the widget tries to
   emulate a shaped window as closely as possible by updating the
   background whenever the parent's background changes.  We don't want
   to use a real shaped window here because changing the shape in the
   middle of the animations causes some ugly flickering.  */

#include <config.h>
#include <gnome.h>

#include "gnome-animator.h"

/* ------------------------------------------------------------------------- */

/* This is kept private as things might change in the future.  */
struct _GnomeAnimatorFrame
  {
    /* Rendered image.  */
    GdkImage *image;

    /* Shape mask (NULL if no shape mask).  */
    GdkBitmap *mask;

    /* X/Y position in the widget.  */
    gint x_offset;
    gint y_offset;

    /* Interval for the next frame (as for `gtk_timeout_add()').  This
       value is divided by `playback_speed' before being used.  */
    guint32 interval;
  };

struct _GnomeAnimatorPrivate
  {
    /* Pixmap used for drawing offscreen.  */
    GdkPixmap *offscreen_pixmap;

    /* Pixmap storing the window's background.  This is kept in sync
       with the parent's window background using the "style_set" and
       "state_changed" signals.  */
    GdkPixmap *background_pixmap;
  };

/* ------------------------------------------------------------------------- */

static void                class_init              (GnomeAnimatorClass *class);
static void                init                    (GnomeAnimator *animator);
static void                destroy                 (GtkObject *object);
static void                prepare_aux_pixmaps     (GnomeAnimator *animator);
static void                draw_background_pixmap  (GnomeAnimator *animator);
static void                realize                 (GtkWidget *widget);
static void                size_allocate           (GtkWidget *widget,
                                                    GtkAllocation *allocation);
static void                draw                    (GtkWidget *widget,
                                                    GdkRectangle *area);
static gint                expose                  (GtkWidget *widget,
                                                    GdkEventExpose *event);
static void                update                  (GnomeAnimator *animator);
static void                setup_window_and_style  (GnomeAnimator *animator);
static GnomeAnimatorFrame *append_frame            (GnomeAnimator *animator);
static gint                timer_cb                (gpointer data);
static void                parent_state_changed_cb (GtkWidget *widget,
                                                    void *data);
static void                parent_style_set_cb     (GtkWidget *widget,
                                                    void *data);

static GtkWidgetClass *parent_class;

/* ------------------------------------------------------------------------- */

static void
init (GnomeAnimator *animator)
{
  GTK_WIDGET_SET_FLAGS (animator, GTK_BASIC);

  animator->frames = NULL;
  animator->num_frames = 0;
  animator->current_frame_number = 0;
  animator->status = GNOME_ANIMATOR_STATUS_STOPPED;
  animator->loop_type = GNOME_ANIMATOR_LOOP_NONE;
  animator->playback_direction = +1;
  animator->timeout_id = 0;
  animator->playback_speed = 1.0;

  animator->privat = g_new (GnomeAnimatorPrivate, 1);
  animator->privat->offscreen_pixmap = NULL;
  animator->privat->background_pixmap = NULL;

  /* This allows us to "emulate" a transparent window, by making our
     background look exactly like the parent's background.  */
  gtk_signal_connect (GTK_OBJECT (animator), "state_changed",
                      GTK_SIGNAL_FUNC (parent_state_changed_cb), NULL);
  gtk_signal_connect (GTK_OBJECT (animator), "style_set",
                      GTK_SIGNAL_FUNC (parent_style_set_cb), NULL);
}

static void
class_init (GnomeAnimatorClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;

  parent_class = gtk_type_class (gtk_widget_get_type ());

  widget_class->realize = realize;
  widget_class->size_allocate = size_allocate;
  widget_class->draw = draw;
  widget_class->expose_event = expose;
  object_class->destroy = destroy;
}

static void
free_all_frames (GnomeAnimator *animator)
{
  if (animator->num_frames > 0)
    {
      guint i;

      for (i = 0; i < animator->num_frames; i++)
        {
          GnomeAnimatorFrame *frame = animator->frames + i;

          if (frame->image != NULL)
            gdk_image_destroy (frame->image);
          if (frame->mask != NULL)
            gdk_imlib_free_bitmap (frame->mask);
        }

      g_free (animator->frames);
    }
}

static void
destroy (GtkObject *object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (GNOME_IS_ANIMATOR (object));

  free_all_frames (GNOME_ANIMATOR (object));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/* Create the offscreen and background pixmaps if they do not exist,
   or resize them in case the widget's size has changed.  */
static void
prepare_aux_pixmaps (GnomeAnimator *animator)
{
  GtkWidget *widget = GTK_WIDGET (animator);
  GnomeAnimatorPrivate *privat = animator->privat;
  gint old_width, old_height;

  if (privat->offscreen_pixmap != NULL)
    gdk_window_get_size (privat->offscreen_pixmap, &old_width, &old_height);
  else
    old_width = old_height = 0;

  if ((widget->requisition.width != (guint16) old_width)
      && (widget->requisition.height != (guint16) old_height))
    {
      GdkVisual *visual = gtk_widget_get_visual (widget);

      if (privat->offscreen_pixmap != NULL)
        {
          gdk_pixmap_unref (privat->offscreen_pixmap);
          privat->offscreen_pixmap = NULL;
        }

      if (privat->background_pixmap != NULL)
        {
          gdk_pixmap_unref (privat->background_pixmap);
          privat->background_pixmap = NULL;
        }

      if (widget->requisition.width > 0
          && widget->requisition.height > 0)
        {
          privat->offscreen_pixmap = gdk_pixmap_new (widget->window,
                                                      widget->requisition.width,
                                                      widget->requisition.height,
                                                      visual->depth);
          privat->background_pixmap = gdk_pixmap_new (widget->window,
                                                       widget->requisition.width,
                                                       widget->requisition.height,
                                                       visual->depth);
        }
    }
}

/* Prepare the background pixmap.  This is called whenever the
   parent's background changes.  Using a background pixmap makes it
   faster to update the window, because this we can just copy this
   pixmap instead of actually redrawing the background rectangle.  */
static void
draw_background_pixmap (GnomeAnimator *animator)
{
  GnomeAnimatorPrivate *privat = animator->privat;

  if (privat->background_pixmap != NULL)
    {
      GtkWidget *widget;
      GtkStyle *style;
      guint state;
      GdkGC *gc;
      gint x, y;

      widget = GTK_WIDGET (animator);
      style = gtk_widget_get_style (widget->parent);
      state = GTK_WIDGET_STATE (widget->parent);
      gc = gdk_gc_new (widget->window);
      gdk_gc_copy (gc, style->bg_gc[state]);

      if (style->bg_pixmap[state] != NULL)
        {
          gdk_gc_set_tile (gc, style->bg_pixmap[state]);
          gdk_gc_set_fill (gc, GDK_TILED);
        }
      else
        gdk_gc_set_fill (gc, GDK_SOLID);

      gdk_window_get_position (widget->window, &x, &y);
      gdk_gc_set_ts_origin (gc, -x, -y);

      gdk_draw_rectangle (privat->background_pixmap,
                          gc,
                          TRUE,
                          0, 0,
                          widget->requisition.width,
                          widget->requisition.height);

      gdk_gc_destroy (gc);
    }
}

static void
realize (GtkWidget *widget)
{
  GnomeAnimator *animator;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_ANIMATOR (widget));

  animator = GNOME_ANIMATOR (widget);
  GTK_WIDGET_SET_FLAGS (widget, GTK_REALIZED);

  setup_window_and_style (animator);

  update (animator);
}

static void
size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  GnomeAnimator *animator;
  gint x, y;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_ANIMATOR (widget));
  g_return_if_fail (allocation != NULL);

  animator = GNOME_ANIMATOR (widget);
  widget->allocation = *allocation;

  /* The animator's window always has the specified size, but is
     centered in the given allocation.  */
  /* FIXME: This does not handle the case when the size is smaller
     than requested very well...  */

  x = widget->allocation.x + (widget->allocation.width
                              - widget->requisition.width) / 2;
  y = widget->allocation.y + (widget->allocation.height
                              - widget->requisition.height) / 2;

  gdk_window_move (widget->window, x, y);

  prepare_aux_pixmaps (animator);
  draw_background_pixmap (animator);

  update (animator);
}

static void
paint (GnomeAnimator *animator, GdkRectangle *area)
{
  if (animator->frames != NULL)
    {
      GnomeAnimatorFrame *frame; 
      GtkWidget *widget; 
      GnomeAnimatorPrivate *privat;

      frame = (animator->frames + animator->current_frame_number);
      widget = GTK_WIDGET (animator);
      privat = animator->privat;
      
      /* Update the window using double buffering to make the
         animation as smooth as possible.  */

      /* Copy the background into the offscreen pixmap.  */
      gdk_draw_pixmap (privat->offscreen_pixmap,
                       widget->style->black_gc,
                       privat->background_pixmap,
                       0, 0,
                       0, 0,
                       widget->requisition.width,
                       widget->requisition.height);

      /* Draw the (shaped) frame into the offscreen pixmap.  */
      gdk_gc_set_clip_mask (widget->style->black_gc, frame->mask);
      gdk_gc_set_clip_origin (widget->style->black_gc,
                              frame->x_offset, frame->y_offset);
      gdk_draw_image (privat->offscreen_pixmap,
                      widget->style->black_gc,
                      frame->image,
                      area->x, area->y,
                      frame->x_offset, frame->y_offset,
                      frame->image->width, frame->image->height);
      gdk_gc_set_clip_mask (widget->style->black_gc, NULL);

      /* Copy the offscreen pixmap into the window.  */
      gdk_draw_pixmap (widget->window,
                       widget->style->black_gc,
                       privat->offscreen_pixmap,
                       area->x, area->y,
                       area->x, area->y,
                       area->width, area->height);
    }
  else
    gdk_window_clear_area (animator->widget.window,
                           area->x, area->y,
                           area->width, area->height);
}

static void
draw (GtkWidget *widget, GdkRectangle *area)
{
  GnomeAnimator *animator;
  GdkRectangle w_area, p_area;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_ANIMATOR (widget));
  g_return_if_fail (area != NULL);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      animator = GNOME_ANIMATOR (widget);

      area->x -= (widget->allocation.width - widget->requisition.width) / 2;
      area->y -= (widget->allocation.height - widget->requisition.height) / 2;

      w_area.x = 0;
      w_area.y = 0;
      w_area.width = widget->requisition.width;
      w_area.height = widget->requisition.height;

      if (gdk_rectangle_intersect (area, &w_area, &p_area))
        paint (animator, &p_area);
    }
}

static gint
expose (GtkWidget *widget, GdkEventExpose *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GNOME_IS_ANIMATOR (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (GTK_WIDGET_REALIZED (widget))
    /* FIXME: We could be smarter here.  */
    gtk_widget_queue_draw (widget);

  return FALSE;
}

static void
update (GnomeAnimator *animator)
{
  GtkWidget *widget = GTK_WIDGET (animator);

  if (GTK_WIDGET_REALIZED (widget))
    {
      if (animator->frames != NULL)
        {
          GnomeAnimatorFrame *frame;

          frame = animator->frames + animator->current_frame_number;

          gdk_window_show (widget->window);

          gtk_widget_queue_draw (widget);
        }
      else
        {
          /* We don't want any window to be visible if there are no
             frames to play; this way, we are 100% transparent.  */
          gdk_window_hide (widget->window);
        }
    }
}

static void
setup_window_and_style (GnomeAnimator *animator)
{
  GdkWindowAttr attr;
  gint attr_mask;
  GtkWidget *widget;

  widget = GTK_WIDGET (animator);

  if (widget->window)
    gdk_window_unref (widget->window);

  attr.window_type = GDK_WINDOW_CHILD;
  attr.wclass = GDK_INPUT_OUTPUT;
  attr.visual = gtk_widget_get_visual (widget);
  attr.colormap = gtk_widget_get_colormap (widget);
  attr.width = widget->requisition.width;
  attr.height = widget->requisition.height;
  attr.x = (widget->allocation.x
            + (widget->allocation.width - attr.width) / 2);
  attr.y = (widget->allocation.y
            + (widget->allocation.height - attr.height) / 2);
  attr.event_mask = gtk_widget_get_events (widget) | GDK_EXPOSURE_MASK;

  attr_mask = GDK_WA_X | GDK_WA_Y | GDK_WA_VISUAL | GDK_WA_COLORMAP;

  widget->window = gdk_window_new (gtk_widget_get_parent_window (widget),
                                   &attr, attr_mask);
  gdk_window_set_user_data (widget->window, widget);

  widget->style = gtk_style_attach (widget->style, widget->window);

  /* Set back pixmap to the parent's one.  */
  gdk_window_set_back_pixmap (widget->window, NULL, TRUE);
}

static GnomeAnimatorFrame *
append_frame (GnomeAnimator *animator)
{
  GnomeAnimatorFrame *new_frame;

  if (animator->frames == NULL)
    animator->frames = g_new (GnomeAnimatorFrame, 1);
  else
    animator->frames = g_realloc (animator->frames,
                                  (sizeof (GnomeAnimatorFrame)
                                   * (animator->num_frames + 1)));

  new_frame = animator->frames + animator->num_frames;
  animator->num_frames++;

  return new_frame;
}

static gint
timer_cb (gpointer data)
{
  GnomeAnimator *animator;

  animator = GNOME_ANIMATOR (data);
  if (animator->status != GNOME_ANIMATOR_STATUS_STOPPED)
    gnome_animator_advance (animator, +1);

  return FALSE;
}

static void
parent_state_changed_cb (GtkWidget *widget, void *data)
{
  GnomeAnimator *animator;

  g_assert (GNOME_IS_ANIMATOR (widget));
  
  animator = GNOME_ANIMATOR (widget);
  prepare_aux_pixmaps (animator);
  draw_background_pixmap (animator);
}

static void
parent_style_set_cb (GtkWidget *widget, void *data)
{
  GnomeAnimator *animator;

  g_assert (GNOME_IS_ANIMATOR (widget));
  
  animator = GNOME_ANIMATOR (widget);
  prepare_aux_pixmaps (animator);
  draw_background_pixmap (animator);
}

/* ------------------------------------------------------------------------- */

/* Public interface.  */

guint
gnome_animator_get_type (void)
{
  static guint animator_type = 0;

  if (!animator_type)
    {
      GtkTypeInfo animator_info =
      {
        "GnomeAnimator",
        sizeof (GnomeAnimator),
        sizeof (GnomeAnimatorClass),
        (GtkClassInitFunc) class_init,
        (GtkObjectInitFunc) init,
        NULL,
        NULL
      };

      animator_type = gtk_type_unique (gtk_widget_get_type (), &animator_info);
    }

  return animator_type;
}

GtkWidget *
gnome_animator_new_with_size (guint width, guint height)
{
  GnomeAnimator *animator;
  GtkWidget *widget;

  animator = gtk_type_new (gnome_animator_get_type ());
  widget = GTK_WIDGET (animator);

  gtk_widget_set_usize (widget, width, height);

  if (GTK_WIDGET_REALIZED (widget))
    {
      if (GTK_WIDGET_MAPPED (widget))
        gdk_window_hide (widget->window);

      setup_window_and_style (animator);
    }

  if (GTK_WIDGET_MAPPED (widget))
    gdk_window_show (widget->window);

  if (GTK_WIDGET_VISIBLE (widget))
    gtk_widget_queue_resize (widget);

  return widget;
}

void
gnome_animator_set_loop_type (GnomeAnimator *animator,
                              GnomeAnimatorLoopType loop_type)
{
  g_return_if_fail (animator != NULL);
  g_return_if_fail (GNOME_IS_ANIMATOR (animator));

  animator->loop_type = loop_type;
}

GnomeAnimatorLoopType
gnome_animator_get_loop_type (GnomeAnimator *animator)
{
  g_return_val_if_fail (animator != NULL, GNOME_ANIMATOR_LOOP_NONE);

  return animator->loop_type;
}

void
gnome_animator_set_playback_direction (GnomeAnimator *animator,
                                       gint playback_direction)
{
  g_return_if_fail (animator != NULL);

  animator->playback_direction = playback_direction;
}

gint
gnome_animator_get_playback_direction (GnomeAnimator *animator)
{
  g_return_val_if_fail (animator != NULL, 0);

  return animator->playback_direction;
}

gboolean
gnome_animator_append_frame_from_imlib_at_size (GnomeAnimator *animator,
                                                GdkImlibImage *image,
                                                gint x_offset,
                                                gint y_offset,
                                                guint32 interval,
                                                guint width,
                                                guint height)
{
  GnomeAnimatorFrame *new_frame;
  GdkPixmap *pixmap;

  g_return_val_if_fail (animator != NULL, FALSE);
  g_return_val_if_fail (image != NULL, FALSE);

  if (width == 0)
    width = image->rgb_width;
  if (height == 0)
    height = image->rgb_height;

  new_frame = append_frame (animator);

  gdk_imlib_render (image, width, height);
  
  pixmap = gdk_imlib_move_image (image);

  /* FIXME: GDK always creates a non-SHM image this way!  A quick look
     at the source reveals that GDK can do XShmPutImage, but not
     XShmGetImage.  So either we get XImage support in Imlib, or this
     needs to be fixed.  */
  new_frame->image = gdk_image_get (pixmap, 0, 0, width, height);
  
  new_frame->mask = gdk_imlib_move_mask (image);
  
  gdk_imlib_free_pixmap (pixmap);
  
  new_frame->x_offset = x_offset;
  new_frame->y_offset = y_offset;

  new_frame->interval = interval;

  return TRUE;
}

gboolean
gnome_animator_append_frame_from_imlib (GnomeAnimator *animator,
                                        GdkImlibImage *image,
                                        gint x_offset,
                                        gint y_offset,
                                        guint32 interval)
{
  return gnome_animator_append_frame_from_imlib_at_size (animator, image,
                                                         x_offset,
                                                         y_offset,
                                                         interval,
                                                         0, 0);
}

gboolean
gnome_animator_append_frame_from_file_at_size (GnomeAnimator *animator,
                                               const gchar *name,
                                               gint x_offset,
                                               gint y_offset,
                                               guint32 interval,
                                               guint width,
                                               guint height)
{
  GdkImlibImage *image;

  g_return_val_if_fail (animator != NULL, FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  image = gdk_imlib_load_image ((char *) name); /* FIXME: This cast is evil. */
  if (image == NULL)
    return FALSE;

  gnome_animator_append_frame_from_imlib_at_size (animator,
                                                  image,
                                                  x_offset,
                                                  y_offset,
                                                  interval,
                                                  width, height);
  gdk_imlib_destroy_image (image);

  return TRUE;
}

gboolean
gnome_animator_append_frame_from_file (GnomeAnimator *animator,
                                       const gchar *name,
                                       gint x_offset,
                                       gint y_offset,
                                       guint32 interval)
{
  return gnome_animator_append_frame_from_file_at_size (animator,
                                                        name,
                                                        x_offset,
                                                        y_offset,
                                                        interval,
                                                        0, 0);
}

gboolean
gnome_animator_append_frames_from_imlib_at_size (GnomeAnimator *animator,
                                                 GdkImlibImage *image,
                                                 gint x_offset,
                                                 gint y_offset,
                                                 guint32 interval,
                                                 gint x_unit,
                                                 guint width,
                                                 guint height)
{
  gint i;

  if (width == 0)
    width = x_unit;
  if (height == 0)
    height = image->rgb_height;

  for (i = 0; i + x_unit < image->rgb_width; i += x_unit)
    {
      GdkImlibImage *tmp;

      tmp = gdk_imlib_crop_and_clone_image (image, i, 0, x_unit,
                                            image->rgb_height);
      gnome_animator_append_frame_from_imlib_at_size (animator,
                                                      tmp,
                                                      x_offset,
                                                      y_offset,
                                                      interval,
                                                      width,
                                                      height);
      gdk_imlib_destroy_image (tmp);
    }

  return TRUE;
}

gboolean
gnome_animator_append_frames_from_imlib (GnomeAnimator *animator,
                                         GdkImlibImage *image,
                                         gint x_offset,
                                         gint y_offset,
                                         guint32 interval,
                                         gint x_unit)
{
  return gnome_animator_append_frames_from_imlib_at_size (animator,
                                                          image,
                                                          x_offset,
                                                          y_offset,
                                                          interval,
                                                          x_unit,
                                                          0, 0);
}

gboolean
gnome_animator_append_frames_from_file_at_size (GnomeAnimator *animator,
                                                const gchar *name,
                                                gint x_offset,
                                                gint y_offset,
                                                guint32 interval,
                                                gint x_unit,
                                                guint width,
                                                guint height)
{
  GdkImlibImage *image;
  gboolean retval;

  g_return_val_if_fail (animator != NULL, FALSE);
  g_return_val_if_fail (name != NULL, FALSE);

  image = gdk_imlib_load_image ((char *) name); /* FIXME: This cast is evil. */
  if (image == NULL)
    return FALSE;

  retval = gnome_animator_append_frames_from_imlib_at_size (animator,
                                                            image,
                                                            x_offset,
                                                            y_offset,
                                                            interval,
                                                            x_unit,
                                                            width,
                                                            height);

  gdk_imlib_destroy_image (image);

  return retval;
}

gboolean
gnome_animator_append_frames_from_file (GnomeAnimator *animator,
                                        const gchar *name,
                                        gint x_offset,
                                        gint y_offset,
                                        guint32 interval,
                                        gint x_unit)
{
  return gnome_animator_append_frames_from_file_at_size (animator,
                                                         name,
                                                         x_offset,
                                                         y_offset,
                                                         interval,
                                                         x_unit,
                                                         0, 0);
}

gboolean
gnome_animator_append_frame_from_gnome_pixmap (GnomeAnimator *animator,
                                               GnomePixmap *pixmap,
                                               gint x_offset,
                                               gint y_offset,
                                               guint32 interval)
{
  GnomeAnimatorFrame *new_frame;
  guint width, height;
  
  g_return_val_if_fail (animator != NULL, FALSE);
  g_return_val_if_fail (pixmap != NULL, FALSE);

  new_frame = append_frame (animator);

  gdk_window_get_size (pixmap->pixmap, &width, &height);
  
  new_frame->image = gdk_image_get (pixmap->pixmap, 0, 0, width, height);
  
  if (pixmap->mask != NULL)
    new_frame->mask = gdk_bitmap_ref (pixmap->mask);
  else
    new_frame->mask = NULL;
  
  new_frame->x_offset = x_offset;
  new_frame->y_offset = y_offset;
  new_frame->interval = interval;

  return TRUE;
}

void
gnome_animator_start (GnomeAnimator *animator)
{
  g_return_if_fail (animator != NULL);

  if (animator->frames != NULL)
    {
      animator->status = GNOME_ANIMATOR_STATUS_RUNNING;

      /* This forces the animator to start playback.  */
      gnome_animator_advance (animator, 0);
    }
}

void
gnome_animator_stop (GnomeAnimator *animator)
{
  g_return_if_fail (animator != NULL);

  if (animator->status == GNOME_ANIMATOR_STATUS_RUNNING)
    gtk_timeout_remove (animator->timeout_id);
  animator->status = GNOME_ANIMATOR_STATUS_STOPPED;
}

/* Advance the animator `num' frames.  If `num' is positive, use the
   specified `playback_direction'; if it is negative, go in the
   opposite direction.  After the call, the animator is in the same
   state it would be if it had actually executed the specified number
   of iterations.  The return value is `TRUE' if the animator is now
   stopped.  */
gboolean
gnome_animator_advance (GnomeAnimator *animator, gint num)
{
  gboolean stop;
  guint new_frame;

  g_return_val_if_fail (animator != NULL, FALSE);

  if (num == 0)
    {
      stop = (animator->status == GNOME_ANIMATOR_STATUS_STOPPED);
      new_frame = animator->current_frame_number;
    }
  else
    {
      if (animator->playback_direction < 0)
        num = -num;

      if ((num < 0
           && animator->current_frame_number >= (guint)-num)
          || (num > 0
              && (guint) num < (animator->num_frames
                                - animator->current_frame_number)))
        {
          new_frame = animator->current_frame_number + num;
          stop = FALSE;
        }
      else
        /* We are overflowing the frame list: handle the various loop
           types.  */
        switch (animator->loop_type)
          {
          default:
          case GNOME_ANIMATOR_LOOP_NONE:
            if (num < 0)
              new_frame = 0;
            else
              new_frame = animator->num_frames - 1;
            if ((num < 0 && animator->playback_direction > 0)
                || (num > 0 && animator->playback_direction < 0))
              stop = FALSE;
            else
              stop = TRUE;
            break;
          case GNOME_ANIMATOR_LOOP_RESTART:
            if (num > 0)
              new_frame = ((num - (animator->num_frames
                                   - animator->current_frame_number))
                           % animator->num_frames);
            else
              new_frame = (animator->num_frames - 1
                           - ((-num - (animator->current_frame_number + 1))
                              % animator->num_frames));
            stop = FALSE;
            break;
          case GNOME_ANIMATOR_LOOP_PING_PONG:
            {
              guint num1;
              gboolean back;

              if (num > 0)
                num1 = num - (animator->num_frames - 1
                              - animator->current_frame_number);
              else
                num1 = -num - animator->current_frame_number;

              back = (((num1 / (animator->num_frames - 1)) % 2) == 0);
              if (num < 0)
                back = !back;

              if (back)
                {
                  new_frame = (animator->num_frames - 1
                               - (num1 % (animator->num_frames - 1)));
                  animator->playback_direction = -1;
                }
              else
                {
                  new_frame = num1 % (animator->num_frames - 1);
                  animator->playback_direction = +1;
                }
            }
            stop = FALSE;
            break;
          }
    }

  if (stop)
    gnome_animator_stop (animator);

  gnome_animator_goto_frame (animator, new_frame);

  return stop;
}

/* Jump to the specified `frame_number'.  This is also responsible for
   setting up the timeout callback for this frame.  */
void
gnome_animator_goto_frame (GnomeAnimator *animator, guint frame_number)
{
  g_return_if_fail (animator != NULL);
  g_return_if_fail (frame_number < animator->num_frames);

  animator->current_frame_number = frame_number;

  update (animator);

  if (animator->status == GNOME_ANIMATOR_STATUS_RUNNING)
    {
      guint32 interval;
      GnomeAnimatorFrame *frame;

      if (animator->timeout_id != 0)
        gtk_timeout_remove (animator->timeout_id);

      frame = animator->frames + animator->current_frame_number;
      interval = (guint32) ((double) frame->interval
                            / animator->playback_speed + .5);

      animator->timeout_id = gtk_timeout_add (interval,
                                              timer_cb,
                                              (gpointer) animator);
    }
}

guint
gnome_animator_get_current_frame_number (GnomeAnimator *animator)
{
  g_return_val_if_fail (animator != NULL, 0);

  return animator->current_frame_number;
}

GnomeAnimatorStatus
gnome_animator_get_status (GnomeAnimator *animator)
{
  g_return_val_if_fail (animator != NULL, GNOME_ANIMATOR_STATUS_STOPPED);

  return animator->status;
}

void
gnome_animator_set_playback_speed (GnomeAnimator *animator, double speed)
{
  g_return_if_fail (animator != NULL);
  g_return_if_fail (speed > 0.0);

  animator->playback_speed = speed;
}

double
gnome_animator_get_playback_speed (GnomeAnimator *animator)
{
  g_return_val_if_fail (animator != NULL, -1.0);

  return animator->playback_speed;
}

