/*
 * GnomeApp widget (C) 1998 Red Hat Software, The Free Software Foundation,
 * Miguel de Icaza, Federico Menu, Chris Toshok.
 *
 * Originally by Elliot Lee,
 *
 * improvements and rearrangement by Miguel,
 * and I don't know what you other people did :)
 */
#include "config.h"
#include <glib.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-i18nP.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-config.h"
#include "gnome-app.h"
#include <string.h>
#include <gtk/gtk.h>

static void gnome_app_class_init     	      (GnomeAppClass *appclass);
static void gnome_app_destroy        	      (GtkObject     *object);
static void gnome_app_init           	      (GnomeApp      *app);
static void gnome_app_setpos_activate_menubar (GtkMenuItem *menu_item,
					       GnomeApp *app);
static void gnome_app_setpos_activate_toolbar (GtkMenuItem *menu_item,
					       GnomeApp *app);

static GtkWindowClass *parent_class = NULL;

/* keep in sync with GnomeAppWidgetPositionType */
static char *locations [] = {
	"top", "bottom", "left", "right", "float,", NULL
};

static GnomeAppWidgetPositionType
get_orientation (char *str)
{
	int i;
	
	for (i = 0; locations [i]; i++)
		if (strcasecmp (str, locations [i]) == 0)
			return i;

	/* If we dont recognize it => top */
	return GNOME_APP_POS_TOP;
}

guint
gnome_app_get_type(void)
{
	static guint gnomeapp_type = 0;
	if(!gnomeapp_type) {
		GtkTypeInfo gnomeapp_info = {
			"GnomeApp",
			sizeof(GnomeApp),
			sizeof(GnomeAppClass),
			(GtkClassInitFunc) gnome_app_class_init,
			(GtkObjectInitFunc) gnome_app_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
		};
		gnomeapp_type = gtk_type_unique(gtk_window_get_type(), &gnomeapp_info);
		parent_class = gtk_type_class(gtk_window_get_type());
	}
	return gnomeapp_type;
}

#if 0
/* Commented out until the code in gnome_app_class_init is removed or
   deleted.  */
static void
gnome_app_add (GtkContainer *container,
	       GtkWidget    *widget)
{
	g_return_if_fail (container != NULL);
	g_return_if_fail (GNOME_IS_APP (container));
	g_return_if_fail (widget != NULL);

	/* Buggy sigh */
	gnome_app_set_contents (GNOME_APP (container), widget);
}
#endif

static void
gnome_app_class_init(GnomeAppClass *appclass)
{
	GtkObjectClass *object_class = GTK_OBJECT_CLASS          (appclass);
/*	GtkContainerClass *container_class = GTK_CONTAINER_CLASS (appclass);*/
		
	object_class->destroy = gnome_app_destroy;
/*	container_class->add = gnome_app_add; */
}

static void
gnome_app_init(GnomeApp *app)
{
	app->menubar = app->toolbar = app->contents = NULL;
	app->statusbar = NULL;
	app->table = gtk_table_new(4, 3, FALSE);
	gtk_widget_show(app->table);
	gtk_container_add(GTK_CONTAINER(app), app->table);
	
	app->pos_menubar = app->pos_toolbar = GNOME_APP_POS_TOP;
}

GtkWidget *
gnome_app_new(gchar *appname, char *title)
{
	GtkWidget *retval;
	GnomeApp  *app;
		
	retval = gtk_type_new(gnome_app_get_type());
	app = GNOME_APP (retval);

	app->name = g_strdup (appname);
	app->prefix = g_copy_strings ("/", appname, "/", NULL);
	
	if (title)
		gtk_window_set_title(GTK_WINDOW(retval), title);

	return retval;
}

static void
gnome_app_destroy(GtkObject *object)
{
	GnomeApp *app;
	
	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_APP (object));

	app = GNOME_APP (object);

	g_free (app->name);
	g_free (app->prefix);

	if(GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(GTK_OBJECT(app));
}

