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
#include "gnome-app.h"

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
	app->table = gtk_table_new (4, 3, FALSE);
	gtk_container_add (GTK_CONTAINER (app), app->table);
	gtk_widget_show (app->table);

	app->accel_group = gtk_accel_group_new ();
	gtk_window_add_accel_group (GTK_WINDOW (app), app->accel_group);
	
	app->pos_menubar = app->pos_toolbar = GNOME_APP_POS_TOP;
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

/* Rearranges the children of the application window to their newly- configured positions */
static void
gnome_app_configure_positions (GnomeApp *app)
{
	/* 1.  The menubar: can go on top or bottom */
	if (app->menubar) {
		GtkWidget *handlebox = app->menubar->parent;

		gtk_widget_ref (handlebox);
		gtk_widget_hide (handlebox);

		if (app->menubar->parent->parent)
			gtk_container_remove (GTK_CONTAINER(app->table), handlebox);

		gtk_table_attach (GTK_TABLE (app->table),
				  handlebox,
				  0, 3,
				  (app->pos_menubar == GNOME_APP_POS_TOP) ? 0 : 2,
				  (app->pos_menubar == GNOME_APP_POS_TOP) ? 1 : 3,
				  GTK_EXPAND | GTK_FILL | GTK_SHRINK,
				  0,
				  0, 0);

		gtk_widget_show (handlebox);
		gtk_widget_unref (handlebox);
	}

	/* 2. the toolbar */
	if (app->toolbar) {
		GtkWidget *handlebox = app->toolbar->parent;
		int offset = 0;

		gtk_widget_ref (handlebox);
		gtk_widget_hide (handlebox);

		if (app->toolbar->parent->parent)
			gtk_container_remove (GTK_CONTAINER (app->table), handlebox);

		if (app->pos_menubar == GNOME_APP_POS_TOP)
			offset = 1;

		if ((app->pos_toolbar == GNOME_APP_POS_LEFT) || (app->pos_toolbar == GNOME_APP_POS_RIGHT)) {
			gtk_table_attach (GTK_TABLE (app->table),
					  handlebox,
					  (app->pos_toolbar == GNOME_APP_POS_LEFT) ? 0 : 2,
					  (app->pos_toolbar == GNOME_APP_POS_LEFT) ? 1 : 3,
					  offset, 3,
					  0,
					  GTK_EXPAND | GTK_FILL | GTK_SHRINK,
					  0, 0);
		} else {
			gint moffset;

			if (app->pos_menubar == app->pos_toolbar)
				moffset = (app->pos_toolbar == GNOME_APP_POS_TOP) ? 1 : -1;
			else
				moffset = 0;

			gtk_table_attach (GTK_TABLE (app->table),
					  handlebox,
					  0, 3,
					  ((app->pos_toolbar == GNOME_APP_POS_TOP) ? 0 : 2) + moffset,
					  ((app->pos_toolbar == GNOME_APP_POS_TOP) ? 1 : 3) + moffset,
					  GTK_EXPAND | GTK_FILL | GTK_SHRINK,
					  0,
					  0, 0);
		}

		gnome_app_configure_toolbar (GTK_TOOLBAR (app->toolbar));
		gtk_widget_show (handlebox);
		gtk_widget_unref (handlebox);
	}

	/* Repack any contents of ours */
	if (app->contents)
		gnome_app_set_contents(app, app->contents);
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
	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (app->menubar != NULL);
	g_return_if_fail ((pos_menubar == GNOME_APP_POS_TOP) || (pos_menubar == GNOME_APP_POS_BOTTOM));

	app->pos_menubar = pos_menubar;

	gnome_app_configure_positions (app);

	/* Save the new setting */

	gnome_config_push_prefix (app->prefix);
	gnome_config_set_string ("Placement/Menu", locations [pos_menubar]);
	gnome_config_pop_prefix ();
	gnome_config_sync ();
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
	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (app->toolbar != NULL);

	app->pos_toolbar = pos_toolbar;

	if ((pos_toolbar == GNOME_APP_POS_LEFT) || (pos_toolbar == GNOME_APP_POS_RIGHT)) {
		gtk_toolbar_set_orientation (GTK_TOOLBAR (app->toolbar), GTK_ORIENTATION_VERTICAL);
		gnome_app_configure_positions (app);
	} else {
		gtk_toolbar_set_orientation (GTK_TOOLBAR (app->toolbar), GTK_ORIENTATION_HORIZONTAL);
		gnome_app_configure_positions (app);
	}

	/* Save the new setting */

	gnome_config_push_prefix (app->prefix);
	gnome_config_set_string ("Placement/Toolbar", locations [pos_toolbar]);
	gnome_config_pop_prefix ();
	gnome_config_sync ();
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

	/* We ref the contents in case app->contents == contents, so that the container_remove will
	 * not destroy the widget.
	 */

	if (contents)
		gtk_widget_ref (contents);

	if (app->contents != NULL)
		gtk_container_remove (GTK_CONTAINER (app->table), app->contents);

	if (contents) {
		gtk_table_attach (GTK_TABLE (app->table),
				  contents,
				  startxs[app->pos_menubar][app->pos_toolbar],
				  endxs[app->pos_menubar][app->pos_toolbar],
				  startys[app->pos_menubar][app->pos_toolbar],
				  endys[app->pos_menubar][app->pos_toolbar],
				  GTK_EXPAND | GTK_FILL | GTK_SHRINK,
				  GTK_EXPAND | GTK_FILL | GTK_SHRINK,
				  0, 0);
		gtk_widget_show (contents);
		gtk_widget_unref (contents);
	}

	app->contents = contents;
}

