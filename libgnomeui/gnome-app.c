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

#define LAYOUT_CONFIG_PATH      "Placement/Dock"

static void gnome_app_class_init (GnomeAppClass *class);
static void gnome_app_init       (GnomeApp      *app);
static void gnome_app_destroy    (GtkObject     *object);
static void gnome_app_realize    (GtkWidget *widget);

static gchar *read_layout_config  (GnomeApp *app);
static void   write_layout_config (GnomeApp *app, GnomeDockLayout *layout);
static void   layout_changed      (GtkWidget *widget, gpointer data);

static GtkWindowClass *parent_class;

static gchar *
read_layout_config (GnomeApp *app)
{
	gchar *s;

	gnome_config_push_prefix (app->prefix);
	s = gnome_config_get_string (LAYOUT_CONFIG_PATH);
	gnome_config_pop_prefix ();

	return s;
}

static void
write_layout_config (GnomeApp *app, GnomeDockLayout *layout)
{
	gchar *s;

	s = gnome_dock_layout_create_string (layout);
	gnome_config_push_prefix (app->prefix);
	gnome_config_set_string (LAYOUT_CONFIG_PATH, s);
	gnome_config_pop_prefix ();
	gnome_config_sync ();

	g_free (s);
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
	GtkWidgetClass *widget_class;

	object_class = (GtkObjectClass *) class;
	widget_class = (GtkWidgetClass *) class;

	parent_class = gtk_type_class (gtk_window_get_type ());

	object_class->destroy = gnome_app_destroy;

	widget_class->realize = gnome_app_realize;
}

static void
gnome_app_init (GnomeApp *app)
{
	app->accel_group = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (app), app->accel_group);
	
	app->vbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (app), app->vbox);

	app->dock = gnome_dock_new ();
	gtk_box_pack_start (GTK_BOX (app->vbox), app->dock,
			    TRUE, TRUE, 0);

	app->layout = gnome_dock_layout_new ();

	app->enable_layout_config = TRUE;
}

static void
gnome_app_realize (GtkWidget *widget)
{
	GnomeApp *app;

	app = GNOME_APP (widget);

	if (app->enable_layout_config) {
		gchar *s;

		/* Override the layout with the user's saved
                   configuration.  */
		s = read_layout_config (app);
		gnome_dock_layout_parse_string (app->layout, s);
		g_free (s);
	}

	gnome_dock_add_from_layout (GNOME_DOCK (app->dock), app->layout);

	gtk_widget_show (app->vbox);
	gtk_widget_show_all (app->dock);

	if (app->enable_layout_config)
		write_layout_config (app, app->layout);
			
	gtk_object_unref (GTK_OBJECT (app->layout));
	app->layout = NULL;

	if (GTK_WIDGET_CLASS (parent_class)->realize != NULL)
		(* GTK_WIDGET_CLASS (parent_class)->realize) (widget);

	gtk_signal_connect (GTK_OBJECT (app->dock),
			    "layout_changed",
			    GTK_SIGNAL_FUNC (layout_changed),
			    (gpointer) app);
}

static void
layout_changed (GtkWidget *w, gpointer data)
{
	GnomeApp *app;

	g_return_if_fail (GNOME_IS_APP (data));
	g_return_if_fail (GNOME_IS_DOCK (w));

	app = GNOME_APP (data);

	if (app->enable_layout_config) {
		GnomeDockLayout *layout;
		gchar *s;

		layout = gnome_dock_get_layout (GNOME_DOCK (app->dock));
		write_layout_config (app, layout);
		gtk_object_unref (GTK_OBJECT (layout));
	}
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
	g_return_if_fail (app->contents == NULL);

	gnome_dock_set_client_area (GNOME_DOCK (app->dock), contents);

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
	g_return_if_fail(app->layout != NULL);

	dock_item = gnome_dock_item_new (GNOME_APP_MENUBAR_NAME,
					 GNOME_DOCK_ITEM_BEH_EXCLUSIVE
					 | GNOME_DOCK_ITEM_BEH_NEVER_VERTICAL);
	gtk_container_set_border_width (GTK_CONTAINER (dock_item), 0);
	gtk_container_add (GTK_CONTAINER (dock_item), GTK_WIDGET (menubar));
	gnome_dock_item_set_shadow_type (GNOME_DOCK_ITEM (dock_item), GTK_SHADOW_NONE);

	gnome_dock_layout_add_item (app->layout,
				    GNOME_DOCK_ITEM (dock_item),
				    GNOME_DOCK_TOP,
				    -1, 0, 0);

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

void
gnome_app_add_toolbar (GnomeApp *app,
		       GtkToolbar *toolbar,
		       const gchar *name,
		       GnomeDockItemBehavior behavior,
		       GnomeDockPlacement placement,
		       gint band_num,
		       gint band_position,
		       gint offset)
{

	GtkWidget *dock_item;
	GtkAccelGroup *ag;

	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(toolbar != NULL);
	g_return_if_fail(app->layout != NULL);

	dock_item = gnome_dock_item_new (name, behavior);

	gtk_container_set_border_width (GTK_CONTAINER (toolbar), 1);
	gtk_container_add (GTK_CONTAINER (dock_item), GTK_WIDGET (toolbar));

	gnome_dock_layout_add_item (app->layout,
				    GNOME_DOCK_ITEM (dock_item),
				    placement,
				    band_num,
				    band_position,
				    offset);

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
	
	gtk_widget_show (GTK_WIDGET (toolbar));
	gtk_widget_show (GTK_WIDGET (dock_item));

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
	/* Making dock items containing toolbars use
	   `GNOME_DOCK_ITEM_BEH_EXCLUSIVE' is not really a
	   requirement.  We only do this for backwards compatibility.  */
	gnome_app_add_toolbar (app, toolbar,
			       GNOME_APP_TOOLBAR_NAME,
			       GNOME_DOCK_ITEM_BEH_EXCLUSIVE,
			       GNOME_DOCK_TOP,
			       0, 0, 0);
}


void
gnome_app_add_docked (GnomeApp *app,
		      GtkWidget *widget,
		      const gchar *name,
		      GnomeDockItemBehavior behavior,
		      GnomeDockPlacement placement,
		      gint band_num,
		      gint band_position,
		      gint offset)
{
	GtkWidget *item;

	g_return_if_fail (app->layout != NULL);

	item = gnome_dock_item_new (name, behavior);

	gnome_dock_layout_add_item (app->layout,
				    GNOME_DOCK_ITEM (item),
				    placement,
				    band_num,
				    band_position,
				    offset);
}

void
gnome_app_enable_layout_config (GnomeApp *app, gboolean enable)
{
	app->enable_layout_config = enable;
}
