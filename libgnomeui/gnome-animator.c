/* GNOME GUI Library
 * Copyright (C) 1998, 1999 the Free Software Foundation
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA. 
 */

/* This is a simple widget for adding animations to GNOME
   applications.  It is updated using double buffering for maximum
   smoothness.  */

#include <config.h>
#include <gtk/gtkmain.h>

#include "gnome-animator.h"

/* ------------------------------------------------------------------------- */

/* This is kept private as things might change in the future.  */
struct _GnomeAnimatorFrame
  {
    /* Rendered pixmap.  */
    GdkPixmap *pixmap;

    /* Shape mask (NULL if no shape mask).  */
    GdkBitmap *mask;

    /* Size.  */
    guint width;
    guint height;

    /* X/Y position in the widget.  */
    gint x_offset;
    gint y_offset;

    /* Interval for the next frame (as for `gtk_timeout_add()').  This
       value is divided by `playback_speed' before being used.  */
    guint32 interval;

    /* Pointer to the next and previous frames.  */
    GnomeAnimatorFrame *prev, *next;
  };

struct _GnomeAnimatorPrivate
  {
    /* Timeout callback ID used for advancing playback.  */
    guint timeout_id;

    /* Rectangle being used for the animation.  */
    GdkRectangle area;

    /* Pixmap used for drawing offscreen.  */
    GdkPixmap *offscreen_pixmap;

    /* Pixmap storing the window's background.  This is kept in sync
       with the parent's window background.  */
    GdkPixmap *background_pixmap;

    /* Pointer to the first frame.  */
    GnomeAnimatorFrame *first_frame;

    /* Pointer to the last frame.  */
    GnomeAnimatorFrame *last_frame;

    /* Pointer to the current frame.  */
    GnomeAnimatorFrame *current_frame;
  };

/* ------------------------------------------------------------------------- */

static void                class_init              (GnomeAnimatorClass *class);
static void                init                    (GnomeAnimator *animator);
static void                destroy                 (GtkObject *object);
static void                prepare_aux_pixmaps     (GnomeAnimator *animator);
static void                draw_background_pixmap  (GnomeAnimator *animator);
static void                size_allocate           (GtkWidget *widget,
                                                    GtkAllocation *allocation);
static void                draw                    (GtkWidget *widget,
                                                    GdkRectangle *area);
static gint                expose                  (GtkWidget *widget,
                                                    GdkEventExpose *event);
static void                update                  (GnomeAnimator *animator);
static GnomeAnimatorFrame *append_frame            (GnomeAnimator *animator);
static void                state_or_style_changed  (GnomeAnimator *animator);
static void                state_changed           (GtkWidget *widget,
                                                    GtkStateType previous_state);
static void                style_set               (GtkWidget *widget,
                                                    GtkStyle *previous_style);
static gint                timer_cb                (gpointer data);

static GtkWidgetClass *parent_class;

/* ------------------------------------------------------------------------- */

static void
init (GnomeAnimator *animator)
{
  GTK_WIDGET_SET_FLAGS (animator, GTK_NO_WINDOW);

  animator->num_frames = 0;
  animator->current_frame_number = 0;
  animator->status = GNOME_ANIMATOR_STATUS_STOPPED;
  animator->loop_type = GNOME_ANIMATOR_LOOP_NONE;
  animator->playback_direction = +1;
  animator->playback_speed = 1.0;

  animator->privat = g_new (GnomeAnimatorPrivate, 1);
  animator->privat->timeout_id = 0;
  animator->privat->offscreen_pixmap = NULL;
  animator->privat->background_pixmap = NULL;
  animator->privat->first_frame = NULL;
  animator->privat->last_frame = NULL;
  animator->privat->current_frame = NULL;
}

static void
class_init (GnomeAnimatorClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;

  parent_class = gtk_type_class (gtk_widget_get_type ());

  widget_class->size_allocate = size_allocate;
  widget_class->draw = draw;
  widget_class->expose_event = expose;
  widget_class->style_set = style_set;
  widget_class->state_changed = state_changed;

  object_class->destroy = destroy;
}

