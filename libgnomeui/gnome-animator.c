/* GNOME GUI Library
 * Copyright (C) 1998, 1999 the Free Software Foundation
 * All rights reserved
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
/*
  @NOTATION@
 */

#include <config.h>
#include <string.h>
#include "gnome-macros.h"
#include <gobject/gparam.h>
#include <gtk/gtkmain.h>
#include "gnome-animator.h"
#include <libgnome/gnome-i18n.h>

enum {
  PROP_0,
  PROP_LOOP_TYPE,
  PROP_DIRECTION,
  PROP_NUM_FRAMES,
  PROP_CURRENT_FRAME,
  PROP_STATUS,
  PROP_SPEED
};

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

struct _GnomeAnimatorFrame
{
  int frame_num;

  /* The pixbuf with this frame's image data */
  GdkPixbuf *pixbuf;

  /* Offsets for overlaying onto the animation's area */
  int x_offset;
  int y_offset;

  /* Overlay mode */
  GdkPixbufFrameAction action;

  GdkBitmap *mask;
  guint32 interval;
  GnomeAnimatorFrame *next;
  GnomeAnimatorFrame *prev;
};

static void gnome_animator_class_init (GnomeAnimatorClass * class);
static void gnome_animator_init (GnomeAnimator * animator);
static void destroy (GtkObject * object);
static void finalize (GObject * object);
static void set_property (GObject * object, 
			  guint param_id,
			  const GValue * value,
			  GParamSpec * pspec);
static void get_property (GObject * object,
			  guint param_id,
			  GValue *value,
			  GParamSpec * pspec);
static void realize (GtkWidget * widget);
static void unrealize (GtkWidget * widget);
static void prepare_aux_pixmaps (GnomeAnimator * animator);
static void draw_background_pixbuf (GnomeAnimator * animator);
static void draw (GtkWidget * widget, GdkRectangle * area);
static gint expose (GtkWidget * widget, GdkEventExpose * event);
static void size_allocate (GtkWidget * widget, GtkAllocation * allocation);
static void update (GnomeAnimator * animator);
/*UNUSED
static void state_or_style_changed (GnomeAnimator * animator);
static void state_changed (GtkWidget * widget, GtkStateType previous_state);
static void style_set (GtkWidget * widget, GtkStyle * previous_style);
*/
static GnomeAnimatorFrame *append_frame (GnomeAnimator * animator);
static void free_frame (GnomeAnimatorFrame *frame);
static gint timer_cb (gpointer data);

GNOME_CLASS_BOILERPLATE (GnomeAnimator, gnome_animator,
			 GtkMisc, gtk_misc)

static void
gnome_animator_init (GnomeAnimator * animator)
{
  GTK_WIDGET_SET_FLAGS (animator, GTK_NO_WINDOW);

  animator->frame_num = 0;
  animator->status = GNOME_ANIMATOR_STATUS_STOPPED;
  animator->loop_type = GNOME_ANIMATOR_LOOP_RESTART;
  animator->playback_direction = GNOME_ANIMATOR_DIRECTION_FORWARD;

  animator->_priv = g_new0 (GnomeAnimatorPrivate, 1);
}

static void
gnome_animator_class_init (GnomeAnimatorClass * class)
{
  GtkObjectClass *object_class;
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass *) class;
  gobject_class = (GObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;

  g_object_class_install_property (gobject_class,
				PROP_LOOP_TYPE,
				g_param_spec_enum ("loop_type",
						   _("Loop type"),
						   _("The type of loop the GnomeAnimator uses"),
						   GTK_TYPE_ENUM,
						   GNOME_ANIMATOR_LOOP_NONE,
						   (G_PARAM_READABLE |
						    G_PARAM_WRITABLE)));
  g_object_class_install_property (gobject_class,
				PROP_DIRECTION,
				g_param_spec_enum ("direction",
						   _("Direction"),
						   _("Animation direction"),
						   GTK_TYPE_ENUM,
						   GNOME_ANIMATOR_DIRECTION_FORWARD,
						   (G_PARAM_READABLE |
						    G_PARAM_WRITABLE)));
  g_object_class_install_property (gobject_class,
				PROP_NUM_FRAMES,
				g_param_spec_uint ("num_frames",
						   _("Number of frames"),
						   _("Total number of frames in animation"),
						   0,
						   0,
						   0,
						   G_PARAM_READABLE));
  g_object_class_install_property (gobject_class,
				PROP_CURRENT_FRAME,
				g_param_spec_uint ("current_frame",
						   _("Current frame"),
						   _("Current frame number"),
						   0,
						   0,
						   0,
						   G_PARAM_READABLE));
  g_object_class_install_property (gobject_class,
				PROP_STATUS,
				g_param_spec_enum ("status",
						   _("Animation status"),
						   _("Animation status"),
						   GTK_TYPE_ENUM,
						   GNOME_ANIMATOR_STATUS_STOPPED,
						   (G_PARAM_READABLE |
						    G_PARAM_WRITABLE)));
  g_object_class_install_property (gobject_class,
				PROP_SPEED,
				g_param_spec_double ("speed",
						     _("Speed"),
						     _("Animation speed"),
						     0.0,
						     1000.0,
						     1.0,
						     (G_PARAM_READABLE |
						      G_PARAM_WRITABLE )));

  widget_class->expose_event = expose;
  widget_class->size_allocate = size_allocate;
  widget_class->realize = realize;
  widget_class->unrealize = unrealize;

  object_class->destroy = destroy;
  gobject_class->finalize = finalize;
  gobject_class->set_property = set_property;
  gobject_class->get_property = get_property;
}

