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
 *
 * Toolbar separators and configurable relief by Andrew Veliath.
 *
 * Half-rewritten by Ettore Perazzoli to support GnomeDock.
 *
 */

#include "config.h"
#include <string.h>
#include <gtk/gtk.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-i18nP.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-config.h"
#include "libgnomeui/gnome-uidefs.h"
#include "libgnomeui/gnome-preferences.h"
#include "libgnomeui/gnome-dock.h"

#include "gnome-app.h"

#define DEFAULT_MENUBAR_NAME	"Menubar"
#define DEFAULT_TOOLBAR_NAME    "Toolbar"

static void gnome_app_class_init (GnomeAppClass *class);
static void gnome_app_init       (GnomeApp      *app);
static void gnome_app_destroy    (GtkObject     *object);

static void gnome_app_setpos_activate_menubar (GtkWidget *menu_item,
					       GnomeApp *app);
static void gnome_app_setpos_activate_toolbar (GtkWidget *menu_item,
					       GnomeApp *app);
static void gnome_app_reparent_handle_box     (GtkHandleBox *hb);


static GtkWindowClass *parent_class;


/* Keep in sync with GnomeAppWidgetPositionType */
static const char * const locations[] = {
	"top", "bottom", "left", "right", "float,"
};


/* Translates the specified orientation string into the proper enum value */
static GnomeAppWidgetPositionType
get_orientation (char *str)
{
	int i;
	
	for (i = 0; i < (int) (sizeof (locations) / sizeof (locations[0])); i++)
		if (strcasecmp (str, locations [i]) == 0)
			return i;

	/* If we dont recognize it => top */

	g_warning ("Unrecognized position type \"%s\", using GNOME_APP_POS_TOP", str);
	return GNOME_APP_POS_TOP;
}

GtkType
gnome_app_get_type (void)
{
	static GtkType gnomeapp_type = 0;

	if (!gnomeapp_type) {
		GtkTypeInfo gnomeapp_info = {
			"GnomeApp",
			sizeof (GnomeApp),
			sizeof (GnomeAppClass),
			(GtkClassInitFunc) gnome_app_class_init,
			(GtkObjectInitFunc) gnome_app_init,
			NULL, /* reserved_1 */
			NULL, /* reserved_2 */
			(GtkClassInitFunc) NULL
		};

		gnomeapp_type = gtk_type_unique (gtk_window_get_type (), &gnomeapp_info);
	}

	return gnomeapp_type;
}

static void
gnome_app_class_init (GnomeAppClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) class;

	parent_class = gtk_type_class (gtk_window_get_type ());

	object_class->destroy = gnome_app_destroy;
}

static void
gnome_app_init (GnomeApp *app)
{
	app->accel_group = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (app), app->accel_group);
	
	app->pos_menubar = app->pos_toolbar = GNOME_APP_POS_TOP;

	app->vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (app), app->vbox);

	app->dock = gnome_dock_new ();
	gtk_box_pack_start (GTK_BOX (app->vbox), app->dock,
			    TRUE, TRUE, 0);

	gtk_widget_show (app->dock);
	gtk_widget_show (app->vbox);
}

/**
 * gnome_app_new
 * @appname: Name of program, using in file names and paths.
 * @title: Window title for application.
 *
 * Description:
 * Create a new (empty) application window.  You must specify the
 * application's name (used internally as an identifier).
 * @title can be left as NULL, in which case the window's title will
 * not be set.
 *
 * Returns: Pointer to new GNOME app object
 **/

GtkWidget *
gnome_app_new(gchar *appname, char *title)
{
	GnomeApp *app;

	g_return_val_if_fail (appname != NULL, NULL);
		
	app = GNOME_APP (gtk_type_new (gnome_app_get_type ()));
	gnome_app_construct (app, appname, title);

	return GTK_WIDGET (app);
}


/**
 * gnome_app_construct
 * @app: Pointer to newly-created GNOME app object.
 * @appname: Name of program, using in file names and paths.
 * @title: Window title for application.
 *
 * Description:
 * Constructor for language bindings; you don't normally need this.
 **/

void 
gnome_app_construct (GnomeApp *app, gchar *appname, char *title)
{
	g_return_if_fail (appname != NULL);

	app->name = g_strdup (appname);
	app->prefix = g_copy_strings ("/", appname, "/", NULL);
	
	if (title)
		gtk_window_set_title (GTK_WINDOW (app), title);
}