static void
free_all_frames (GnomeAnimator *animator)
{
  GnomeAnimatorFrame *p = animator->privat->first_frame;

  while (p != NULL)
    {
      GnomeAnimatorFrame *pnext;

      if (p->pixmap != NULL)
        gdk_imlib_free_pixmap (p->pixmap);

      pnext = p->next;
      g_free (p);
      p = pnext;
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
          && widget->requisition.height > 0
          && GTK_WIDGET_REALIZED(widget))
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
  GtkWidget *widget;

  widget = GTK_WIDGET (animator);
  if (privat->background_pixmap != NULL && widget->window != NULL)
    {
      GtkStyle *style;
      guint state;
      GdkGC *gc;

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

      gdk_gc_set_ts_origin (gc,
                            -animator->privat->area.x,
                            -animator->privat->area.y);

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
size_allocate (GtkWidget *widget, GtkAllocation *allocation)
{
  GnomeAnimator *animator;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_ANIMATOR (widget));
  g_return_if_fail (allocation != NULL);

  animator = GNOME_ANIMATOR (widget);

  if (GTK_WIDGET_REALIZED(widget))
          gdk_window_clear_area (widget->window,
                                 widget->allocation.x, widget->allocation.y,
                                 widget->allocation.width, widget->allocation.height);

  widget->allocation = *allocation;

  if (GTK_WIDGET_REALIZED(widget))
          gdk_window_clear_area (widget->window,
                                 widget->allocation.x, widget->allocation.y,
                                 widget->allocation.width, widget->allocation.height);

  animator->privat->area.x = (widget->allocation.x
                              + (widget->allocation.width
                                 - widget->requisition.width) / 2);
  animator->privat->area.y = (widget->allocation.y
                              + (widget->allocation.height
                                 - widget->requisition.height) / 2);
  animator->privat->area.width = widget->requisition.width;
  animator->privat->area.height = widget->requisition.height;

  prepare_aux_pixmaps (animator);
  draw_background_pixmap (animator);

  update (animator);
}

static void
paint (GnomeAnimator *animator, GdkRectangle *area)
{
  /* FIXME this is a bad hack, because I don't want to introduce
     a new realize()/unrealize() pair in the stable libs right now
     and risk screwing things up
  */
  if (animator->privat->offscreen_pixmap == NULL &&
      GTK_WIDGET_REALIZED(GTK_WIDGET(animator)))
    {
      prepare_aux_pixmaps (animator);
      draw_background_pixmap (animator);
    }
        
  if (animator->num_frames > 0
      && animator->privat->offscreen_pixmap != NULL)
    {
      GtkWidget *widget;
      GnomeAnimatorPrivate *privat;
      GnomeAnimatorFrame *frame;

      widget = GTK_WIDGET (animator);
      privat = animator->privat;
      frame = privat->current_frame;

      /* Update the window using double buffering to make the
         animation as smooth as possible.  */

      /* Copy the background into the offscreen pixmap.  */
      gdk_draw_pixmap (privat->offscreen_pixmap,
                       widget->style->black_gc,
                       privat->background_pixmap,
                       area->x - privat->area.x, area->y - privat->area.y,
                       area->x - privat->area.x, area->y - privat->area.y,
                       area->width,
                       area->height);

      /* Draw the (shaped) frame into the offscreen pixmap.  */
      gdk_gc_set_clip_mask (widget->style->black_gc, frame->mask);
      gdk_gc_set_clip_origin (widget->style->black_gc,
                              frame->x_offset, frame->y_offset);
      gdk_window_copy_area (privat->offscreen_pixmap,
                            widget->style->black_gc,
                            frame->x_offset, frame->y_offset,
                            frame->pixmap,
                            0, 0,
                            frame->width, frame->height);
      gdk_gc_set_clip_mask (widget->style->black_gc, NULL);

      /* Copy the offscreen pixmap into the window.  */
      gdk_draw_pixmap (widget->window,
                       widget->style->black_gc,
                       privat->offscreen_pixmap,
                       area->x - privat->area.x, area->y - privat->area.y,
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

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_ANIMATOR (widget));
  g_return_if_fail (area != NULL);

  if (GTK_WIDGET_DRAWABLE (widget))
    {
      GdkRectangle p_area;

      animator = GNOME_ANIMATOR (widget);

      if (gdk_rectangle_intersect (area, &animator->privat->area, &p_area))
          paint (animator, &p_area);

      gdk_flush ();
    }
}

static gint
expose (GtkWidget *widget, GdkEventExpose *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GNOME_IS_ANIMATOR (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (GTK_WIDGET_REALIZED (widget))
    draw (widget, &event->area);

  return FALSE;
}

static void
update (GnomeAnimator *animator)
{
  GtkWidget *widget = GTK_WIDGET (animator);

  if (GTK_WIDGET_REALIZED (widget))
    {
      if (animator->num_frames > 0)
        gtk_widget_queue_draw (widget);
      else
        gdk_window_clear_area (widget->window,
                               widget->allocation.x,
                               widget->allocation.y,
                               widget->allocation.width,
                               widget->allocation.height);
    }
}

static GnomeAnimatorFrame *
append_frame (GnomeAnimator *animator)
{
  GnomeAnimatorPrivate *privat = animator->privat;
  GnomeAnimatorFrame *new_frame;

  new_frame = g_new (GnomeAnimatorFrame, 1);

  if (privat->first_frame == NULL)
    {
      privat->first_frame = privat->last_frame = privat->current_frame
        = new_frame;
      new_frame->prev = NULL;
    }
  else
    {
      privat->last_frame->next = new_frame;
      new_frame->prev = privat->last_frame;
      privat->last_frame = new_frame;
    }

  new_frame->next = 0;
  animator->num_frames++;

  return new_frame;
}

static void
state_or_style_changed (GnomeAnimator *animator)
{
  prepare_aux_pixmaps (animator);
  draw_background_pixmap (animator);
}

static void
state_changed (GtkWidget *widget, GtkStateType previous_state)
{
  g_assert (GNOME_IS_ANIMATOR (widget));
  state_or_style_changed (GNOME_ANIMATOR (widget));
}

static void
style_set (GtkWidget *widget, GtkStyle *previous_style)
{
  g_assert (GNOME_IS_ANIMATOR (widget));
  state_or_style_changed (GNOME_ANIMATOR (widget));
}

/* ------------------------------------------------------------------------- */

static gint
timer_cb (gpointer data)
{
  GnomeAnimator *animator;

  GDK_THREADS_ENTER ();

  animator = GNOME_ANIMATOR (data);
  if (animator->status != GNOME_ANIMATOR_STATUS_STOPPED)
    gnome_animator_advance (animator, +1);

  GDK_THREADS_LEAVE ();

  return FALSE;
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

/**
 * gnome_animator_new_with_size
 * @width: pixel width of animator widget
 * @height: pixel height of animator widget
 *
 * Description: Creates a new animator widget of the specified size.
 *
 * Returns: Pointer to new animator widget.
 **/

GtkWidget *
gnome_animator_new_with_size (guint width, guint height)
{
  GnomeAnimator *animator;
  GtkWidget *widget;

  gtk_widget_push_visual (gdk_imlib_get_visual ());
  gtk_widget_push_colormap (gdk_imlib_get_colormap ());
  animator = gtk_type_new (gnome_animator_get_type ());
  gtk_widget_pop_colormap ();
  gtk_widget_pop_visual ();
  
  widget = GTK_WIDGET (animator);

  widget->requisition.width = width;
  widget->requisition.height = height;

  if (GTK_WIDGET_VISIBLE (widget))
    gtk_widget_queue_resize (widget);

  return widget;
}

/**
 * gnome_animator_set_loop_type
 * @animator: Animator widget to be updated
 * @loop_type: Type of animation loop desired
 *
 * Description: Sets desired animation loop type.  Available loop
 * types are %GNOME_ANIMATOR_LOOP_NONE (play animation once only),
 * %GNOME_ANIMATOR_LOOP_RESTART (play animation over and over again),
 * and %GNOME_ANIMATOR_LOOP_PING_PONG (play animation over and over
 * again, reversing the playing direction every time.)
 **/

void
gnome_animator_set_loop_type (GnomeAnimator *animator,
                              GnomeAnimatorLoopType loop_type)
{
  g_return_if_fail (animator != NULL);
  g_return_if_fail (GNOME_IS_ANIMATOR (animator));

  animator->loop_type = loop_type;
}

/**
 * gnome_animator_get_loop_type
 * @animator: Animator widget to be queried
 *
 * Description: Obtains current animator loop type.  Available loop
 * types are %GNOME_ANIMATOR_LOOP_NONE (play animation once only),
 * %GNOME_ANIMATOR_LOOP_RESTART (play animation over and over again),
 * and %GNOME_ANIMATOR_LOOP_PING_PONG (play animation over and over
 * again, reversing the playing direction every time.)
 *
 * Returns: Loop type.
 **/

GnomeAnimatorLoopType
gnome_animator_get_loop_type (GnomeAnimator *animator)
{
  g_return_val_if_fail (animator != NULL, GNOME_ANIMATOR_LOOP_NONE);

  return animator->loop_type;
}

/**
 * gnome_animator_set_playback_direction
 * @animator: Animator widget to be updated
 * @playback_direction: Direction animation should be played.
 *
 * Description: Sets direction (forwards or backwards) to play the
 * animation.  If @playback_direction is a positive number, the
 * animation is played from the first to the last frame.  If
 * @playback_direction is negative, the animation is played from the
 * last to the first frame.
 **/

void
gnome_animator_set_playback_direction (GnomeAnimator *animator,
                                       gint playback_direction)
{
  g_return_if_fail (animator != NULL);

  animator->playback_direction = playback_direction;
}

/**
 * gnome_animator_get_playback_direction
 * @animator: Animator widget to be updated
 *
 * Description: Returns the current playing direction (forwards or
 * backwards) for the animation.  If the returned value is a positive
 * number, the animation is played from the first to the last frame.
 * If it is negative, the animation is played from the last to the
 * first frame.
 *
 * Returns: Positive or negative number indicating direction.
 **/

gint
gnome_animator_get_playback_direction (GnomeAnimator *animator)
{
  g_return_val_if_fail (animator != NULL, 0);

  return animator->playback_direction;
}

/**
 * gnome_animator_append_frame_from_imlib_at_size
 * @animator: Animator widget to be updated
 * @image: Image to be added to animator
 * @x_offset: horizontal offset of frame within animator widget
 * @y_offset: vertical offset of frame within animator widget
 * @interval: Number of milliseconds to delay before showing next frame
 * @width: pixel width of frame
 * @height: pixel height of frame
 *
 * Description: Adds frame contained within a &GdkImlibImage image at
 * the end of the current animation.  If @width and @height are
 * different from the actual @image size, the image is scaled
 * proportionally.  The frame display interval is @interval is divided
 * by the playback speed.
 *
 * Returns: %TRUE if append succeeded.
 **/

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

  g_return_val_if_fail (animator != NULL, FALSE);
  g_return_val_if_fail (image != NULL, FALSE);

  if (width == 0)
    width = image->rgb_width;
  if (height == 0)
    height = image->rgb_height;

  new_frame = append_frame (animator);

  gdk_imlib_render (image, width, height);

  new_frame->pixmap = gdk_imlib_move_image (image);
  new_frame->mask = gdk_imlib_move_mask (image);

  new_frame->width = width;
  new_frame->height = height;

  new_frame->x_offset = x_offset;
  new_frame->y_offset = y_offset;

  new_frame->interval = interval;

  return TRUE;
}

/**
 * gnome_animator_append_frame_from_imlib
 * @animator: Animator widget to be updated
 * @image: Image to be added to animator
 * @x_offset: horizontal offset of frame within animator widget
 * @y_offset: vertical offset of frame within animator widget
 * @interval: Number of milliseconds to delay before showing next frame
 *
 * Description: Adds frame contained within a &GdkImlibImage image at
 * the end of the current animation.  The frame display interval is
 * @interval is divided by the playback speed.
 *
 * Returns: %TRUE if append succeeded.
 **/

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

/**
 * gnome_animator_append_frame_from_file_at_size
 * @animator: Animator widget to be updated
 * @name: File path of image to be added to animator
 * @x_offset: horizontal offset of frame within animator widget
 * @y_offset: vertical offset of frame within animator widget
 * @interval: Number of milliseconds to delay before showing next frame
 * @width: pixel width of frame
 * @height: pixel height of frame
 *
 * Description: Adds frame from the given file to the end of the
 * current animation.  If @width and @height are different from the
 * actual @image size, the image is scaled proportionally.  The frame
 * display interval is @interval divided by the playback speed.
 *
 * Returns: %TRUE if append succeeded.
 **/

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

/**
 * gnome_animator_append_frame_from_file
 * @animator: Animator widget to be updated
 * @name: File path of image to be added to animator
 * @x_offset: horizontal offset of frame within animator widget
 * @y_offset: vertical offset of frame within animator widget
 * @interval: Number of milliseconds to delay before showing next frame
 *
 * Description: Adds frame from the given file to the end of the
 * current animation.  The frame display interval is @interval divided
 * by the playback speed.
 *
 * Returns: %TRUE if append succeeded.
 **/

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

/**
 * gnome_animator_append_frames_from_imlib_at_size
 * @animator: Animator widget to be updated
 * @image: Image containing frames to be added to animator
 * @x_offset: horizontal offset of a frame within animator widget
 * @y_offset: vertical offset of a frame within animator widget
 * @interval: Number of milliseconds to delay before showing next frame
 * @x_unit: pixel width of a single frame
 * @width: pixel width of frame
 * @height: pixel height of frame
 *
 * Description: Adds multiple frames contained within a &GdkImlibImage
 * image at the end of the current animation.  Each frame within the
 * image should be next to one another in a single, horizontal row.
 * If @width and @height are different from the actual frame size, the
 * image is scaled proportionally.  The frame display interval is
 * @interval divided by the playback speed.
 *
 * Returns: %TRUE if append succeeded.
 **/

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
  GtkWidget *widget = GTK_WIDGET (animator);
  GdkPixmap *tmp_pixmap;
  GdkBitmap *tmp_mask;
  GdkGC *pixmap_gc, *mask_gc;
  GdkGCValues gc_values;
  guint num_frames;
  guint render_width;
  guint rest;
  guint offs;
  guint i;
  guint depth;

  rest = image->rgb_width % x_unit;
  g_return_val_if_fail (rest == 0, FALSE);

  if (width == 0)
    width = x_unit;
  if (height == 0)
    height = image->rgb_height;

  num_frames = image->rgb_width / x_unit;
  render_width = width * num_frames;

  gdk_imlib_render (image, render_width, height);

  tmp_pixmap = gdk_imlib_move_image (image);
  tmp_mask = gdk_imlib_move_mask (image);

  depth = gtk_widget_get_visual (widget) -> depth;

  gc_values.function = GDK_COPY;

  pixmap_gc = gdk_gc_new_with_values (tmp_pixmap, &gc_values, GDK_GC_FUNCTION);

  if (tmp_mask != NULL)
    mask_gc = gdk_gc_new_with_values (tmp_mask, &gc_values, GDK_GC_FUNCTION);
  else
    mask_gc = NULL;             /* Make compiler happy.  */

  for (i = offs = 0; i < num_frames; i++, offs += width)
    {
      GnomeAnimatorFrame *new_frame;

      new_frame = append_frame (animator);

      new_frame->pixmap = gdk_pixmap_new (tmp_pixmap, width, height, depth);
      gdk_window_copy_area (new_frame->pixmap,
                            pixmap_gc,
                            0, 0,
                            tmp_pixmap,
                            offs, 0,
                            width, height);

      if (tmp_mask != NULL)
        {
          new_frame->mask = gdk_pixmap_new (tmp_pixmap, width, height, 1);
          gdk_window_copy_area (new_frame->mask,
                                mask_gc,
                                0, 0,
                                tmp_mask,
                                offs, 0,
                                width, height);
        }
      else
        new_frame->mask = NULL;

      new_frame->width = width;
      new_frame->height = height;

      new_frame->x_offset = x_offset;
      new_frame->y_offset = y_offset;

      new_frame->interval = interval;
    }

  gdk_gc_unref (pixmap_gc);
  gdk_imlib_free_pixmap (tmp_pixmap);

  if (tmp_mask != NULL)
    {
      /* Imlib already freed the mask, so don't free it again */
      gdk_gc_unref (mask_gc);
    }

  return TRUE;
}

/**
 * gnome_animator_append_frames_from_imlib
 * @animator: Animator widget to be updated
 * @image: Image containing frames to be added to animator
 * @x_offset: horizontal offset of a frame within animator widget
 * @y_offset: vertical offset of a frame within animator widget
 * @interval: Number of milliseconds to delay before showing next frame
 * @x_unit: pixel width of a single frame
 *
 * Description: Adds multiple frames contained within a &GdkImlibImage
 * image to the end of the current animation.  Each frame within the
 * image should be next to one another in a single, horizontal row.
 * The frame display interval is @interval divided by the
 * playbackspeed.
 *
 * Returns: %TRUE if append succeeded.
 **/

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

/**
 * gnome_animator_append_frames_from_file_at_size
 * @animator: Animator widget to be updated
 * @name: File path to image containing frames to be added to animator
 * @x_offset: horizontal offset of a frame within animator widget
 * @y_offset: vertical offset of a frame within animator widget
 * @interval: Number of milliseconds to delay before showing next frame
 * @x_unit: pixel width of a single frame
 * @width: pixel width of frame
 * @height: pixel height of frame
 *
 * Description: Adds multiple frames contained within a single image
 * file to the end of the current animation.  Each frame within the
 * image should be next to one another in a single, horizontal row.
 * If @width and @height are different from the actual frame size, the
 * image is scaled proportionally.  The frame display interval is
 * @interval divided by the playback speed.
 *
 * Returns: %TRUE if append succeeded.
 **/

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

/**
 * gnome_animator_append_frames_from_file
 * @animator: Animator widget to be updated
 * @name: File path to image containing frames to be added to animator
 * @x_offset: horizontal offset of a frame within animator widget
 * @y_offset: vertical offset of a frame within animator widget
 * @interval: Number of milliseconds to delay before showing next frame
 * @x_unit: pixel width of a single frame
 *
 * Description:  Adds multiple frames contained within a single image file
 * to the end of the current animation.  Each frame within the image
 * should be next to one another in a single, horizontal row.
 * The frame display interval is @interval divided by the playback_speed.
 *
 * Returns: %TRUE if append succeeded.
 **/

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

/**
 * gnome_animator_append_frame_from_gnome_pixmap
 * @animator: Animator widget to be updated
 * @pixmap: GNOME pixmap to be added to animator
 * @x_offset: horizontal offset of frame within animator widget
 * @y_offset: vertical offset of frame within animator widget
 * @interval: Number of milliseconds to delay before showing next frame
 *
 * Description: Adds frame contained within a &GnomePixmap image to
 * the end of the current animation.  The frame display interval is
 * @interval divided by the playback speed.
 *
 * Returns: %TRUE if append succeeded.
 **/

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

  new_frame->pixmap = gdk_pixmap_ref (pixmap->pixmap);
  gdk_window_get_size (new_frame->pixmap, &width, &height);

  if (pixmap->mask != NULL)
    new_frame->mask = gdk_bitmap_ref (pixmap->mask);
  else
    new_frame->mask = NULL;

  new_frame->x_offset = x_offset;
  new_frame->y_offset = y_offset;

  new_frame->width = width;
  new_frame->height = height;

  new_frame->interval = interval;

  return TRUE;
}

/**
 * gnome_animator_start
 * @animator: Animator widget to be started
 *
 * Description:  Initiate display of animated frames.
 **/

void
gnome_animator_start (GnomeAnimator *animator)
{
  g_return_if_fail (animator != NULL);

  if (animator->num_frames > 0)
    {
      animator->status = GNOME_ANIMATOR_STATUS_RUNNING;

      /* This forces the animator to start playback.  */
      gnome_animator_advance (animator, 0);
    }
}

/**
 * gnome_animator_stop
 * @animator: Animator widget to be stopped
 *
 * Description:  Halts display of animated frames.  The current frame
 * in the animation will remain in the animator widget.
 **/

void
gnome_animator_stop (GnomeAnimator *animator)
{
  g_return_if_fail (animator != NULL);

  if (animator->status == GNOME_ANIMATOR_STATUS_RUNNING)
    gtk_timeout_remove (animator->privat->timeout_id);
  animator->status = GNOME_ANIMATOR_STATUS_STOPPED;
}

/**
 * gnome_animator_advance
 * @animator: Animator widget to be updated
 * @num: Number of frames to advance by
 *
 * Description: Advance the animator @num frames.  If @num is
 * positive, use the specified @playback_direction; if it is negative,
 * go in the opposite direction.  After the call, the animator is in
 * the same state it would be if it had actually executed the specified
 * number of iterations. 
 *
 * Returns:  %TRUE if the animator is now stopped.
 **/

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
          default:
            g_warning ("Unknown GnomeAnimatorLoopType %d",
		       animator->loop_type);
            stop = TRUE;
            new_frame = animator->current_frame_number;
            break;
          }
    }

  if (stop)
    gnome_animator_stop (animator);

  gnome_animator_goto_frame (animator, new_frame);

  return stop;
}