static void
gnome_app_configure_positions (GnomeApp *app)
{
	/* 1.  The menubar: can go on top or bottom */
	if (app->menubar)
	{
		GtkWidget *handlebox = app->menubar->parent;

		gtk_widget_ref (handlebox);
		gtk_widget_hide (handlebox);

		if (app->menubar->parent->parent)
			gtk_container_remove (GTK_CONTAINER(app->table), handlebox);

		gtk_table_attach(GTK_TABLE(app->table),
				 handlebox,
				 0, 3,
				 (app->pos_menubar == GNOME_APP_POS_TOP)?0:2,
				 (app->pos_menubar == GNOME_APP_POS_TOP)?1:3,
				 GTK_EXPAND | GTK_FILL,
				 GTK_SHRINK,
				 0, 0);
		gtk_widget_show (handlebox);
		gtk_widget_unref (handlebox);
	}
	
	/* 2. the toolbar */
	if (app->toolbar)
	{
		GtkWidget *handlebox = app->toolbar->parent;
		int offset = 0;

		gtk_widget_ref (handlebox);
		gtk_widget_hide (handlebox);

		if (app->toolbar->parent->parent)
			gtk_container_remove (GTK_CONTAINER(app->table), handlebox);

		if(app->pos_menubar == GNOME_APP_POS_TOP)
			offset = 1;
		
		if(app->pos_toolbar == GNOME_APP_POS_LEFT || app->pos_toolbar == GNOME_APP_POS_RIGHT)
		{
			gtk_table_attach(GTK_TABLE(app->table),
					 handlebox,
					 (app->pos_toolbar==GNOME_APP_POS_LEFT)?0:2,
					 (app->pos_toolbar==GNOME_APP_POS_LEFT)?1:3,
					 offset, 3,
					 GTK_SHRINK,
					 GTK_EXPAND | GTK_FILL,
					 0, 0);
		}
		else
		{
			gint moffset;
			
			if(app->pos_menubar == app->pos_toolbar)
				moffset = (app->pos_toolbar == GNOME_APP_POS_TOP)?1:-1;
			else
				moffset = 0;
			gtk_table_attach(GTK_TABLE(app->table),
					 handlebox,
					 0, 3,
					 ((app->pos_toolbar==GNOME_APP_POS_TOP)?0:2) + moffset,
					 ((app->pos_toolbar==GNOME_APP_POS_TOP)?1:3) + moffset,
					 GTK_EXPAND | GTK_FILL,
					 GTK_SHRINK,
					 0, 0);
		}
		gtk_widget_show (handlebox);
		gtk_widget_unref (handlebox);
	}
	/* Repack any contents of ours */
	if(app->contents)
		gnome_app_set_contents(app, app->contents);
}	

void
gnome_app_menu_set_position(GnomeApp *app, GnomeAppWidgetPositionType pos_menubar)
{
	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));

	g_return_if_fail(app->menubar != NULL);
	g_return_if_fail(pos_menubar == GNOME_APP_POS_TOP || pos_menubar == GNOME_APP_POS_BOTTOM);
	
	app->pos_menubar = pos_menubar;
	
	gnome_app_configure_positions (app);
	
	/* Save the new setting */
	if(app->prefix)
	{
		gnome_config_push_prefix (app->prefix);
		gnome_config_set_string ("Placement/Menu",
					 locations [pos_menubar]);
		gnome_config_pop_prefix ();
		gnome_config_sync ();
	}
}

void
gnome_app_toolbar_set_position(GnomeApp *app, GnomeAppWidgetPositionType pos_toolbar)
{
	g_return_if_fail(app->toolbar != NULL);
	
	app->pos_toolbar = pos_toolbar;
	
	if(pos_toolbar == GNOME_APP_POS_LEFT || pos_toolbar == GNOME_APP_POS_RIGHT)
	{
		if (GTK_IS_TOOLBAR (app->toolbar))
			gtk_toolbar_set_orientation(GTK_TOOLBAR(app->toolbar),
						    GTK_ORIENTATION_VERTICAL);
		gnome_app_configure_positions (app);
	}
	else 
	{
		/* assume GNOME_APP_POS_TOP || GNOME_APP_POS_BOTTOM */
		if (GTK_IS_TOOLBAR (app->toolbar))
			gtk_toolbar_set_orientation(GTK_TOOLBAR(app->toolbar),
						    GTK_ORIENTATION_HORIZONTAL);
		gnome_app_configure_positions (app);
	}
	
	/* Save the new setting */
	if(app->prefix)
	{
		gnome_config_push_prefix (app->prefix);
		gnome_config_set_string ("Placement/Toolbar",
					 locations [pos_toolbar]);
		gnome_config_pop_prefix ();
		gnome_config_sync ();
	}
}

/* These are used for knowing where to pack the contents into the
   table, so we don't have to recompute every time we set_contents.
   The first dimension is the current position of the menubar, and the
   second dimension is the current position of the toolbar. Put it all
   together and you find out the top-left and bottom-right coordinates
   of the contents inside the placement table.
*/
static gint startxs[2][4] = {
	{0, 0, 1, 0},
	{0, 0, 1, 0}
};

static gint endxs[2][4] = {
	{3, 3, 3, 2},
	{3, 3, 3, 2}
};

static gint startys[2][4] = {
	{2, 1, 1, 1},
	{1, 0, 0, 0}
};

static gint endys[2][4] = {
	{3, 2, 3, 3},
	{2, 1, 2, 2}
};