/* The global menus for setting the toolbar and menu bar position */

static GtkWidget *toolbar_menu = NULL;
static GtkWidget *menu_menu = NULL;
static GtkWidget *menuitems [2];
static GtkWidget *toolitems [4];

/* Creates the menu used to set the menu bar's position */
static void
make_button_menubar (GnomeApp *app)
{
	int i;

	menu_menu = gtk_menu_new ();
	menuitems [0] = gtk_menu_item_new_with_label (_("Top"));
	menuitems [1] = gtk_menu_item_new_with_label (_("Bottom"));

	for (i = 0; i < 2; i++) {
		gtk_widget_show (menuitems[i]);
		gtk_menu_append (GTK_MENU (menu_menu), menuitems [i]);
		gtk_signal_connect (GTK_OBJECT (menuitems[i]), "activate",
				    GTK_SIGNAL_FUNC (gnome_app_setpos_activate_menubar),
				    app);
	}
}

static void
configure_toolbar (GtkMenuItem *menu_item, GnomeApp *app)
{
	/* FIXME: add toolbar configuration dialog box here */
}
			       
/* Creates the menu used to set the toolbar's position */
static void
make_button_toolbar (GnomeApp *app)
{
	int i;

	toolbar_menu = gtk_menu_new ();

	toolitems [0] = gtk_menu_item_new_with_label (_("Top"));
	toolitems [1] = gtk_menu_item_new_with_label (_("Bottom"));
	toolitems [2] = gtk_menu_item_new_with_label (_("Left"));
	toolitems [3] = gtk_menu_item_new_with_label (_("Right"));
	toolitems [4] = gtk_menu_item_new_with_label (_("Configure toolbar"));

	/* Change 4 to 5 to do the toolbar configuration code */
	for (i = 0; i < 4; i++) {
		GtkSignalFunc function;

		gtk_widget_show (toolitems [i]);
		gtk_menu_append (GTK_MENU (toolbar_menu), toolitems [i]);

		if (i == 4)
			function = GTK_SIGNAL_FUNC (configure_toolbar);
		else
			function = GTK_SIGNAL_FUNC (gnome_app_setpos_activate_toolbar);

		gtk_signal_connect (GTK_OBJECT (toolitems[i]), "activate", function, app);
	}
}

