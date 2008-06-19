/* gnome-appbar.h: statusbar/progress/minibuffer widget for Gnome apps
 * 
 * This version by Havoc Pennington
 * Based on MozStatusbar widget, by Chris Toshok
 * In turn based on GtkStatusbar widget, from Gtk+,
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * GtkStatusbar Copyright (C) 1998 Shawn T. Amundson
 * All rights reserved.
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

#ifndef __GNOME_APPBAR_H__
#define __GNOME_APPBAR_H__

#include <gtk/gtk.h>

#include "gnome-types.h"

G_BEGIN_DECLS

#define GNOME_TYPE_APPBAR            (gnome_appbar_get_type ())
#define GNOME_APPBAR(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), GNOME_TYPE_APPBAR, GnomeAppBar))
#define GNOME_APPBAR_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GNOME_TYPE_APPBAR, GnomeAppBarClass))
#define GNOME_IS_APPBAR(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GNOME_TYPE_APPBAR))
#define GNOME_IS_APPBAR_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_APPBAR))
#define GNOME_APPBAR_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GNOME_TYPE_APPBAR, GnomeAppBarClass))

/* Used in gnome-app-util to determine the capabilities of the appbar */
#define GNOME_APPBAR_HAS_STATUS(appbar) (gnome_appbar_get_status(GNOME_APPBAR(appbar)) != NULL)
#define GNOME_APPBAR_HAS_PROGRESS(appbar) (gnome_appbar_get_progress(GNOME_APPBAR(appbar)) != NULL)

typedef struct _GnomeAppBar        GnomeAppBar;
typedef struct _GnomeAppBarPrivate GnomeAppBarPrivate;
typedef struct _GnomeAppBarClass   GnomeAppBarClass;
typedef struct _GnomeAppBarMsg     GnomeAppBarMsg;

struct _GnomeAppBar
{
  GtkHBox parent_widget;

  /*< private >*/
  GnomeAppBarPrivate *_priv;
};

struct _GnomeAppBarClass
{
  GtkHBoxClass parent_class;

  /* Emitted when the user hits enter after a prompt. */
  void (* user_response) (GnomeAppBar * ab);
  /* Emitted when the prompt is cleared. */
  void (* clear_prompt)  (GnomeAppBar * ab);

  /* Padding for possible expansion */
  gpointer padding1;
  gpointer padding2;
};

#define GNOME_APPBAR_INTERACTIVE(ab) ((ab) ? (ab)->interactive : FALSE)

GType      gnome_appbar_get_type     	(void) G_GNUC_CONST;

GtkWidget* gnome_appbar_new          	(gboolean has_progress, 
					 gboolean has_status,
					 GnomePreferencesType interactivity);

/* Sets the status label without changing widget state; next set or push
   will destroy this permanently. */
void       gnome_appbar_set_status       (GnomeAppBar * appbar,
					  const gchar * status);

/* get the statusbar */
GtkWidget* gnome_appbar_get_status       (GnomeAppBar * appbar);

/* What to show when showing nothing else; defaults to nothing */
void	   gnome_appbar_set_default      (GnomeAppBar * appbar,
					  const gchar * default_status);

void       gnome_appbar_push             (GnomeAppBar * appbar,
					  const gchar * status);
/* OK to call on empty stack */
void       gnome_appbar_pop              (GnomeAppBar * appbar);

/* Nuke the stack. */
void       gnome_appbar_clear_stack      (GnomeAppBar * appbar);

/* Sugar function to set the percentage of the progressbar. */
void	     gnome_appbar_set_progress_percentage	  (GnomeAppBar *appbar,
							   gfloat percentage);
/* use GtkProgress functions on returned value */
GtkProgressBar* gnome_appbar_get_progress    (GnomeAppBar * appbar);

/* Reflect the current state of stack/default. Useful to force a set_status
   to disappear. */
void       gnome_appbar_refresh         (GnomeAppBar * appbar);

/* Put a prompt in the appbar and wait for a response. When the 
   user responds or cancels, a user_response signal is emitted. */
void       gnome_appbar_set_prompt          (GnomeAppBar * appbar,
					     const gchar * prompt,
					     gboolean modal);
/* Remove any prompt */
void       gnome_appbar_clear_prompt    (GnomeAppBar * appbar);

/* Get the response to the prompt, if any. Result must be g_free'd. */
gchar *    gnome_appbar_get_response    (GnomeAppBar * appbar);


G_END_DECLS

#endif /* __GNOME_APPBAR_H__ */