void
gnome_app_set_contents(GnomeApp *app, GtkWidget *contents)
{
	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP(app));

	/* We ref the contents in case app->contents == contents, so
	 * that the container_remove will not destroy the widget.
	 */

	gtk_widget_ref (contents);
  
	if (app->contents != NULL)
		gtk_container_remove (GTK_CONTAINER (app->table), app->contents);

	gtk_table_attach (GTK_TABLE (app->table),
			  contents,
			  startxs[app->pos_menubar][app->pos_toolbar],
			  endxs[app->pos_menubar][app->pos_toolbar],
			  startys[app->pos_menubar][app->pos_toolbar],
			  endys[app->pos_menubar][app->pos_toolbar],
			  GTK_EXPAND | GTK_FILL,
			  GTK_EXPAND | GTK_FILL,
			  0, 0);
	app->contents = contents;
	gtk_widget_show (contents);

	gtk_widget_unref (contents);
}

static GtkWidget *toolbar_menu = NULL;
static GtkWidget *menu_menu = NULL;
static GtkWidget *menuitems[2];
static GtkWidget *toolitems[4];

static void
make_button_menubar(GnomeApp *app)
{
	int i;
	
	menu_menu = gtk_menu_new();
	menuitems [0] = gtk_menu_item_new_with_label(_("Top"));
	menuitems [1] = gtk_menu_item_new_with_label(_("Bottom"));

	for(i = 0; i < 2; i++)
	{
		gtk_widget_show(menuitems[i]);
		gtk_menu_append(GTK_MENU(menu_menu), menuitems[i]);
		gtk_signal_connect(GTK_OBJECT(menuitems[i]), "activate",
				   GTK_SIGNAL_FUNC(gnome_app_setpos_activate_menubar), app);
	}
}

static void
configure_toolbar (GtkMenuItem *menu_item, GnomeApp *app)
{
	/* FIXME: add toolbar configuration dialog box here */
}
			       
static void
make_button_toolbar (GnomeApp *app)
{
	int i;

	toolbar_menu = gtk_menu_new ();

	toolitems [0] = gtk_menu_item_new_with_label(_("Top"));
	toolitems [1] = gtk_menu_item_new_with_label(_("Bottom"));
	toolitems [2] = gtk_menu_item_new_with_label(_("Left"));
	toolitems [3] = gtk_menu_item_new_with_label(_("Right"));
	toolitems [4] = gtk_menu_item_new_with_label(_("Configure toolbar"));

	/* Change 4 to 5 to do the toolbar configuration code */
	for (i = 0; i < 4; i++)
	{
		GtkSignalFunc function;
		
		gtk_widget_show (toolitems [i]);
		gtk_menu_append (GTK_MENU (toolbar_menu), toolitems [i]);
		
		if (i == 4)
			function = GTK_SIGNAL_FUNC (configure_toolbar);
		else
			function = GTK_SIGNAL_FUNC (gnome_app_setpos_activate_toolbar);
		
		gtk_signal_connect (GTK_OBJECT(toolitems[i]), "activate", function, app);
	}
}

static void
gnome_app_rightclick_menubar(GtkWidget *widget, GdkEventButton *event, GnomeApp *app)
{
	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));

	if (menu_menu == NULL)
		make_button_menubar (app);

	if (GTK_WIDGET_VISIBLE (menu_menu))
		gtk_menu_popdown (GTK_MENU (menu_menu));

	if(event->button != 3)
		return;

	gtk_menu_popup (GTK_MENU (menu_menu),
		       NULL, NULL, NULL, NULL,
		       event->button,
		       event->time);
}

static void
gnome_app_rightclick_toolbar(GtkWidget *widget, GdkEventButton *event, GnomeApp *app)
{
	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));

	/* Double click on handlebox, reparents */
	if (event->button == 1 && event->type == GDK_2BUTTON_PRESS){
		/* FIXME: Mhm, there is no easy way to tell the handlebox to reparent itself */
		return;
	}
		
	/* Right click on handlebox, brings up menu */
	if (toolbar_menu == NULL)
		make_button_toolbar (app);

	if (GTK_WIDGET_VISIBLE(toolbar_menu))
		gtk_menu_popdown(GTK_MENU(toolbar_menu));

	if (event->button != 3)
		return;

	gtk_menu_popup(GTK_MENU(toolbar_menu),
		       NULL, NULL, NULL, NULL,
		       event->button,
		       event->time);
}