static void
destroy (GtkObject * object)
{
	GnomeAnimator *self = GNOME_ANIMATOR (object);
	GnomeAnimatorFrame *frame;

	/* remember, destroy can be run multiple times! */

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ANIMATOR (object));

	if (self->_priv->offscreen_pixmap != NULL) {
		gdk_pixmap_unref (self->_priv->offscreen_pixmap);
		self->_priv->offscreen_pixmap = NULL;
	}

	if (self->_priv->background_pixbuf != NULL) {
		gdk_pixbuf_unref (self->_priv->background_pixbuf);
		self->_priv->background_pixbuf = NULL;
	}

	/* free all frames */
	frame = self->_priv->first_frame;
	while (frame != NULL) {
		GnomeAnimatorFrame *next = frame->next;
		free_frame (frame);
		frame = next;
	}
	self->_priv->first_frame = NULL;
	self->_priv->last_frame = NULL;
	self->_priv->current_frame = NULL;

	GNOME_CALL_PARENT_HANDLER (GTK_OBJECT_CLASS, destroy, (object));
}

static void
finalize (GObject * object)
{
	GnomeAnimator *self;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ANIMATOR (object));

	self = GNOME_ANIMATOR(object);

	g_free(self->_priv);
	self->_priv = NULL;

	GNOME_CALL_PARENT_HANDLER (G_OBJECT_CLASS, finalize, (object));
}

static void
set_property (GObject * object,
	      guint param_id,
	      const GValue * value,
	      GParamSpec * pspec)
{
  GnomeAnimator *animator;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GNOME_IS_ANIMATOR (object));

  animator = GNOME_ANIMATOR (object);
  switch (param_id) {
  case PROP_LOOP_TYPE:
    gnome_animator_set_loop_type (animator, g_value_get_enum (value));
    break;

  case PROP_DIRECTION:
    gnome_animator_set_playback_direction (animator,
					   g_value_get_enum (value));
    break;

  case PROP_CURRENT_FRAME:
    gnome_animator_goto_frame (animator, g_value_get_uint (value));
    break;

  case PROP_STATUS: {
    GnomeAnimatorStatus status = g_value_get_enum (value);
    if (status == GNOME_ANIMATOR_STATUS_RUNNING)
      gnome_animator_start (animator);
    else
      gnome_animator_stop (animator);
    
    break;
  }

  case PROP_SPEED:
    gnome_animator_set_playback_speed (animator, g_value_get_double (value));
    break;

  default:
    break;
  }
}


static void
get_property (GObject * object,
	      guint param_id,
	      GValue * value,
	      GParamSpec * pspec)
{
  GnomeAnimator *animator;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GNOME_IS_ANIMATOR (object));

  animator = GNOME_ANIMATOR (object);

  switch (param_id) {
  case PROP_LOOP_TYPE:
    g_value_set_enum (value, animator->loop_type);
    break;

  case PROP_DIRECTION:
    g_value_set_enum (value, animator->playback_direction);
    break;

  case PROP_NUM_FRAMES:
    g_value_set_uint (value, animator->n_frames);
    break;

  case PROP_CURRENT_FRAME:
    g_value_set_uint (value, animator->frame_num);
    break;

  case PROP_STATUS:
    g_value_set_enum (value, animator->status);
    break;

  case PROP_SPEED:
    g_value_set_double (value, animator->playback_speed);
    break;
  }
}
    
