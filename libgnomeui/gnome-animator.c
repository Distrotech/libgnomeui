/* GNOME GUI Library
 * Copyright (C) 1998, 1999 the Free Software Foundation
 *
 * Authors: Cody Russell  (bratsche@dfw.net)
 *          Ettore Perazzoli (ettore@comm2000.it)
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

#include <config.h>
#include <gtk/gtkmain.h>
#include "gnome-animator.h"

struct _GnomeAnimatorPrivate
{
  GdkRectangle area;
  guint timeout_id;

  GdkPixmap *offscreen_pixmap;
  GdkPixbuf *background_pixbuf;

  GnomeAnimatorFrame *first_frame;
  GnomeAnimatorFrame *last_frame;
  GnomeAnimatorFrame *current_frame;
};

/*
 * enum GdkPixbufFrameAction:
 *    GDK_PIXBUF_FRAME_RETAIN
 *    GDK_PIXBUF_FRAME_DISPOSE
 *    GDK_PIXBUF_FRAME_REVERT
 *
 * struct GdkPixbufFrame:
 *    GdkPixbuf *pixbuf
 *    int x_offset;
 *    int y_offset;
 *    int delay_time;
 *    GdkPixbufFrameAction action;
 */

struct _GnomeAnimatorFrame
{
  int frame_num;
  GdkPixbufFrame *frame;
  GdkBitmap *mask;
  guint32 interval;
  GnomeAnimatorFrame *next;
  GnomeAnimatorFrame *prev;
};

static void class_init (GnomeAnimatorClass * class);
static void init (GnomeAnimator * animator);
static void destroy (GtkObject * object);
static void prepare_aux_pixmaps (GnomeAnimator * animator);
static void draw_background_pixbuf (GnomeAnimator * animator);
static void draw (GtkWidget * widget, GdkRectangle * area);
static gint expose (GtkWidget * widget, GdkEventExpose * event);
static void size_allocate (GtkWidget * widget, GtkAllocation * allocation);
static void update (GnomeAnimator * animator);
static void state_or_style_changed (GnomeAnimator * animator);
static void state_changed (GtkWidget * widget, GtkStateType previous_state);
static void style_set (GtkWidget * widget, GtkStyle * previous_style);
static GnomeAnimatorFrame *append_frame (GnomeAnimator * animator);
static gint timer_cb (gpointer data);

static GtkWidgetClass *parent_class;

static void
init (GnomeAnimator * animator)
{
  GTK_WIDGET_SET_FLAGS (animator, GTK_NO_WINDOW);
  animator->frame_num = 0;
  animator->status = GNOME_ANIMATOR_STATUS_STOPPED;
  animator->loop_type = GNOME_ANIMATOR_LOOP_RESTART;
  animator->playback_direction = GNOME_ANIMATOR_DIRECTION_FORWARD;

  animator->internal = g_new0 (GnomeAnimatorPrivate, 1);
}

static void
class_init (GnomeAnimatorClass * class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;

  parent_class = gtk_type_class (gtk_misc_get_type ());

  widget_class->draw = draw;
  widget_class->expose_event = expose;
  widget_class->size_allocate = size_allocate;

  object_class->destroy = destroy;
}

