/* My vague and ugly attempt at adding popup menus to GnomeUI */
/* By: Mark Crichton <mcrichto@purdue.edu> */
/* Written under the heavy infulence of whatever was playing */
/* in my CDROM drive at the time... */
/* First pass: July 8, 1998 */

/* gnome-popupmenu.h
 * 
 * Copyright (C) 1998, Mark Crichton
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

#include <config.h>
#include <glib.h>
#include <gdk/gdkkeysyms.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-config.h"
#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-popup-menu.h"
#include <gtk/gtk.h>

/* FIXME: all this file is completely broken.  I will rewrite it in a couple of days - Federico */

void
gnome_app_create_popup_menus_custom (GnomeApp *app,
		 		     GtkWidget *child,
		 		     GnomeUIInfo *menuinfo,
		 		     gpointer handler,
		  		     GnomeUIBuilderData *uibdata)

{
#if 0
#ifdef GTK_HAVE_FEATURES_1_1_0

	GtkWidget *menubar;
	GtkWidget *oldparent;
	GtkWidget *eb;
	GtkArg	  *temparg;
	guint	  nargs = 0;

	g_return_if_fail(app !=NULL);
	g_return_if_fail(GNOME_IS_APP(app));

	g_warning("We're here");
	menubar = gtk_menu_new();
	eb = gtk_event_box_new();
	gtk_widget_set_events(eb, GDK_BUTTON_PRESS_MASK);

	gtk_signal_connect(GTK_OBJECT(eb), "button_press_event",
			   GTK_SIGNAL_FUNC(gnome_app_rightclick_popup), app);

	/* Now, the magic begins... */

	gtk_widget_ref(child); 
	oldparent = child->parent;

	/* first, get all the args needed for the new child */

	temparg = gtk_container_query_child_args(GTK_WIDGET_TYPE(oldparent), NULL, &nargs);
	gtk_container_child_getv(GTK_CONTAINER(oldparent), child, nargs, temparg);

	/* Now we can remove the child */

	gtk_container_remove(GTK_CONTAINER(oldparent), child);

	/* GTK_WIDGET_NO_WINDOW() */
	/* also use query_args and addv */
	/* SLICK...thanks Tim Janik! */

	gtk_container_add(GTK_CONTAINER(oldparent), eb);
	gtk_container_add(GTK_CONTAINER(eb), child);
	gtk_container_child_setv(GTK_CONTAINER(oldparent), eb, nargs, temparg);

	gtk_widget_show(eb);
	gtk_widget_show(child);

	gtk_widget_unref(child); 
	g_free (temparg);

	/* Now fill the keys... */

	gtk_object_set_data (GTK_OBJECT(eb), "gnome_popup_menu", menubar);
	gtk_object_set_data (GTK_OBJECT(eb), "gnome_popup_old_handler", handler);
	gtk_object_set_data (GTK_OBJECT(eb), "gnome_popup_child", child);

	if (menuinfo)
		gnome_app_do_menu_creation(app, menubar, 0, menuinfo, uibdata);
#endif /* GTK_HAVE_FEATURES_1_1_0 */
#endif
}

void
gnome_app_create_popup_menus (GnomeApp *app, GtkWidget *child,
			      GnomeUIInfo *menudata, gpointer handler)
{
#if 0
#ifdef GTK_HAVE_FEATURES_1_1_0

	GnomeUIBuilderData uidata = { GNOME_UISIGFUNC(gnome_app_do_ui_signal_connect),
				      NULL, FALSE, NULL, NULL };

	gnome_app_create_popup_menus_custom(app, child, menudata, handler,
					    &uidata);
#endif
#endif
}
