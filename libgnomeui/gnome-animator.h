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

#ifndef _GNOME_ANIMATOR_H
#define _GNOME_ANIMATOR_H

#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>

#include "gnome-pixmap.h"

G_BEGIN_DECLS

#define GNOME_TYPE_ANIMATOR            (gnome_animator_get_type())
#define GNOME_ANIMATOR(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_ANIMATOR, GnomeAnimator))
#define GNOME_ANIMATOR_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ANIMATOR, GnomeAnimatorClass))
#define GNOME_IS_ANIMATOR(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_ANIMATOR))
#define GNOME_IS_ANIMATOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ANIMATOR))
#define GNOME_ANIMATOR_GET_CLASS(obj)  (GTK_CHECK_GET_CLASS ((obj), GNOME_TYPE_ANIMATOR, GnomeAnimatorClass))

/* Current animator status.  */
typedef enum
{
  GNOME_ANIMATOR_STATUS_STOPPED,
  GNOME_ANIMATOR_STATUS_RUNNING
} GnomeAnimatorStatus;

typedef enum
{
  GNOME_ANIMATOR_SIMPLE,
  GNOME_ANIMATOR_COLOR
} GnomeAnimatorDrawMode;

typedef enum
{
  GNOME_ANIMATOR_DIRECTION_FORWARD,
  GNOME_ANIMATOR_DIRECTION_BACKWARD
} GnomeAnimatorDirection;

/* Looping type. The behavior depends upon the playback direction.
 *    NONE:      Do not loop the animation. Stop once it has completed one
 *               cycle.
 *    RESTART:   Once the cycle has completed, restart it.
 *    PING_PONG: After the cycle has completed, reverse the direction
 *               of playback.
 */
typedef enum
{
  GNOME_ANIMATOR_LOOP_NONE,
  GNOME_ANIMATOR_LOOP_RESTART,
  GNOME_ANIMATOR_LOOP_PING_PONG
} GnomeAnimatorLoopType;

typedef struct _GnomeAnimator GnomeAnimator;
typedef struct _GnomeAnimatorClass GnomeAnimatorClass;

/* Private stuff.  */
typedef struct _GnomeAnimatorFrame GnomeAnimatorFrame;
typedef struct _GnomeAnimatorPrivate GnomeAnimatorPrivate;

struct _GnomeAnimator
{
  GtkMisc misc;

  guint frame_num;
  guint n_frames;
  guint low_range;
  guint high_range;
  GnomeAnimatorStatus status;
  GnomeAnimatorLoopType loop_type;
  GnomeAnimatorDirection playback_direction;
  gdouble playback_speed;
  GnomeAnimatorDrawMode mode;

  /*< private >*/
  GnomeAnimatorPrivate *_priv;
};

struct _GnomeAnimatorClass
{
  GtkMiscClass parent_class;
};

GtkType gnome_animator_get_type (void) G_GNUC_CONST;
GtkWidget *gnome_animator_new_with_size (guint width, guint height);

void gnome_animator_set_loop_type (GnomeAnimator * animator,
				   GnomeAnimatorLoopType loop_type);
GnomeAnimatorLoopType gnome_animator_get_loop_type (GnomeAnimator * animator);
void gnome_animator_set_playback_direction (GnomeAnimator * animator,
					    GnomeAnimatorDirection direction);
GnomeAnimatorDirection gnome_animator_get_playback_direction (GnomeAnimator *
							      animator);

/* Append a frame from `pixbuf', rendering it at the specified size.  */
gboolean gnome_animator_append_frame_from_pixbuf_at_size (GnomeAnimator *
							  animator,
							  GdkPixbuf * pixbuf,
							  gint x_offset,
							  gint y_offset,
							  guint32 interval,
							  guint width,
							  guint height);

/* Append a frame from `pixbuf', rendering it at its natural size.  */
gboolean gnome_animator_append_frame_from_pixbuf (GnomeAnimator * animator,
						  GdkPixbuf * pixbuf,
						  gint x_offset,
						  gint y_offset,
						  guint32 interval);

/* Crop `pixbuf' into frames `x_unit' pixels wide, and append them as
 * frames to the animator with the specified `interval' and offsets.
 * Each frame is rendered at the specified size. If `pixbuf' is an
 * animated pixbuf, `x_unit' and `interval' will be ignored and the
 * frames will be appended as they are stored in the pixbuf.
 */
gboolean gnome_animator_append_frames_from_pixbuf_at_size (GnomeAnimator *
							   animator,
							   GdkPixbuf * pixbuf,
							   gint x_offset,
							   gint y_offset,
							   guint32 interval,
							   gint x_unit,
							   guint width,
							   guint height);

/* Crop `pixbuf' into frames `x_unit' pixels wide, and append them as
 * frames to the animator with the specified `interval' and offsets.
 * Each frame is rendered at its natural size. If `pixbuf' is an animated
 * pixbuf, `x_unit' and `interval' will be ignored and the frames will be
 * appended as they are stored in the pixbuf.
 */
gboolean gnome_animator_append_frames_from_pixbuf (GnomeAnimator * animator,
						   GdkPixbuf * pixbuf,
						   gint x_offset,
						   gint y_offset,
						   guint32 interval,
						   gint x_unit);

/* Append frames from a GdkPixbufAnimation */
gboolean
gnome_animator_append_frames_from_pixbuf_animation (GnomeAnimator * animator,
						    GdkPixbufAnimation * pixbuf);

/* Start and stop playback in the GTK loop. */
void gnome_animator_start (GnomeAnimator * animator);
void gnome_animator_stop (GnomeAnimator * animator);

/* Advance the animation by `num' frames.  A positive value uses the
   specified playback direction; a negative one goes in the opposite
   direction.  If the loop type is `GNOME_ANIMATOR_LOOP_NONE' and the
   value causes the frame counter to overflow, `FALSE' is returned and
   the animator is stopped; otherwise, `TRUE' is returned.  */
gboolean gnome_animator_advance (GnomeAnimator * animator, gint num);

/* Set the number of current frame.  The result is immediately
   visible.  */
void gnome_animator_goto_frame (GnomeAnimator * animator, guint frame_number);

/* Get the total number of frames. */
guint gnome_animator_get_total_frames (GnomeAnimator * animator);

/* Get the number of current frame.  */
guint gnome_animator_get_current_frame_number (GnomeAnimator * animator);

/* Get the animator status.  */
GnomeAnimatorStatus gnome_animator_get_status (GnomeAnimator * animator);

/* Set/get speed factor (the higher, the faster: the `interval' value
   is divided by this factor before being used).  Default is 1.0.  */
void gnome_animator_set_playback_speed (GnomeAnimator * animator,
					gdouble speed);
gdouble gnome_animator_get_playback_speed (GnomeAnimator * animator);
void gnome_animator_set_range (GnomeAnimator * animator,
			       guint low, guint high);

G_END_DECLS

#endif /* _GNOME_ANIMATOR_H */