static void
destroy (GtkObject * object)
{
  g_return_if_fail (object != NULL);
  g_return_if_fail (GNOME_IS_ANIMATOR (object));

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (*GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static void
size_allocate (GtkWidget * widget, GtkAllocation * allocation)
{
  GnomeAnimator *animator;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_ANIMATOR (widget));
  g_return_if_fail (allocation != NULL);

  animator = GNOME_ANIMATOR (widget);

  if (GTK_WIDGET_REALIZED (widget))
    {
      g_print ("size_allocate(): realized!\n");
      gdk_window_clear_area (widget->window,
			     widget->allocation.x, widget->allocation.y,
			     widget->allocation.width,
			     widget->allocation.height);
    }

  widget->allocation = *allocation;
  g_print ("size_allocate(): x %d  y %d  width %d  height %d\n",
	   allocation->x, allocation->y, allocation->width,
	   allocation->height);

  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_clear_area (widget->window,
			   widget->allocation.x, widget->allocation.y,
			   widget->allocation.width,
			   widget->allocation.height);

  animator->internal->area.x = widget->allocation.x +
    (widget->allocation.width - widget->requisition.width) / 2;
  animator->internal->area.y = widget->allocation.y +
    (widget->allocation.height - widget->requisition.height) / 2;
  animator->internal->area.width = widget->requisition.width;
  animator->internal->area.height = widget->requisition.height;

  prepare_aux_pixmaps (animator);
  draw_background_pixbuf (animator);

  update (animator);
}


/* Create the offscreen and background pixmaps if they do not exist,
   or resize them in case the widget's size has changed.  */
static void
prepare_aux_pixmaps (GnomeAnimator * animator)
{
  GtkWidget *widget = GTK_WIDGET (animator);
  GnomeAnimatorPrivate *internal = animator->internal;
  gint old_width, old_height;

  if (internal->offscreen_pixmap != NULL)
    gdk_window_get_size (internal->offscreen_pixmap, &old_width, &old_height);
  else
    old_width = old_height = 0;

  if ((widget->requisition.width != (guint16) old_width)
      && (widget->requisition.height != (guint16) old_height))
    {
      GdkVisual *visual = gtk_widget_get_visual (widget);

      if (internal->offscreen_pixmap != NULL)
	{
	  gdk_pixmap_unref (internal->offscreen_pixmap);
	  internal->offscreen_pixmap = NULL;
	}

      if (internal->background_pixbuf != NULL)
	{
	  gdk_pixbuf_unref (internal->background_pixbuf);
	  internal->background_pixbuf = NULL;
	}

      if (widget->requisition.width > 0 &&
	  widget->requisition.height > 0 && GTK_WIDGET_REALIZED (widget))
	{
	  internal->offscreen_pixmap = gdk_pixmap_new (widget->window,
						       widget->requisition.
						       width,
						       widget->requisition.
						       height, visual->depth);

	  internal->background_pixbuf = gdk_pixbuf_new (ART_PIX_RGB,
							FALSE,
							8,
							widget->requisition.
							width,
							widget->requisition.
							height);
	}
    }
}

/*
 * Clear background_pixbuf then draw the next frame into it.
 */
static void
draw_background_pixbuf (GnomeAnimator * animator)
{
  GnomeAnimatorPrivate *internal = animator->internal;
  GtkWidget *widget;
  GdkPixmap *pixmap;

  widget = GTK_WIDGET (animator);

  if (internal->background_pixbuf != NULL && widget->window != NULL)
    {
      GdkVisual *visual;
      GdkColormap *colormap;
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
			    -animator->internal->area.x,
			    -animator->internal->area.y);

      visual = gtk_widget_get_visual (widget);

      pixmap = gdk_pixmap_new (widget->window,
			       widget->requisition.width,
			       widget->requisition.height, visual->depth);
      colormap = gdk_rgb_get_cmap ();

      gdk_draw_rectangle (pixmap,
			  gc,
			  TRUE,
			  0, 0,
			  widget->requisition.width,
			  widget->requisition.height);

      animator->internal->background_pixbuf =
	gdk_pixbuf_get_from_drawable (animator->internal->background_pixbuf,
				      pixmap,
				      colormap,
				      0, 0,
				      0, 0,
				      widget->requisition.width,
				      widget->requisition.height);

      gdk_gc_destroy (gc);
    }
}

/*
 * Call draw_background_pixbuf(), then draw background_pixbuf onto
 * the window.
 */
