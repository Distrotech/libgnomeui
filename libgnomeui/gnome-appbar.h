/* gnome-appbar.h: statusbar/progress/minibuffer widget for Gnome apps
 * 
 * This version by Havoc Pennington
 * Based on MozStatusbar widget, by Chris Toshok
 * In turn based on GtkStatusbar widget, from Gtk+,
 * Copyright (C) 1995-1997 Peter Mattis, Spencer Kimball and Josh MacDonald
 * GtkStatusbar Copyright (C) 1998 Shawn T. Amundson
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

#ifndef __GNOME_APPBAR_H__
#define __GNOME_APPBAR_H__

#include <gtk/gtkhbox.h>

#include "libgnome/gnome-defs.h"

BEGIN_GNOME_DECLS

#define GNOME_APPBAR(obj)          GTK_CHECK_CAST (obj, gnome_appbar_get_type (), GnomeAppBar)
#define GNOME_APPBAR_CLASS(klass)  GTK_CHECK_CLASS_CAST (klass, gnome_appbar_get_type (), GnomeAppBarClass)
#define GNOME_IS_APPBAR(obj)       GTK_CHECK_TYPE (obj, gnome_appbar_get_type ())

/* Used in gnome-app-util to determine the capabilities of the appbar */
#define GNOME_APPBAR_HAS_STATUS(appbar) (GNOME_APPBAR(appbar)->status != NULL)
#define GNOME_APPBAR_HAS_PROGRESS(appbar) (GNOME_APPBAR(appbar)->progress != NULL)

typedef struct _GnomeAppBar      GnomeAppBar;
typedef struct _GnomeAppBarClass GnomeAppBarClass;
typedef struct _GnomeAppBarMsg GnomeAppBarMsg;

struct _GnomeAppBar
{
  GtkHBox parent_widget;

  /* Private; there's no guarantee on the type of these in the
     future. Statusbar could be a label, entry, GtkStatusbar, or
     something else; progress could be a label or progress bar; it's
     all up in the air for now. */
  GtkWidget * progress;
  GtkWidget * status;

  /* Keep it simple; no contexts. 
     if (status_stack) display_top_of_stack;
     else if (default_status) display_default;
     else display_nothing;      */
  /* Also private by the way */
  GSList * status_stack;
  gchar  * default_status;
};

struct _GnomeAppBarClass
{
  GtkHBoxClass parent_class;
};

guint      gnome_appbar_get_type     	(void);

GtkWidget* gnome_appbar_new          	(gboolean has_progress, 
					 gboolean has_status);

/* Sets the status label without changing widget state; next set or push
   will destroy this permanently. */
void       gnome_appbar_set_status       (GnomeAppBar * appbar,
					  const gchar * status);

/* What to show when showing nothing else; defaults to nothing */
void	   gnome_appbar_set_default      (GnomeAppBar * appbar,
					  const gchar * default_status);

void       gnome_appbar_push             (GnomeAppBar * appbar,
					  const gchar * status);
/* OK to call on empty stack */
void       gnome_appbar_pop              (GnomeAppBar * appbar);

/* Nuke the stack. */
void       gnome_appbar_clear_stack      (GnomeAppBar * appbar);

void	   gnome_appbar_set_progress	(GnomeAppBar * statusbar,
					 gfloat percentage);

/* Reflect the current state of stack/default. Useful to force a set_status
   to disappear. */
void       gnome_appbar_refresh         (GnomeAppBar * appbar);

END_GNOME_DECLS

#endif /* __GNOME_APPBAR_H__ */