/* Handles button clicks on the menu bar's handle box */
static void
gnome_app_rightclick_menubar (GtkWidget *widget, GdkEventButton *event, GnomeApp *app)
{
	if (menu_menu == NULL)
		make_button_menubar (app);

	if (GTK_WIDGET_VISIBLE (menu_menu))
		gtk_menu_popdown (GTK_MENU (menu_menu));

	/* Double click on handlebox, reparents */
	if ((event->button == 1) && (event->type == GDK_2BUTTON_PRESS)) {
		GtkHandleBox *handle_box;

		handle_box = GTK_HANDLE_BOX (app->menubar->parent);
		gnome_app_reparent_handle_box (handle_box);
		return;
	}

	if (event->button != 3)
		return;

	gtk_menu_popup (GTK_MENU (menu_menu),
			NULL, NULL, NULL, NULL,
			event->button,
			event->time);
}

/* Reparents the handle box back to its docked position -- FIXME: this should go into Gtk. */
static void
gnome_app_reparent_handle_box (GtkHandleBox *hb)
{
	/* This code taken from gtkhandlebox.c until a function gets properly exported */

	gdk_window_hide (hb->float_window);
	gdk_window_reparent (hb->bin_window, GTK_WIDGET (hb)->window, 0, 0);
	hb->float_window_mapped = FALSE;
	hb->child_detached = FALSE;

	if (hb->in_drag) {
		gdk_pointer_ungrab (GDK_CURRENT_TIME);
		gtk_grab_remove (GTK_WIDGET (hb));
		hb->in_drag = FALSE;
	}

	gtk_widget_queue_resize (GTK_WIDGET (hb));
}

/* Handles button presses on the toolbar's handle box */
static void
gnome_app_rightclick_toolbar (GtkWidget *widget, GdkEventButton *event, GnomeApp *app)
{
	/* Double click on handlebox, reparents */

	if ((event->button == 1) && (event->type == GDK_2BUTTON_PRESS)) {
		GtkHandleBox *handle_box;

		handle_box = GTK_HANDLE_BOX (app->toolbar->parent);
		gnome_app_reparent_handle_box (handle_box);
		return;
	}

	/* Right click on handlebox, brings up menu */

	if (toolbar_menu == NULL)
		make_button_toolbar (app);

	if (GTK_WIDGET_VISIBLE (toolbar_menu))
		gtk_menu_popdown (GTK_MENU (toolbar_menu));

	if (event->button != 3)
		return;

	gtk_menu_popup (GTK_MENU (toolbar_menu),
			NULL, NULL, NULL, NULL,
			event->button,
			event->time);
}

/* Callback used when the menu position is changed by the user */
static void
gnome_app_setpos_activate_menubar (GtkWidget *menu_item, GnomeApp *app)
{
	int i;

	gtk_menu_popdown (GTK_MENU (menu_menu));

	/* We only go through the 1st two since a menubar can only go top or bottom */

	for (i = 0; i < 2; i++)
		if (menu_item == menuitems[i]) {
			gnome_app_menu_set_position (app, i);
			break;
		}
}