static void
realize (GtkWidget * widget)
{
	GnomeAnimator *self = GNOME_ANIMATOR (widget);

	GNOME_CALL_PARENT_HANDLER (GTK_WIDGET_CLASS, realize, (widget));

	prepare_aux_pixmaps (self);
	draw_background_pixbuf (self);
}

static void
unrealize (GtkWidget * widget)
{
	GnomeAnimator *self = GNOME_ANIMATOR (widget);

	GNOME_CALL_PARENT_HANDLER (GTK_WIDGET_CLASS, unrealize, (widget));

	if (self->_priv->offscreen_pixmap != NULL) {
		gdk_pixmap_unref (self->_priv->offscreen_pixmap);
		self->_priv->offscreen_pixmap = NULL;
	}

	if (self->_priv->background_pixbuf != NULL) {
		gdk_pixbuf_unref (self->_priv->background_pixbuf);
		self->_priv->background_pixbuf = NULL;
	}
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

  if (GTK_WIDGET_REALIZED (widget))
    gdk_window_clear_area (widget->window,
			   widget->allocation.x, widget->allocation.y,
			   widget->allocation.width,
			   widget->allocation.height);

  animator->_priv->area.x = widget->allocation.x +
    (widget->allocation.width - widget->requisition.width) / 2;
  animator->_priv->area.y = widget->allocation.y +
    (widget->allocation.height - widget->requisition.height) / 2;
  animator->_priv->area.width = widget->requisition.width;
  animator->_priv->area.height = widget->requisition.height;

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
  GnomeAnimatorPrivate *_priv = animator->_priv;
  gint old_width, old_height;

  if (_priv->offscreen_pixmap != NULL)
    gdk_window_get_size (_priv->offscreen_pixmap, &old_width, &old_height);
  else
    old_width = old_height = 0;

  if ((widget->requisition.width != (guint16) old_width)
      && (widget->requisition.height != (guint16) old_height))
    {
      GdkVisual *visual = gtk_widget_get_visual (widget);

      if (_priv->offscreen_pixmap != NULL)
	{
	  gdk_pixmap_unref (_priv->offscreen_pixmap);
	  _priv->offscreen_pixmap = NULL;
	}

      if (_priv->background_pixbuf != NULL)
	{
	  gdk_pixbuf_unref (_priv->background_pixbuf);
	  _priv->background_pixbuf = NULL;
	}

      if (widget->requisition.width > 0 &&
	  widget->requisition.height > 0 && GTK_WIDGET_REALIZED (widget))
	{
	  _priv->offscreen_pixmap = gdk_pixmap_new (widget->window,
						    widget->requisition.
						    width,
						    widget->requisition.
						    height, visual->depth);

	  _priv->background_pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
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
  GnomeAnimatorPrivate *_priv = animator->_priv;
  GtkWidget *widget;
  GdkPixmap *pixmap;

  widget = GTK_WIDGET (animator);

  if (_priv->background_pixbuf != NULL && widget->window != NULL)
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
			    -animator->_priv->area.x,
			    -animator->_priv->area.y);

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

      animator->_priv->background_pixbuf =
	gdk_pixbuf_get_from_drawable (animator->_priv->background_pixbuf,
				      pixmap,
				      colormap,
				      0, 0,
				      0, 0,
				      widget->requisition.width,
				      widget->requisition.height);

      gdk_gc_destroy (gc);
    }
}


static GdkPixbuf *
get_current_frame (GnomeAnimator *animator, int *x_offset, int *y_offset)
{
	GnomeAnimatorFrame *frame, *framei;
	GdkPixbuf *pixbuf;
	guchar *pixels;
	int rowstride;

	*x_offset = 0;
	*y_offset = 0;

	if (animator->_priv->current_frame == NULL)
		return NULL;

	frame = animator->_priv->current_frame;
	if(frame->prev == NULL ||
	   frame->prev->action == GDK_PIXBUF_FRAME_DISPOSE) {
		*x_offset = frame->x_offset;
		*y_offset = frame->y_offset;

		return gdk_pixbuf_ref (frame->pixbuf);
	}

	/* This not incredibly efficent and is done for each frame,
	 * sometime in the future someone should make this efficent
	 * I guess but I doubt it will ever be a problem */

	pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
				 animator->_priv->area.width,
				 animator->_priv->area.height);

	pixels = gdk_pixbuf_get_pixels (pixbuf);
	rowstride = gdk_pixbuf_get_rowstride (pixbuf);

	memset(pixels, 0, rowstride * animator->_priv->area.height);

	framei = frame->prev;
	while (framei->prev != NULL) {
		if (framei->action == GDK_PIXBUF_FRAME_DISPOSE)
			break;
		framei = frame->prev;
	}

	while (frame != framei) {
		if (framei->action != GDK_PIXBUF_FRAME_REVERT)
			gdk_pixbuf_composite(framei->pixbuf,
					     pixbuf,
					     framei->x_offset,
					     framei->y_offset,
					     gdk_pixbuf_get_width(framei->pixbuf),
					     gdk_pixbuf_get_height(framei->pixbuf),
					     0, 0, 1, 1,
					     GDK_INTERP_NEAREST,
					     255);

		framei = framei->next;
	}

	gdk_pixbuf_composite(frame->pixbuf,
			     pixbuf,
			     frame->x_offset,
			     frame->y_offset,
			     gdk_pixbuf_get_width(frame->pixbuf),
			     gdk_pixbuf_get_height(frame->pixbuf),
			     0, 0, 1, 1,
			     GDK_INTERP_NEAREST,
			     255);

	return pixbuf;
}

