/* -*- Mode: C; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * GnomeApp widget (C) 1998 Red Hat Software, The Free Software Foundation,
 * Miguel de Icaza, Federico Menu, Chris Toshok.
 *
 * Originally by Elliot Lee,
 *
 * improvements and rearrangement by Miguel,
 * and I don't know what you other people did :)
 *
 * Even more changes by Federico Mena.
 */

#ifndef GNOME_APP_H
#define GNOME_APP_H

#include <gtk/gtkmenubar.h>
#include <gtk/gtktoolbar.h>
#include <gtk/gtkwindow.h>
#include <libgnome/gnome-defs.h>


BEGIN_GNOME_DECLS


/* These define the possible positions for the menu bar and toolbars of the application window.
 * Note that a menu bar cannot be on the LEFT or RIGHT.
 */
typedef enum 
{
	GNOME_APP_POS_TOP,
	GNOME_APP_POS_BOTTOM,
	GNOME_APP_POS_LEFT,
	GNOME_APP_POS_RIGHT,
	GNOME_APP_POS_FLOATING
} GnomeAppWidgetPositionType;

/* Everything gets put into a table that looks like:
 *
 * XXX
 * ABC
 * YYY
 *
 * There's one table element on top, three in the middle, and one on
 * the bottom.
 *
 * Obviously you can change the positions of things as needed
 * using the supplied function.
 */

#define GNOME_APP(obj)         GTK_CHECK_CAST(obj, gnome_app_get_type(), GnomeApp)
#define GNOME_APP_CLASS(class) GTK_CHECK_CAST_CLASS(class, gnome_app_get_type(), GnomeAppClass)
#define GNOME_IS_APP(obj)      GTK_CHECK_TYPE(obj, gnome_app_get_type())

typedef struct _GnomeApp       GnomeApp;
typedef struct _GnomeAppClass  GnomeAppClass;

struct _GnomeApp {
	GtkWindow parent_object;

	char *name;			/* Application name */
	char *prefix;			/* Prefix for gnome-config */

        /* FIXME: Most of this stuff will have to be removed.  */
	GtkWidget *menubar;		/* The Menubar */
	GtkWidget *toolbar;		/* The Toolbar */
        GtkWidget *statusbar;		/* The Statusbar */
	GtkWidget *contents;		/* The contents (dock->client_area) */

	GtkWidget *vbox;	        /* The vbox widget that ties them all */

	GtkAccelGroup *accel_group;	/* Main accelerator group for this window (hotkeys live here)*/

        /* FIXME: this will be removed.  */
	/* Positions for the menubar and the toolbar */
	GnomeAppWidgetPositionType pos_menubar, pos_toolbar;

        /* The main dock widget.  */
        GtkWidget *dock;
};

struct _GnomeAppClass {
	GtkWindowClass parent_class;
};


/* Standard Gtk function */
GtkType gnome_app_get_type (void);

/* Create a new (empty) application window.  You must specify the application's name (used
 * internally as an identifier).  The window title can be left as NULL, in which case the window's
 * title will not be set.
 */
GtkWidget *gnome_app_new (gchar *appname, char *title);

/* Constructor for language bindings; you don't normally need this. */
void gnome_app_construct (GnomeApp *app, gchar *appname, char *title);

/* Sets the menu bar of the application window */
void gnome_app_set_menus (GnomeApp *app, GtkMenuBar *menubar);

/* Sets the main toolbar of the application window */
void gnome_app_set_toolbar (GnomeApp *app, GtkToolbar *toolbar);

/* Sets the status bar of the application window */
void gnome_app_set_statusbar (GnomeApp *app, GtkWidget *statusbar);

/* Sets the content area of the application window */
void gnome_app_set_contents (GnomeApp *app, GtkWidget *contents);

/* Sets the position of the toolbar */
void gnome_app_toolbar_set_position (GnomeApp *app, GnomeAppWidgetPositionType pos_toolbar);

/* Sets the position of the menu bar */
void gnome_app_menu_set_position (GnomeApp *app, GnomeAppWidgetPositionType pos_menu);


END_GNOME_DECLS

#endif /* GNOME_APP_H */