static void
gnome_app_destroy (GtkObject *object)
{
	GnomeApp *app;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_APP (object));

	app = GNOME_APP (object);

	g_free (app->name);
	g_free (app->prefix);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/**
 * gnome_app_menu_set_position
 * @app: Pointer to GNOME app object.
 * @pos_menubar: Indicates position of menu bar.
 *
 * Description:
 * Sets the position of the tool bar within the GNOME app's main window.
 * Possible @pos_menubar values
 * are %GNOME_APP_POS_TOP, %GNOME_APP_POS_BOTTOM, and
 * %GNOME_APP_POS_FLOATING.
 **/

void
gnome_app_menu_set_position (GnomeApp *app, GnomeAppWidgetPositionType pos_menubar)
{
	g_message ("gnome_app_menu_set_position is deprecated.");
}


/**
 * gnome_app_toolbar_set_position
 * @app: Pointer to GNOME app object.
 * @pos_toolbar: Indicates position of tool bar.
 *
 * Description:
 * Sets the position of the tool bar within the GNOME app's main window.
 * Possible @pos_menubar values
 * are %GNOME_APP_POS_TOP, %GNOME_APP_POS_BOTTOM, %GNOME_APP_POS_LEFT,
 * %GNOME_APP_POS_RIGHT, and %GNOME_APP_POS_FLOATING.
 **/

void
gnome_app_toolbar_set_position (GnomeApp *app, GnomeAppWidgetPositionType pos_toolbar)
{
	g_message ("gnome_app_toolbar_set_position is deprecated.");
}

/* These are used for knowing where to pack the contents into the
   table, so we don't have to recompute every time we set_contents.
   The first dimension is the current position of the menubar, and the
   second dimension is the current position of the toolbar. Put it all
   together and you find out the top-left and bottom-right coordinates
   of the contents inside the placement table.
*/
static const gint startxs[2][4] = {
	{ 0, 0, 1, 0 },
	{ 0, 0, 1, 0 }
};

static const gint endxs[2][4] = {
	{ 3, 3, 3, 2 },
	{ 3, 3, 3, 2 }
};

static const gint startys[2][4] = {
	{ 2, 1, 1, 1 },
	{ 1, 0, 0, 0 }
};

static const gint endys[2][4] = {
	{ 3, 2, 3, 3 },
	{ 2, 1, 2, 2 }
};


/**
 * gnome_app_set_contents
 * @app: Pointer to GNOME app object.
 * @contents: Widget to be application content area.
 *
 * Description:
 * Sets the content area of the GNOME app's main window.
 **/

void
gnome_app_set_contents (GnomeApp *app, GtkWidget *contents)
{
	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP(app));
	g_return_if_fail (app->dock != NULL);

	gnome_dock_set_client_area (GNOME_DOCK (app->dock), contents);

	if (contents != NULL)
		gtk_widget_show (contents);

	app->contents = contents;
}

/**
 * gnome_app_set_menus
 * @app: Pointer to GNOME app object.
 * @menubar: Menu bar widget for main app window.
 *
 * Description:
 * Sets the menu bar of the application window.
 **/

void
gnome_app_set_menus (GnomeApp *app, GtkMenuBar *menubar)
{
	GtkWidget *dock_item;
	GtkAccelGroup *ag;

	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(app->menubar == NULL);
	g_return_if_fail(menubar != NULL);
	g_return_if_fail(GTK_IS_MENU_BAR(menubar));

	dock_item = gnome_dock_item_new (DEFAULT_MENUBAR_NAME,
					 GNOME_DOCK_ITEM_BEH_EXCLUSIVE
					 | GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL);
	gtk_container_set_border_width (GTK_CONTAINER (dock_item), 0);
	gtk_container_add (GTK_CONTAINER (dock_item), GTK_WIDGET (menubar));
	gnome_dock_item_set_shadow_type (GNOME_DOCK_ITEM (dock_item), GTK_SHADOW_NONE);

	gnome_dock_add_item (GNOME_DOCK (app->dock), dock_item,
			     GNOME_DOCK_POS_TOP, 0, 0, 0, TRUE);

	app->menubar = GTK_WIDGET (menubar);

	gtk_widget_show (GTK_WIDGET (menubar));
	gtk_widget_show (GTK_WIDGET (dock_item));

	/* Configure menu to gnome preferences, if possible.
	 * (sync to gnome-app-helper.c:gnome_app_fill_menu_custom) */
	if (!gnome_preferences_get_menubar_relief ())
		gtk_menu_bar_set_shadow_type (GTK_MENU_BAR (app->menubar), GTK_SHADOW_NONE);

	ag = gtk_object_get_data(GTK_OBJECT(app), "GtkAccelGroup");
	if (ag && !g_slist_find(gtk_accel_groups_from_object (GTK_OBJECT (app)), ag))
	        gtk_window_add_accel_group(GTK_WINDOW(app), ag);
}