/* Callback used when the toolbar position is changed by the user */
static void
gnome_app_setpos_activate_toolbar (GtkWidget *menu_item, GnomeApp *app)
{
	int i;

	gtk_menu_popdown (GTK_MENU (toolbar_menu));

	for (i = 0; i < 4; i++)
		if (menu_item == toolitems[i]) {
			gnome_app_toolbar_set_position (app, i);
			break;
		}
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
	GnomeAppWidgetPositionType pos = GNOME_APP_POS_TOP;
	char *location = NULL;
	GtkWidget *hb;
	GtkAccelGroup *ag;

	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(app->menubar == NULL);
	g_return_if_fail(menubar != NULL);
	g_return_if_fail(GTK_IS_MENU_BAR(menubar));

	if (gnome_preferences_get_menubar_handlebox()){
		hb = gtk_handle_box_new();
		gtk_widget_show(hb);
	} else {
		hb = gtk_event_box_new();
		gtk_widget_set_events(hb, GDK_BUTTON_PRESS_MASK);
	}

	app->menubar = GTK_WIDGET(menubar);

	if (!gnome_preferences_get_menubar_relief ())
		gtk_menu_bar_set_shadow_type (GTK_MENU_BAR (app->menubar), GTK_SHADOW_NONE);
	
	gtk_signal_connect(GTK_OBJECT(hb), "button_press_event",
			   GTK_SIGNAL_FUNC(gnome_app_rightclick_menubar), app);
	gtk_widget_show(app->menubar);
	gtk_container_add(GTK_CONTAINER(hb), app->menubar);

	/* Load the position from the configuration file */
	if (app->prefix){
		gnome_config_push_prefix (app->prefix);
		location = gnome_config_get_string ("Placement/Menu=top");
		pos = get_orientation (location);
	}

	/* Menus can not go on left or right */
	if (pos != GNOME_APP_POS_TOP && pos != GNOME_APP_POS_BOTTOM)
		pos = GNOME_APP_POS_TOP;
	gnome_app_menu_set_position (app, pos);
	if (app->prefix){
		g_free (location);
		gnome_config_pop_prefix ();
	}

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
	GnomeAppWidgetPositionType pos = GNOME_APP_POS_TOP;
	GtkWidget *hb;
	char *location;
	GtkAccelGroup *ag;

	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(toolbar != NULL);
	g_return_if_fail(app->toolbar == NULL);

	if ( gnome_preferences_get_toolbar_handlebox() ) {
	  hb = gtk_handle_box_new ();
	  gtk_widget_show (hb);
	  if ( gnome_preferences_get_toolbar_flat() )
		  gtk_handle_box_set_shadow_type (GTK_HANDLE_BOX (hb), GTK_SHADOW_NONE);
	  else if ( ! gnome_preferences_get_toolbar_relief() ) {
		  /* Avoid relief overlap with flat buttons + handlebox relief */
		  gtk_container_set_border_width (GTK_CONTAINER (hb), 2);
	  }
	}
	else {
   	  /* Non-detachable toolbars are always flat */
	  hb = gtk_event_box_new();
	  gtk_widget_set_events(hb, GDK_BUTTON_PRESS_MASK);
	}

	if ( gnome_preferences_get_toolbar_relief() ||
	     !gnome_preferences_get_toolbar_lines() ) {
	  gtk_toolbar_set_space_size (GTK_TOOLBAR (toolbar), GNOME_PAD);
	} else {
	  gtk_toolbar_set_space_size (GTK_TOOLBAR (toolbar), GNOME_PAD_SMALL);
	}

	if ( !gnome_preferences_get_toolbar_relief() )
	  gtk_toolbar_set_button_relief(toolbar, GTK_RELIEF_NONE);
	
	if (!gnome_preferences_get_toolbar_labels ())
		gtk_toolbar_set_style (toolbar, GTK_TOOLBAR_ICONS);
	
	app->toolbar = GTK_WIDGET (toolbar);
	gtk_signal_connect (GTK_OBJECT(hb), "button_press_event",
			    GTK_SIGNAL_FUNC (gnome_app_rightclick_toolbar), app);
	gtk_widget_show(app->toolbar);
	gtk_container_add(GTK_CONTAINER(hb), app->toolbar);

	/* Load the position from the configuration file */
	if (app->prefix)
	{
		gnome_config_push_prefix (app->prefix);
		location = gnome_config_get_string ("Placement/Toolbar=top");
		pos = get_orientation (location);
		gnome_app_toolbar_set_position (app, pos);
		g_free (location);
		gnome_config_pop_prefix ();
	}
	else
		gnome_app_toolbar_set_position (app, pos);

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
	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(statusbar != NULL);
	g_return_if_fail(app->statusbar == NULL);

	app->statusbar = GTK_WIDGET(statusbar);
	gtk_widget_show(app->statusbar);

	gtk_table_attach (GTK_TABLE (app->table),
			  app->statusbar,
			  0, 3,
			  3, 4,
			  GTK_EXPAND | GTK_FILL | GTK_SHRINK,
			  0,
			  0, 0);
}