static void
paint (GnomeAnimator * animator, GdkRectangle * area)
{
  GnomeAnimatorPrivate *internal;
  GdkPixbuf *draw_source;
  GtkMisc *misc;
  GtkWidget *widget;
  gint top_clip, bottom_clip, left_clip, right_clip;
  gint width, height;
  gint x_off, y_off;

  g_return_if_fail (GTK_WIDGET_DRAWABLE (animator));

  widget = GTK_WIDGET (animator);
  misc = GTK_MISC (animator);
  internal = animator->internal;

  /* FIXME this is a bad hack, because I don't want to introduce
   * a new realize()/unrealize() pair in the stable libs right now
   * and risk screwing things up
   */
  if (internal->offscreen_pixmap == NULL &&
      GTK_WIDGET_REALIZED (GTK_WIDGET (animator)))
    {
      prepare_aux_pixmaps (animator);
      draw_background_pixbuf (animator);
    }

  draw_source = animator->internal->current_frame->frame->pixbuf;

  x_off = animator->internal->current_frame->frame->x_offset;
  y_off = animator->internal->current_frame->frame->y_offset;

  width = gdk_pixbuf_get_width (draw_source);
  height = gdk_pixbuf_get_height (draw_source);

  left_clip = (x_off < area->x) ? area->x - x_off : 0;
  top_clip = (y_off < area->y) ? area->y - y_off : 0;
  if (x_off + width > area->x + area->width)
    right_clip = x_off + width - (area->x + area->width);
  else
    right_clip = 0;
  if (y_off + height > area->y + area->height)
    bottom_clip = y_off + height - (area->y + area->height);
  else
    bottom_clip = 0;
  if (right_clip + left_clip >= width || top_clip + bottom_clip >= height)
    return;

  if (animator->n_frames > 0 && internal->offscreen_pixmap != NULL)
    {
      GnomeAnimatorFrame *frame;

      frame = internal->current_frame;

      /* Update the window using double buffering to make the
         animation as smooth as possible.  */

      /* Copy the background into the offscreen pixmap.  */
      gdk_pixbuf_render_to_drawable (internal->background_pixbuf,
				     internal->offscreen_pixmap,
				     widget->style->black_gc,
				     area->x - internal->area.x,
				     area->y - internal->area.y,
				     area->x - internal->area.x,
				     area->y - internal->area.y,
				     area->width,
				     area->height,
				     GDK_RGB_DITHER_NORMAL, area->x, area->y);

      /* Draw the (shaped) frame into the offscreen pixmap.  */
      if (animator->mode == GNOME_ANIMATOR_SIMPLE ||
	  !gdk_pixbuf_get_has_alpha (draw_source))
	{
	  if (frame->mask)
	    {
	      gdk_gc_set_clip_mask (widget->style->black_gc, frame->mask);
	      gdk_gc_set_clip_origin (widget->style->black_gc, x_off,	/* frame->frame->x_offset */
				      y_off);	/* frame->frame->y_offset */
	    }

	  gdk_pixbuf_render_to_drawable (draw_source,
					 internal->offscreen_pixmap,
					 widget->style->black_gc,
					 left_clip, top_clip,
					 x_off + left_clip, y_off + top_clip,
					 width - left_clip - right_clip,
					 height - top_clip - bottom_clip,
					 GDK_RGB_DITHER_NORMAL, 0, 0);
	  if (frame->mask)
	    {
	      gdk_gc_set_clip_mask (widget->style->black_gc, NULL);
	      gdk_gc_set_clip_origin (widget->style->black_gc, 0, 0);
	    }

	  /* Copy the offscreen pixmap into the window.  */
	  gdk_draw_pixmap (widget->window,
			   widget->style->black_gc,
			   internal->offscreen_pixmap,
			   area->x - internal->area.x,
			   area->y - internal->area.y, area->x, area->y,
			   area->width, area->height);
	}
      else if (animator->mode == GNOME_ANIMATOR_COLOR)
	{
	  GdkPixbuf *dest_source;
	  gint i, j, rowstride, dest_rowstride;
	  gint r, g, b;
	  gchar *dest_pixels, *c, *a, *original_pixels;

	  dest_source = gdk_pixbuf_new (ART_PIX_RGB,
					FALSE,
					gdk_pixbuf_get_bits_per_sample
					(draw_source),
					width - left_clip - right_clip,
					height - top_clip - bottom_clip);
	  gdk_gc_set_clip_mask (widget->style->black_gc, frame->mask);
	  gdk_gc_set_clip_origin (widget->style->black_gc, x_off, y_off);

	  r = widget->style->bg[GTK_WIDGET_STATE (widget)].red >> 8;
	  g = widget->style->bg[GTK_WIDGET_STATE (widget)].green >> 8;
	  b = widget->style->bg[GTK_WIDGET_STATE (widget)].blue >> 8;
	  rowstride = gdk_pixbuf_get_rowstride (draw_source);
	  dest_rowstride = gdk_pixbuf_get_rowstride (dest_source);
	  original_pixels = gdk_pixbuf_get_pixels (draw_source);
	  for (i = 0; i < height; i++)
	    {
	      for (j = 0; j < width; j++)
		{
		  c =
		    original_pixels + (i + top_clip) * rowstride + (j +
								    left_clip)
		    * 4;
		  a = c + 3;
		  *(dest_pixels + i * dest_rowstride + j * 3) =
		    r + (((*c - r) * (*a) + 0x80) >> 8);
		  c++;
		  *(dest_pixels + i * dest_rowstride + j * 3 + 1) =
		    g + (((*c - g) * (*a) + 0x80) >> 8);
		  c++;
		  *(dest_pixels + i * dest_rowstride + j * 3 + 2) =
		    b + (((*c - b) * (*a) + 0x80) >> 8);
		}
	    }

	  gdk_pixbuf_render_to_drawable (dest_source,
					 internal->offscreen_pixmap,
					 widget->style->black_gc,
					 0, 0,
					 x_off + left_clip, y_off + top_clip,
					 width, height,
					 GDK_RGB_DITHER_NORMAL, 0, 0);
	  gdk_gc_set_clip_mask (widget->style->black_gc, NULL);
	  gdk_gc_set_clip_origin (widget->style->black_gc, 0, 0);

	  gdk_pixbuf_unref (dest_source);

	  gdk_draw_pixmap (widget->window,
			   widget->style->black_gc,
			   internal->offscreen_pixmap,
			   area->x - internal->area.x,
			   area->y - internal->area.y, area->x, area->y,
			   area->width, area->height);
	}
    }
  else
    gdk_window_clear_area (widget->window,
			   area->x, area->y, area->width, area->height);
}