/**
 * gnome_animator_goto_frame
 * @animator: Animator widget to be updated
 * @frame_number: Frame number to be made current
 *
 * Description: Jump to the specified @frame_number and display it.
 **/

void
gnome_animator_goto_frame (GnomeAnimator *animator, guint frame_number)
{
  GnomeAnimatorPrivate *privat;
  GnomeAnimatorFrame *frame;
  gint dist1;
  guint dist2;
  guint i;

  g_return_if_fail (animator != NULL);
  g_return_if_fail (frame_number < animator->num_frames);

  privat = animator->privat;

  /* Try to be smart and minimize the number of steps spent walking on
     the linked list.  */

  dist1 = frame_number - animator->current_frame_number;
  dist2 = animator->num_frames - 1 - frame_number;

  if (frame_number < (guint) ABS (dist1) && frame_number < dist2)
    {
      frame = privat->first_frame;
      for (i = 0; i < frame_number; i++)
        frame = frame->next;
    }
  else if (dist2 < (guint) ABS (dist1))
    {
      frame = privat->last_frame;
      for (i = 0; i < dist2; i++)
        frame = frame->prev;
    }
  else
    {
      frame = privat->current_frame;
      if (dist1 > 0)
        for (i = 0; i < (guint) dist1; i++)
          frame = frame->next;
      else
        for (i = 0; i < (guint) -dist1; i++)
          frame = frame->prev;
    }

  animator->current_frame_number = frame_number;
  privat->current_frame = frame;

  update (animator);

  if (animator->status == GNOME_ANIMATOR_STATUS_RUNNING)
    {
      guint32 interval;

      if (privat->timeout_id != 0)
        gtk_timeout_remove (privat->timeout_id);

      interval = (guint32) ((double) frame->interval
                            / animator->playback_speed + .5);

      privat->timeout_id = gtk_timeout_add (interval,
                                            timer_cb,
                                            (gpointer) animator);
    }
}