/**
 * gnome_app_set_toolbar
 * @app: Pointer to GNOME app object.
 * @toolbar: Toolbar widget for main app window.
 *
 * Description:
 * Sets the main toolbar of the application window.
 **/

void
gnome_app_set_toolbar (GnomeApp *app,
		       GtkToolbar *toolbar)
{
	GtkWidget *dock_item;
	GtkAccelGroup *ag;

	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(toolbar != NULL);
	g_return_if_fail(app->toolbar == NULL);

	/* Having dock items containing toolbars use
	   `GNOME_DOCK_ITEM_BEH_EXCLUSIVE' is not really a
	   requirement.  We only do this for backwards compatibility.  */
	dock_item = gnome_dock_item_new (DEFAULT_TOOLBAR_NAME,
					 GNOME_DOCK_ITEM_BEH_EXCLUSIVE);
	gtk_container_set_border_width (GTK_CONTAINER (toolbar), 1);
	gtk_container_add (GTK_CONTAINER (dock_item), GTK_WIDGET (toolbar));

	gnome_dock_add_item (GNOME_DOCK (app->dock), dock_item,
			     GNOME_DOCK_POS_TOP, 1, 0, 0, TRUE);
	if (gnome_preferences_get_toolbar_relief ()) {
		gtk_container_set_border_width (GTK_CONTAINER (dock_item), 1);
	} else {
		gnome_dock_item_set_shadow_type (GNOME_DOCK_ITEM (dock_item),
						 GTK_SHADOW_NONE);
	}
	
	/* Configure toolbar to gnome preferences, if possible.  (Sync
	   to gnome_app_helper.c:gnome_app_toolbar_custom.)  */
	if (gnome_preferences_get_toolbar_lines ()) {
		gtk_toolbar_set_space_style (toolbar, GTK_TOOLBAR_SPACE_LINE);
		gtk_toolbar_set_space_size (toolbar, GNOME_PAD * 2);
	} else {
		gtk_toolbar_set_space_size (toolbar, GNOME_PAD);
	}

	if (!gnome_preferences_get_toolbar_relief_btn ())
		gtk_toolbar_set_button_relief(toolbar, GTK_RELIEF_NONE);
	
	if (!gnome_preferences_get_toolbar_labels ())
		gtk_toolbar_set_style (toolbar, GTK_TOOLBAR_ICONS);
	
	app->toolbar = GTK_WIDGET (toolbar);

	gtk_widget_show (GTK_WIDGET (toolbar));
	gtk_widget_show (GTK_WIDGET (dock_item));

	ag = gtk_object_get_data(GTK_OBJECT(app), "GtkAccelGroup");
	if (ag && !g_slist_find(gtk_accel_groups_from_object (GTK_OBJECT (app)), ag))
	        gtk_window_add_accel_group(GTK_WINDOW(app), ag);
}


/**
 * gnome_app_set_statusbar
 * @app: Pointer to GNOME app object
 * @statusbar: Statusbar widget for main app window
 *
 * Description:
 * Sets the status bar of the application window.
 **/

void
gnome_app_set_statusbar (GnomeApp *app,
		         GtkWidget *statusbar)
{
	GtkWidget *frame;
	GtkWidget *hbox;

	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(statusbar != NULL);
	g_return_if_fail(app->statusbar == NULL);

	app->statusbar = GTK_WIDGET(statusbar);
	gtk_widget_show(app->statusbar);

	frame = gtk_frame_new(NULL);
	gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_OUT);
	gtk_container_set_border_width(GTK_CONTAINER(frame), 0);
	gtk_widget_show(frame);

	hbox = gtk_hbox_new(FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 1);
	gtk_box_pack_start(GTK_BOX(hbox), statusbar, TRUE, TRUE, 0);
	gtk_widget_show(hbox);

	gtk_container_add(GTK_CONTAINER(frame), hbox);

	gtk_box_pack_start (GTK_BOX (app->vbox), frame, FALSE, FALSE, 0);
}