static gint
expose (GtkWidget * widget, GdkEventExpose * event)
{
  GnomeAnimator *animator;

  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (GNOME_IS_ANIMATOR (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  animator = GNOME_ANIMATOR (widget);

  if (GTK_WIDGET_DRAWABLE (widget))
    draw (GTK_WIDGET (animator), &event->area);

  return FALSE;
}

static void
draw (GtkWidget * widget, GdkRectangle * area)
{
  GnomeAnimator *animator;

  g_return_if_fail (widget != NULL);
  g_return_if_fail (GNOME_IS_ANIMATOR (widget));
  g_return_if_fail (area != NULL);

  if (GTK_WIDGET_REALIZED (widget))
    {
      GdkRectangle p_area;

      animator = GNOME_ANIMATOR (widget);

      if (gdk_rectangle_intersect (area, &animator->internal->area, &p_area))
	paint (animator, &p_area);

      gdk_flush ();
    }
}

static void
update (GnomeAnimator * animator)
{
  GtkWidget *widget = GTK_WIDGET (animator);

  if (GTK_WIDGET_REALIZED (widget))
    {
      if (animator->n_frames > 0)
	gtk_widget_queue_draw (widget);
      else
	gdk_window_clear_area (widget->window,
			       widget->allocation.x,
			       widget->allocation.y,
			       widget->allocation.width,
			       widget->allocation.height);
    }
}

static void
state_or_style_changed (GnomeAnimator * animator)
{
  prepare_aux_pixmaps (animator);
  draw_background_pixbuf (animator);
}

static void
state_changed (GtkWidget * widget, GtkStateType previous_state)
{
  g_assert (GNOME_IS_ANIMATOR (widget));
  state_or_style_changed (GNOME_ANIMATOR (widget));
}

static void
style_set (GtkWidget * widget, GtkStyle * previous_style)
{
  g_assert (GNOME_IS_ANIMATOR (widget));
  state_or_style_changed (GNOME_ANIMATOR (widget));
}

static GnomeAnimatorFrame *
append_frame (GnomeAnimator * animator)
{
  GnomeAnimatorPrivate *internal = animator->internal;
  GnomeAnimatorFrame *new_frame;

  new_frame = g_new0 (GnomeAnimatorFrame, 1);

  if (animator->high_range == animator->n_frames)
    animator->high_range++;

  if (internal->first_frame == NULL)
    {
      internal->first_frame = internal->last_frame = internal->current_frame =
	new_frame;
      new_frame->prev = NULL;
    }
  else
    {
      internal->last_frame->next = new_frame;
      new_frame->prev = internal->last_frame;
      internal->last_frame = new_frame;
    }

  new_frame->frame_num = animator->n_frames;
  new_frame->next = 0;
  animator->n_frames++;

  return new_frame;
}

/* ------------------------------------------------------------------------- */

static gint
timer_cb (gpointer data)
{
  GnomeAnimator *animator;

  GDK_THREADS_ENTER ();

  animator = GNOME_ANIMATOR (data);
  if (animator->status != GNOME_ANIMATOR_STATUS_STOPPED)
    gnome_animator_advance (animator, 1);

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
      GtkTypeInfo animator_info = {
	"GnomeAnimator",
	sizeof (GnomeAnimator),
	sizeof (GnomeAnimatorClass),
	(GtkClassInitFunc) class_init,
	(GtkObjectInitFunc) init,
	NULL,
	NULL
      };

      animator_type = gtk_type_unique (gtk_misc_get_type (), &animator_info);
    }

  return animator_type;
}

/*
 * FIXME:  I think we should get rid of this function and set
 *         the size with gtk_widget_set_usize().
 */
GtkWidget *
gnome_animator_new_with_size (guint width, guint height)
{
  GnomeAnimator *animator;
  GtkWidget *widget;

  animator = gtk_type_new (gnome_animator_get_type ());
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
gnome_animator_set_loop_type (GnomeAnimator * animator,
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

GnomeAnimatorLoopType gnome_animator_get_loop_type (GnomeAnimator * animator)
{
  g_print ("gnome_animator_get_loop_type()\n");

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
gnome_animator_set_playback_direction (GnomeAnimator * animator,
				       GnomeAnimatorDirection direction)
{
  g_return_if_fail (animator != NULL);

  animator->playback_direction = direction;
}

/*
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
 */

GnomeAnimatorDirection
gnome_animator_get_playback_direction (GnomeAnimator * animator)
{
  g_return_val_if_fail (animator != NULL, 0);

  return animator->playback_direction;
}

/*
 * Begin pixbuf loading code.
 */

gboolean
gnome_animator_append_frame_from_pixbuf_at_size (GnomeAnimator * animator,
						 GdkPixbuf * pixbuf,
						 gint x_offset,
						 gint y_offset,
						 guint32 interval,
						 guint width, guint height)
{
  GnomeAnimatorFrame *new_frame;

  g_return_val_if_fail (animator != NULL, FALSE);
  g_return_val_if_fail (pixbuf != NULL, FALSE);

  if (width == 0)
    width = gdk_pixbuf_get_width (pixbuf);
  if (height == 0)
    height = gdk_pixbuf_get_height (pixbuf);

  new_frame = append_frame (animator);

  new_frame->frame = g_new0 (GdkPixbufFrame, 1);
  new_frame->frame->pixbuf = gdk_pixbuf_new (gdk_pixbuf_get_format (pixbuf),
					     TRUE,
					     gdk_pixbuf_get_bits_per_sample
					     (pixbuf), width, height);
  gdk_pixbuf_copy_area (pixbuf, x_offset, y_offset, width, height,
			new_frame->frame->pixbuf, 0, 0);
  new_frame->frame->x_offset = x_offset;
  new_frame->frame->y_offset = y_offset;
  new_frame->interval = interval;
  animator->n_frames++;

  return TRUE;
}

gboolean
gnome_animator_append_frames_from_pixbuf_at_size (GnomeAnimator * animator,
						  GdkPixbuf * pixbuf,
						  gint x_offset,
						  gint y_offset,
						  guint32 interval,
						  gint x_unit,
						  guint width, guint height)
{
  guint num_frames, i, offs;

  if (width == 0)
    width = x_unit;
  if (height == 0)
    height = gdk_pixbuf_get_height (pixbuf);

  num_frames = gdk_pixbuf_get_width (pixbuf) / x_unit;

  for (i = offs = 0; i < num_frames; i++, offs += width)
    {
      GnomeAnimatorFrame *frame;

      frame = append_frame (animator);
      frame->frame = g_new0 (GdkPixbufFrame, 1);
      frame->frame->pixbuf = gdk_pixbuf_new (gdk_pixbuf_get_format (pixbuf),
					     TRUE,
					     gdk_pixbuf_get_bits_per_sample
					     (pixbuf), width, height);
      gdk_pixbuf_copy_area (pixbuf, offs, y_offset, width, height,
			    frame->frame->pixbuf, 0, 0);
      frame->frame->x_offset = x_offset;
      frame->frame->y_offset = y_offset;
      frame->interval = interval;
    }

  return TRUE;
}

gboolean
gnome_animator_append_frames_from_pixbuf (GnomeAnimator * animator,
					  GdkPixbuf * pixbuf,
					  gint x_offset,
					  gint y_offset,
					  guint32 interval, gint x_unit)
{
  return gnome_animator_append_frames_from_pixbuf_at_size (animator,
							   pixbuf,
							   x_offset,
							   y_offset,
							   interval,
							   x_unit, 0, 0);
}

#if 0
gboolean
gnome_animator_append_frames_from_pixbuf_animation (GnomeAnimator * animator,
						    GdkPixbufAnimation *
						    pixbuf)
{
  int x;

  g_print ("gnome_animator_append_frames_from_pixbuf_animation()\n");

  g_return_val_if_fail (animator != NULL, FALSE);
  g_return_val_if_fail (pixbuf != NULL, FALSE);

  for (x = 0; x < pixbuf->n_frames; x++)
    {
      GnomeAnimatorFrame *frame;
      GdkPixbufFrame *pixbuf_frame;

      pixbuf_frame = g_list_nth (pixbuf->frames, x);

      frame = append_frame (animator);
      frame->frame = g_new0 (GdkPixbufFrame, 1);
      frame->frame->pixbuf = pixbuf_frame->pixbuf;
      gdk_pixbuf_ref (frame->frame->pixbuf);
      frame->frame->x_offset = pixbuf_frame->x_offset;
      frame->frame->y_offset = pixbuf_frame->y_offset;
      frame->delay_time = pixbuf_frame->delay_time;
      frame->mode = pixbuf_frame->mode;
    }

  return TRUE;
}
#endif

/**
 * gnome_animator_start
 * @animator: Animator widget to be started
 *
 * Description:  Initiate display of animated frames.
 **/

void
gnome_animator_start (GnomeAnimator * animator)
{
  g_return_if_fail (animator != NULL);

  if (animator->n_frames > 0)
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
gnome_animator_stop (GnomeAnimator * animator)
{
  g_return_if_fail (animator != NULL);

  if (animator->status == GNOME_ANIMATOR_STATUS_RUNNING)
    gtk_timeout_remove (animator->internal->timeout_id);
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
gnome_animator_advance (GnomeAnimator * animator, gint num)
{
  gboolean stop;
  guint new_frame;

  g_return_val_if_fail (animator != NULL, FALSE);

  if (num == 0)
    {
      stop = (animator->status == GNOME_ANIMATOR_STATUS_STOPPED);
      new_frame = animator->frame_num;
    }
  else
    {
      if (animator->playback_direction == GNOME_ANIMATOR_DIRECTION_BACKWARD)
	num = -num;

      if ((num < 0 && animator->frame_num >= (guint) - num) ||
	  (num > 0 && (guint) num < animator->n_frames - animator->frame_num))
	{
	  new_frame = animator->frame_num + num;
	  stop = FALSE;
	}
      else
	{
	  /* We are overflowing the frame list: handle the various loop
	     types.  */
	  switch (animator->loop_type)
	    {
	    case GNOME_ANIMATOR_LOOP_NONE:
	      if (num < 0)
		new_frame = 0;
	      else
		new_frame = animator->n_frames - 1;
	      if (
		  (num < 0
		   && animator->playback_direction ==
		   GNOME_ANIMATOR_DIRECTION_FORWARD) || (num > 0
							 && animator->
							 playback_direction ==
							 GNOME_ANIMATOR_DIRECTION_BACKWARD))
		stop = FALSE;
	      else
		stop = TRUE;
	      break;

	    case GNOME_ANIMATOR_LOOP_RESTART:
	      if (num > 0)
		new_frame = ((num - (animator->n_frames -
				     animator->frame_num)) %
			     animator->n_frames);
	      else
		new_frame =
		  (animator->n_frames - 1 -
		   ((-num - (animator->frame_num + 1)) % animator->n_frames));
	      stop = FALSE;
	      break;

	    case GNOME_ANIMATOR_LOOP_PING_PONG:
	      {
		guint num1;
		gboolean back;

		if (num > animator->n_frames)
		  num1 = num - (animator->n_frames - 1 - animator->frame_num);
		else
		  num1 = -num - animator->frame_num;

		back = (((num1 / (animator->n_frames - 1)) % 2) == 0);
		if (num < 0)
		  back = !back;

		if (back)
		  {
		    new_frame = (animator->n_frames - 1 -
				 (num1 % (animator->n_frames - 1)));
		    animator->playback_direction =
		      GNOME_ANIMATOR_DIRECTION_BACKWARD;
		  }
		else
		  {
		    new_frame = num1 % (animator->n_frames - 1);
		    animator->playback_direction =
		      GNOME_ANIMATOR_DIRECTION_FORWARD;
		  }
	      }

	      stop = FALSE;
	      break;

	    default:
	      g_warning ("Unknown GnomeAnimatorLoopType %d",
			 animator->loop_type);
	      stop = TRUE;
	      new_frame = animator->frame_num;
	      break;
	    }
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
gnome_animator_goto_frame (GnomeAnimator * animator, guint frame_number)
{
  GnomeAnimatorPrivate *internal;
  GnomeAnimatorFrame *frame;
  gint dist1;
  guint dist2;
  guint i;

  g_return_if_fail (animator != NULL);
  g_return_if_fail (frame_number < animator->n_frames);

  internal = animator->internal;

  /* Try to be smart and minimize the number of steps spent walking on
     the linked list.  */

  dist1 = frame_number - animator->frame_num;
  dist2 = animator->n_frames - 1 - frame_number;

  if (frame_number < (guint) ABS (dist1) && frame_number < dist2)
    {
      frame = internal->first_frame;
      for (i = 0; i < frame_number; i++)
	frame = frame->next;
    }
  else if (dist2 < (guint) ABS (dist1))
    {
      frame = internal->last_frame;
      for (i = 0; i < dist2; i++)
	frame = frame->prev;
    }
  else
    {
      frame = internal->current_frame;
      if (dist1 > 0)
	for (i = 0; i < (guint) dist1; i++)
	  frame = frame->next;
      else
	for (i = 0; i < (guint) - dist1; i++)
	  frame = frame->prev;
    }

  animator->frame_num = frame_number;
  internal->current_frame = frame;

  update (animator);

  if (animator->status == GNOME_ANIMATOR_STATUS_RUNNING)
    {
      guint32 interval;

      if (internal->timeout_id != 0)
	gtk_timeout_remove (internal->timeout_id);

      interval = (guint32) ((double) frame->interval /
			    animator->playback_speed + 0.5);

      internal->timeout_id = gtk_timeout_add (interval,
					      timer_cb, (gpointer) animator);
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
gnome_animator_get_current_frame_number (GnomeAnimator * animator)
{
  g_return_val_if_fail (animator != NULL, 0);

  return animator->frame_num;
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
gnome_animator_get_status (GnomeAnimator * animator)
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
gnome_animator_set_playback_speed (GnomeAnimator * animator, double speed)
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

gdouble
gnome_animator_get_playback_speed (GnomeAnimator * animator)
{
  g_return_val_if_fail (animator != NULL, -1.0);

  return animator->playback_speed;
}

void
gnome_animator_set_range (GnomeAnimator * animator, guint low, guint high)
{
  g_return_if_fail (animator != NULL);
  g_return_if_fail (high < animator->n_frames);
  g_return_if_fail (low < high);

  animator->low_range = low;
  animator->high_range = high;
}