/**
 * gnome_animator_get_current_frame_number
 * @animator: Animator widget to be queried
 *
 * Description:  Obtains current frame number from animator widget.
 *
 * Returns: Current frame number.
 **/

guint
gnome_animator_get_current_frame_number (GnomeAnimator *animator)
{
  g_return_val_if_fail (animator != NULL, 0);

  return animator->current_frame_number;
}

/**
 * gnome_animator_get_status
 * @animator: Animator widget to be queried
 *
 * Description:  Obtains current status from animator widget.  Possible
 * return values include %GNOME_ANIMATOR_STATUS_STOPPED and
 * %GNOME_ANIMATOR_STATUS_RUNNING.
 *
 * Returns: Status constant.
 **/

GnomeAnimatorStatus
gnome_animator_get_status (GnomeAnimator *animator)
{
  g_return_val_if_fail (animator != NULL, GNOME_ANIMATOR_STATUS_STOPPED);

  return animator->status;
}

/**
 * gnome_animator_set_playback_speed
 * @animator: Animator widget to be updated
 * @speed: Rate multiplier for playback speed
 *
 * Description: Sets the playback speed.  The delay between every
 * frame is divided by this value before being used.  As a
 * consequence, higher values give higher playback speeds.
 **/

void
gnome_animator_set_playback_speed (GnomeAnimator *animator, double speed)
{
  g_return_if_fail (animator != NULL);
  g_return_if_fail (speed > 0.0);

  animator->playback_speed = speed;
}

/**
 * gnome_animator_get_playback_speed
 * @animator: Animator widget to be queried
 *
 * Description: Returns the current playback speed.
 *
 * Returns: &double indicating the playback speed.
 **/

double
gnome_animator_get_playback_speed (GnomeAnimator *animator)
{
  g_return_val_if_fail (animator != NULL, -1.0);

  return animator->playback_speed;
}