/*
 * Call draw_background_pixbuf(), then draw background_pixbuf onto
 * the window.
 */
static void
paint (GnomeAnimator * animator, GdkRectangle * area)
{
  GnomeAnimatorPrivate *_priv;
  GdkPixbuf *draw_source;
  GtkMisc *misc;
  GtkWidget *widget;
  gint top_clip, bottom_clip, left_clip, right_clip;
  gint width, height;
  gint x_off, y_off;

  g_return_if_fail (GTK_WIDGET_DRAWABLE (animator));

  widget = GTK_WIDGET (animator);
  misc = GTK_MISC (animator);
  _priv = animator->_priv;

  draw_source = get_current_frame (animator, &x_off, &y_off);

  if (draw_source == NULL) {
	  gdk_draw_rectangle (widget->window,
			      widget->style->black_gc,
			      TRUE,
			      0, 0,
			      widget->allocation.width,
			      widget->allocation.height);
	  return;
  }

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

  if (animator->n_frames > 0 && _priv->offscreen_pixmap != NULL)
    {
      GnomeAnimatorFrame *frame;

      /*FIXME: deal with mask for other then _DISPOSE modes */
      frame = _priv->current_frame;

      /* Update the window using double buffering to make the
         animation as smooth as possible.  */

      /* Copy the background into the offscreen pixmap.  */
      gdk_pixbuf_render_to_drawable (_priv->background_pixbuf,
				     _priv->offscreen_pixmap,
				     widget->style->black_gc,
				     area->x - _priv->area.x,
				     area->y - _priv->area.y,
				     area->x - _priv->area.x,
				     area->y - _priv->area.y,
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
	      gdk_gc_set_clip_origin (widget->style->black_gc, x_off,	/* frame->x_offset */
				      y_off);	/* frame->y_offset */
	    }

	  gdk_pixbuf_render_to_drawable (draw_source,
					 _priv->offscreen_pixmap,
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
			   _priv->offscreen_pixmap,
			   area->x - _priv->area.x,
			   area->y - _priv->area.y, area->x, area->y,
			   area->width, area->height);
	}
      else if (animator->mode == GNOME_ANIMATOR_COLOR)
	{
	  GdkPixbuf *dest_source;
	  gint i, j, rowstride, dest_rowstride;
	  gint r, g, b;
	  gchar *dest_pixels, *c, *a, *original_pixels;

	  dest_source = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
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
	  dest_rowstride = gdk_pixbuf_get_rowstride (dest_source);
	  dest_pixels = gdk_pixbuf_get_pixels (dest_source);
	  rowstride = gdk_pixbuf_get_rowstride (draw_source);
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
					 _priv->offscreen_pixmap,
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
			   _priv->offscreen_pixmap,
			   area->x - _priv->area.x,
			   area->y - _priv->area.y, area->x, area->y,
			   area->width, area->height);
	}
    }
  else
    gdk_window_clear_area (widget->window,
			   area->x, area->y, area->width, area->height);

  gdk_pixbuf_unref (draw_source);
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

      if (gdk_rectangle_intersect (area, &animator->_priv->area, &p_area))
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

/* UNUSED
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
*/

static void
free_frame (GnomeAnimatorFrame *frame)
{
	if (frame->pixbuf != NULL)
		gdk_pixbuf_unref (frame->pixbuf);
	frame->pixbuf = NULL;
	g_free(frame);
}

