/* WARNING ____ IMMATURE API ____ liable to change */

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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */

#ifndef _GNOME_ANIMATOR_H
#define _GNOME_ANIMATOR_H

#include <gdk/gdk.h>
#include <gtk/gtkwidget.h>
#include <gdk_imlib.h>
#include <libgnome/gnome-defs.h>
#include "gnome-pixmap.h"

BEGIN_GNOME_DECLS

/* ------------------------------------------------------------------------- */

/* GnomeAnimator class, derived from GtkWidget.  */

#define GNOME_TYPE_ANIMATOR            (gnome_animator_get_type())
#define GNOME_ANIMATOR(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_ANIMATOR, GnomeAnimator))
#define GNOME_ANIMATOR_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ANIMATOR, GnomeAnimatorClass))
#define GNOME_IS_ANIMATOR(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_ANIMATOR))
#define GNOME_IS_ANIMATOR_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ANIMATOR))

/* Current animator status.  */
typedef enum
{
    GNOME_ANIMATOR_STATUS_STOPPED,
    GNOME_ANIMATOR_STATUS_RUNNING
} GnomeAnimatorStatus;

/* Looping type.  The behavior depends on the playback direction,
   forwards or (backwards).  */
typedef enum
{
    /* No loop: after the last (first) frame is played, the animation
       is stopped.  */
    GNOME_ANIMATOR_LOOP_NONE,

    /* After the last (first) frame is played, restart from the first
       (last) frame.  */
    GNOME_ANIMATOR_LOOP_RESTART,

    /* After the last (first) frame is played, the playback direction
       is reversed.  */
    GNOME_ANIMATOR_LOOP_PING_PONG
} GnomeAnimatorLoopType;

typedef struct _GnomeAnimator GnomeAnimator;
typedef struct _GnomeAnimatorClass GnomeAnimatorClass;

/* Private stuff.  */
typedef struct _GnomeAnimatorFrame GnomeAnimatorFrame;
typedef struct _GnomeAnimatorPrivate GnomeAnimatorPrivate;

struct _GnomeAnimator
  {
    GtkWidget widget;

    guint num_frames;

    guint current_frame_number;

    GnomeAnimatorStatus status;

    GnomeAnimatorLoopType loop_type;

    gint playback_direction;

    gdouble playback_speed;

    GnomeAnimatorPrivate *privat;
  };

struct _GnomeAnimatorClass
  {
    GtkWidgetClass parent_class;
  };

/* ------------------------------------------------------------------------- */

guint    gnome_animator_get_type                      (void);

/* Create a new animator of the specified size.  */
GtkWidget *gnome_animator_new_with_size               (guint width,
                                                       guint height);

/* Set/get loop type (See `GnomeAnimatorLoopType').  */
void     gnome_animator_set_loop_type                 (GnomeAnimator *animator,
                                                       GnomeAnimatorLoopType loop_type);
GnomeAnimatorLoopType gnome_animator_get_loop_type    (GnomeAnimator *animator);

/* Set/get playback direction.  A positive value means "forwards"; a
   negative one means "backwards".  */
void     gnome_animator_set_playback_direction        (GnomeAnimator *animator,
                                                       gint playback_direction);
gint     gnome_animator_get_playback_direction        (GnomeAnimator *animator);

/* Append a frame from `image', rendering it at the specified size.  */
gboolean gnome_animator_append_frame_from_imlib_at_size (GnomeAnimator *animator,
                                                       GdkImlibImage *image,
                                                       gint x_offset,
                                                       gint y_offset,
                                                       guint32 interval,
                                                       guint width,
                                                       guint height);

/* Append a frame from `image', rendering it at its natural size.  */
gboolean gnome_animator_append_frame_from_imlib       (GnomeAnimator *animator,
                                                       GdkImlibImage *image,
                                                       gint x_offset,
                                                       gint y_offset,
                                                       guint32 interval);

