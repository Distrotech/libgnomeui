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

#include <gtk/gtk.h>

#include "gnome-appbar.h"
#include "gnome-uidefs.h"

static void gnome_appbar_class_init               (GnomeAppBarClass *class);
static void gnome_appbar_init                     (GnomeAppBar      *ab);
static void gnome_appbar_destroy                  (GtkObject         *object);
     
static GtkContainerClass *parent_class;

guint      
gnome_appbar_get_type ()
{
  static guint ab_type = 0;

  if (!ab_type)
    {
      GtkTypeInfo ab_info =
      {
        "GnomeAppBar",
        sizeof (GnomeAppBar),
        sizeof (GnomeAppBarClass),
        (GtkClassInitFunc) gnome_appbar_class_init,
        (GtkObjectInitFunc) gnome_appbar_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL,
      };

      ab_type = gtk_type_unique (gtk_hbox_get_type (), &ab_info);
    }

  return ab_type;
}

static void
gnome_appbar_class_init (GnomeAppBarClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;
  container_class = (GtkContainerClass *) class;

  parent_class = gtk_type_class (gtk_hbox_get_type ());

  object_class->destroy = gnome_appbar_destroy;
}

static GSList * 
stringstack_push(GSList * stringstack, const gchar * s)
{
  return g_slist_prepend(stringstack, g_strdup(s));
}

static GSList *
stringstack_pop(GSList * stringstack)
{
  /* Could be optimized */
  if (stringstack) {
    g_free(stringstack->data);
    return g_slist_remove(stringstack, stringstack->data);
  }
  else return NULL;
}

static const gchar *
stringstack_top(GSList * stringstack)
{
  if (stringstack) return stringstack->data;
  else return NULL;
}

static void
stringstack_free(GSList * stringstack)
{
  GSList * tmp = stringstack;
  while (tmp) {
    g_free(tmp->data);
    tmp = g_slist_next(tmp);
  }
  g_slist_free(stringstack);
}

static void
gnome_appbar_init (GnomeAppBar *ab)
{
  ab->default_status = NULL;
  ab->status_stack   = NULL;
}

GtkWidget* 
gnome_appbar_new (gboolean has_progress,
		  gboolean has_status)
{
  GtkBox *box;
  GnomeAppBar * ab = gtk_type_new (gnome_appbar_get_type ());

  /* Has to have either a progress bar or a status bar */
  g_return_val_if_fail( (has_progress == TRUE) || (has_status == TRUE), NULL );

  box = GTK_BOX (ab);

  box->spacing = GNOME_PAD_SMALL;
  box->homogeneous = FALSE;

  if ( has_progress ) {
    ab->progress = gtk_progress_bar_new();
    gtk_box_pack_start (box, ab->progress, FALSE, FALSE, 0);
    gtk_widget_show (ab->progress);
  }
  else ab->progress = NULL;

  if ( has_status ) {
    GtkWidget * frame;
    
    frame = gtk_frame_new (NULL);
    gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_IN);

    ab->status = gtk_label_new ("");
    /*    gtk_misc_set_alignment (GTK_MISC (ab->status), 0.0, 0.0); */

    gtk_box_pack_start (box, frame, TRUE, TRUE, 0);
    gtk_container_add (GTK_CONTAINER(frame), ab->status);

    gtk_widget_show (ab->status);
    gtk_widget_show (frame);
  }
  else ab->status = NULL;

  return GTK_WIDGET(ab);
}

void 
gnome_appbar_refresh           (GnomeAppBar * appbar)
{
  g_return_if_fail(appbar != NULL);
  g_return_if_fail(GNOME_IS_APPBAR(appbar));

  if (appbar->status_stack)
    gnome_appbar_set_status(appbar, stringstack_top(appbar->status_stack));
  else if (appbar->default_status)
    gnome_appbar_set_status(appbar, appbar->default_status);
  else 
    gnome_appbar_set_status(appbar, "");
}

void       
gnome_appbar_set_status       (GnomeAppBar * appbar,
			       const gchar * status)
{
  g_return_if_fail(appbar != NULL);
  g_return_if_fail(status != NULL);
  g_return_if_fail(GNOME_IS_APPBAR(appbar));

  gtk_label_set(GTK_LABEL(appbar->status), status);
}

void	   
gnome_appbar_set_default      (GnomeAppBar * appbar,
			       const gchar * default_status)
{
  g_return_if_fail(appbar != NULL);
  g_return_if_fail(default_status != NULL);
  g_return_if_fail(GNOME_IS_APPBAR(appbar));
  
  if (appbar->default_status) g_free(appbar->default_status);
  appbar->default_status = g_strdup(default_status);
  gnome_appbar_refresh(appbar);
}

void       
gnome_appbar_push             (GnomeAppBar * appbar,
			       const gchar * status)
{
  g_return_if_fail(appbar != NULL);
  g_return_if_fail(status != NULL);
  g_return_if_fail(GNOME_IS_APPBAR(appbar));

  appbar->status_stack = stringstack_push(appbar->status_stack, status);
  gnome_appbar_refresh(appbar);
}

void       
gnome_appbar_pop              (GnomeAppBar * appbar)
{
  g_return_if_fail(appbar != NULL);
  g_return_if_fail(GNOME_IS_APPBAR(appbar));
  
  appbar->status_stack = stringstack_pop(appbar->status_stack);
  gnome_appbar_refresh(appbar);
}

void       
gnome_appbar_clear_stack      (GnomeAppBar * appbar)
{
  g_return_if_fail(appbar != NULL);
  g_return_if_fail(GNOME_IS_APPBAR(appbar));

  stringstack_free(appbar->status_stack);
  appbar->status_stack = NULL;
  gnome_appbar_refresh(appbar);
}

void
gnome_appbar_set_progress(GnomeAppBar *ab,
			  gfloat percentage)
{
  g_return_if_fail(ab != NULL);
  g_return_if_fail(ab->progress != NULL);
  g_return_if_fail(GNOME_IS_APPBAR(ab));

  gtk_progress_bar_update(GTK_PROGRESS_BAR(ab->progress), percentage);
}

static void
gnome_appbar_destroy (GtkObject *object)
{
  GnomeAppBar *ab;
  GnomeAppBarClass *class;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GNOME_IS_APPBAR (object));

  ab = GNOME_APPBAR (object);
  class = GNOME_APPBAR_CLASS (GTK_OBJECT (ab)->klass);

  gnome_appbar_clear_stack(ab);
  g_free(ab->default_status);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