static GnomeAnimatorFrame *
append_frame (GnomeAnimator * animator)
{
  GnomeAnimatorPrivate *_priv = animator->_priv;
  GnomeAnimatorFrame *new_frame;

  new_frame = g_new0 (GnomeAnimatorFrame, 1);

  if (animator->high_range == animator->n_frames)
    animator->high_range++;

  if (_priv->first_frame == NULL)
    {
      _priv->first_frame =
	      _priv->last_frame =
	      _priv->current_frame =
	      new_frame;
      new_frame->prev = NULL;
    }
  else
    {
      _priv->last_frame->next = new_frame;
      new_frame->prev = _priv->last_frame;
      _priv->last_frame = new_frame;
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

  new_frame->pixbuf = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (pixbuf),
				      TRUE,
				      gdk_pixbuf_get_bits_per_sample
				      (pixbuf), width, height);
  gdk_pixbuf_copy_area (pixbuf, x_offset, y_offset, width, height,
			new_frame->pixbuf, 0, 0);
  new_frame->x_offset = x_offset;
  new_frame->y_offset = y_offset;
  new_frame->interval = interval;
  new_frame->action = GDK_PIXBUF_FRAME_DISPOSE;
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
      frame->pixbuf = gdk_pixbuf_new (gdk_pixbuf_get_colorspace (pixbuf),
				      TRUE,
				      gdk_pixbuf_get_bits_per_sample
				      (pixbuf), width, height);
      gdk_pixbuf_copy_area (pixbuf, offs, y_offset, width, height,
			    frame->pixbuf, 0, 0);
      frame->x_offset = x_offset;
      frame->y_offset = y_offset;
      frame->interval = interval;
      frame->action = GDK_PIXBUF_FRAME_DISPOSE;
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

gboolean
gnome_animator_append_frames_from_pixbuf_animation (GnomeAnimator * animator,
						    GdkPixbufAnimation * pixbuf)
{
  GList *li;

  g_return_val_if_fail (animator != NULL, FALSE);
  g_return_val_if_fail (pixbuf != NULL, FALSE);

  for(li = gdk_pixbuf_animation_get_frames(pixbuf); li != NULL; li = li->next)
    {
      GnomeAnimatorFrame *frame;
      GdkPixbufFrame *pixbuf_frame = li->data;

      frame = append_frame (animator);
      frame->pixbuf = gdk_pixbuf_frame_get_pixbuf(pixbuf_frame);
      gdk_pixbuf_ref (frame->pixbuf);
      frame->x_offset = gdk_pixbuf_frame_get_x_offset(pixbuf_frame);
      frame->y_offset = gdk_pixbuf_frame_get_y_offset(pixbuf_frame);
      frame->interval = gdk_pixbuf_frame_get_delay_time(pixbuf_frame);
      frame->action = gdk_pixbuf_frame_get_action(pixbuf_frame);
    }

  return TRUE;
}

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
    gtk_timeout_remove (animator->_priv->timeout_id);
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
  GnomeAnimatorPrivate *_priv;
  GnomeAnimatorFrame *frame;
  gint dist1;
  guint dist2;
  guint i;

  g_return_if_fail (animator != NULL);
  g_return_if_fail (frame_number < animator->n_frames);

  _priv = animator->_priv;

  /* Try to be smart and minimize the number of steps spent walking on
     the linked list.  */

  dist1 = frame_number - animator->frame_num;
  dist2 = animator->n_frames - 1 - frame_number;

  if (frame_number < (guint) ABS (dist1) && frame_number < dist2)
    {
      frame = _priv->first_frame;
      for (i = 0; i < frame_number; i++)
	frame = frame->next;
    }
  else if (dist2 < (guint) ABS (dist1))
    {
      frame = _priv->last_frame;
      for (i = 0; i < dist2; i++)
	frame = frame->prev;
    }
  else
    {
      frame = _priv->current_frame;
      if (dist1 > 0)
	for (i = 0; i < (guint) dist1; i++)
	  frame = frame->next;
      else
	for (i = 0; i < (guint) - dist1; i++)
	  frame = frame->prev;
    }

  animator->frame_num = frame_number;
  _priv->current_frame = frame;

  update (animator);

  if (animator->status == GNOME_ANIMATOR_STATUS_RUNNING)
    {
      guint32 interval;

      if (_priv->timeout_id != 0)
	gtk_timeout_remove (_priv->timeout_id);

      interval = (guint32) ((double) frame->interval /
			    animator->playback_speed + 0.5);

      _priv->timeout_id = gtk_timeout_add (interval,
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