/* Append a frame from file `name', rendering it at the specified size.  */
gboolean gnome_animator_append_frame_from_file_at_size (GnomeAnimator *animator,
                                                       const gchar *name,
                                                       gint x_offset,
                                                       gint y_offset,
                                                       guint32 interval,
                                                       guint width,
                                                       guint height);

/* Append a frame from file `name', rendering it at its natural size.  */
gboolean gnome_animator_append_frame_from_file        (GnomeAnimator *animator,
                                                       const gchar *name,
                                                       gint x_offset,
                                                       gint y_offset,
                                                       guint32 interval);

/* Crop `image' into frames `x_unit' pixels wide, and append them as
   frames to the animator with the specified `interval' and offsets.
   Each frame is rendered at the specified size.  */
gboolean gnome_animator_append_frames_from_imlib_at_size (GnomeAnimator *animator,
                                                       GdkImlibImage *image,
                                                       gint x_offset,
                                                       gint y_offset,
                                                       guint32 interval,
                                                       gint x_unit,
                                                       guint width,
                                                       guint height);

/* Crop `image' into frames `x_unit' pixels wide, and append them as
   frames to the animator with the specified `interval' and offsets.
   Each frame is rendered at its natural size.  */
gboolean gnome_animator_append_frames_from_imlib      (GnomeAnimator *animator,
                                                       GdkImlibImage *image,
                                                       gint x_offset,
                                                       gint y_offset,
                                                       guint32 interval,
                                                       gint x_unit);

/* Load an image from `name', crop it into frames `x_unit' pixels
   wide, and append them as frames to the animator.  Each frame is
   rendered at the specified size.  */
gboolean gnome_animator_append_frames_from_file_at_size (GnomeAnimator *animator,
                                                       const gchar *name,
                                                       gint x_offset,
                                                       gint y_offset,
                                                       guint32 interval,
                                                       gint x_unit,
                                                       guint width,
                                                       guint height);

/* Load an image from `name', crop it into frames `x_unit' pixels
   wide, and append them as frames to the animator.  Each frame is
   rendered at its natural size.  */
gboolean gnome_animator_append_frames_from_file       (GnomeAnimator *animator,
                                                       const gchar *name,
                                                       gint x_offset,
                                                       gint y_offset,
                                                       guint32 interval,
                                                       gint x_unit);

/* Append `pixmap' to animator's list of frames.  */
gboolean gnome_animator_append_frame_from_gnome_pixmap (GnomeAnimator *animator,
                                                        GnomePixmap *pixmap,
                                                        gint x_offset,
                                                        gint y_offset,
                                                        guint32 interval);

/* Start playing the animation in the GTK loop.  */
void     gnome_animator_start                         (GnomeAnimator *animator);

/* Stop playing the animation in the GTK loop.  */
void     gnome_animator_stop                          (GnomeAnimator *animator);

/* Advance the animation by `num' frames.  A positive value uses the
   specified playback direction; a negative one goes in the opposite
   direction.  If the loop type is `GNOME_ANIMATOR_LOOP_NONE' and the
   value causes the frame counter to overflow, `FALSE' is returned and
   the animator is stopped; otherwise, `TRUE' is returned.  */
gboolean gnome_animator_advance                       (GnomeAnimator *animator,
                                                       gint num);

/* Set the number of current frame.  The result is immediately
   visible.  */
void     gnome_animator_goto_frame                    (GnomeAnimator *animator,
                                                       guint frame_number);

/* Get the number of current frame.  */
guint    gnome_animator_get_current_frame_number      (GnomeAnimator *animator);

/* Get the animator status.  */
GnomeAnimatorStatus gnome_animator_get_status         (GnomeAnimator *animator);

/* Set/get speed factor (the higher, the faster: the `interval' value
   is divided by this factor before being used).  Default is 1.0.  */
void     gnome_animator_set_playback_speed            (GnomeAnimator *animator,
                                                       gdouble speed);
gdouble  gnome_animator_get_playback_speed            (GnomeAnimator *animator);

END_GNOME_DECLS

#endif /* _GNOME_ANIMATOR_H */