static void
gnome_app_setpos_activate_menubar (GtkMenuItem *menu_item, GnomeApp *app)
{
	int i;
	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));

	gtk_menu_popdown(GTK_MENU(menu_menu));
	
	/* We only go through the 1st two
	 * since a menubar can only go top
	 * or bottom
	 */
	for(i = 0; i < 2; i++)
		if((gpointer)menu_item == (gpointer)menuitems[i])
		{
			gnome_app_menu_set_position (app, i);
			break;
		}
}

static void
gnome_app_setpos_activate_toolbar (GtkMenuItem *menu_item, GnomeApp *app)
{
	int i;
	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));

	gtk_menu_popdown(GTK_MENU(toolbar_menu));
	
	for (i = 0; i < 4; i++)
		if ((gpointer)menu_item == (gpointer)toolitems[i])
		{
			gnome_app_toolbar_set_position (app, i);
			break;
		}
}

void
gnome_app_set_menus (GnomeApp *app,
		     GtkMenuBar *menubar)
{
	GnomeAppWidgetPositionType pos = GNOME_APP_POS_TOP;
	char *location = NULL;
	GtkWidget *hb;
#ifdef GTK_HAVE_ACCEL_GROUP
	GtkAccelGroup *ag;
#else
	GtkAcceleratorTable *at;
#endif
	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(app->menubar == NULL);
	g_return_if_fail(menubar != NULL);
	g_return_if_fail(GTK_IS_MENU_BAR(menubar));

	if ( gnome_preferences_get_menubar_handlebox() ) {
	  hb = gtk_handle_box_new();
	  gtk_widget_show(hb);
	}
	else {
	  hb = gtk_event_box_new();
	  gtk_widget_set_events(hb, GDK_BUTTON_PRESS_MASK);
	}

	app->menubar = GTK_WIDGET(menubar);

	gtk_signal_connect(GTK_OBJECT(hb), "button_press_event",
			   GTK_SIGNAL_FUNC(gnome_app_rightclick_menubar), app);
	gtk_widget_show(app->menubar);
	gtk_container_add(GTK_CONTAINER(hb), app->menubar);

	/* Load the position from the configuration file */
	if (app->prefix)
	{
		gnome_config_push_prefix (app->prefix);
		location = gnome_config_get_string ("Placement/Menu=top");
		pos = get_orientation (location);
	}

	/* Menus can not go on left or right */
	if (pos != GNOME_APP_POS_TOP && pos != GNOME_APP_POS_BOTTOM)
		pos = GNOME_APP_POS_TOP;
	gnome_app_menu_set_position (app, pos);
	if (app->prefix)
	{
		g_free (location);
		gnome_config_pop_prefix ();
	}
#ifdef GTK_HAVE_ACCEL_GROUP
	ag = gtk_object_get_data(GTK_OBJECT(app), "GtkAccelGroup");
/* FIXME */
/*	if(ag && !g_list_find(GTK_WINDOW(app)->accelerator_groups, ag)) */
	if (ag)
	gtk_window_add_accel_group(GTK_WINDOW(app), ag);
#else
        at = gtk_object_get_data(GTK_OBJECT(app), "GtkAcceleratorTable");
        if(at && !g_list_find(GTK_WINDOW(app)->accelerator_tables, at))
                gtk_window_add_accelerator_table(GTK_WINDOW(app), at);
#endif
}

void
gnome_app_set_toolbar (GnomeApp *app,
		       GtkToolbar *toolbar)
{
	GnomeAppWidgetPositionType pos = GNOME_APP_POS_TOP;
	GtkWidget *hb;
	char *location;
#ifdef GTK_HAVE_ACCEL_GROUP
	GtkAccelGroup *ag;
#else
	GtkAcceleratorTable *at;
#endif
	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(toolbar != NULL);
	g_return_if_fail(app->toolbar == NULL);

	if ( gnome_preferences_get_toolbar_handlebox() ) {
	  hb = gtk_handle_box_new ();
	  gtk_widget_show (hb);
	}
	else {
	  hb = gtk_event_box_new();
	  gtk_widget_set_events(hb, GDK_BUTTON_PRESS_MASK);
	}
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
#ifdef GTK_HAVE_ACCEL_GROUP
	ag = gtk_object_get_data(GTK_OBJECT(app), "GtkAccelGroup");
	/* FIXME
	if(ag && !g_list_find(GTK_WINDOW(app)->accelerator_groups, ag))
	*/
	if (ag)
		gtk_window_add_accel_group(GTK_WINDOW(app), ag);
#else
        at = gtk_object_get_data(GTK_OBJECT(app), "GtkAcceleratorTable");
        if(at && !g_list_find(GTK_WINDOW(app)->accelerator_tables, at))
                gtk_window_add_accelerator_table(GTK_WINDOW(app), at);
#endif
}

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
			  GTK_EXPAND | GTK_FILL,
			  GTK_SHRINK,
			  0, 0);
}






