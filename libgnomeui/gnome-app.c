/*
 * GnomeApp widget by Elliot Lee
 */
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-config.h"
#include "gnome-app.h"
#include "gnome-pixmap.h"
#include <string.h>
#include <gtk/gtk.h>

static void gnome_app_class_init     	  (GnomeAppClass *appclass);
static void gnome_app_destroy        	  (GnomeApp      *app);
static void gnome_app_init           	  (GnomeApp      *app);
static void gnome_app_do_menu_creation    (GtkWidget *parent_widget,
					   GnomeMenuInfo *menuinfo);
static void gnome_app_do_toolbar_creation (GnomeApp *app,
					   GtkWidget *parent_widget,
					   GnomeToolbarInfo *tbinfo);
static void gnome_app_rightclick_event    (GtkWidget *widget,
					   GdkEventButton *event,
					   GnomeApp *app);
static void gnome_app_setpos_activate     (GtkMenuItem *menu_item,
					   GnomeApp *app);

static GtkWindowClass *parent_class = NULL;

/* keep in sync with GnomeAppWidgetPositionType */
static char *locations [] = {
	"top", "botton", "left", "right", "float,", NULL
};

static GnomeAppWidgetPositionType
get_orientation (char *str)
{
	int i;
	
	for (i = 0; locations [i]; i++){
		if (strcasecmp (str, locations [i]) == 0)
			return i;
	}
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
			(GtkArgFunc) NULL,
		};
		gnomeapp_type = gtk_type_unique(gtk_window_get_type(), &gnomeapp_info);
		parent_class = gtk_type_class(gtk_window_get_type());
	}
	return gnomeapp_type;
}

static void
gnome_app_class_init(GnomeAppClass *appclass)
{
	GtkObjectClass *object_class = GTK_OBJECT_CLASS(appclass);
	object_class->destroy = gnome_app_destroy;
}

static void
gnome_app_init(GnomeApp *app)
{
	app->menubar = app->toolbar = app->contents = NULL;
	app->table = gtk_table_new(3, 3, FALSE);
	gtk_widget_show(app->table);
	gtk_container_add(GTK_CONTAINER(app), app->table);
	
	app->pos_menubar = app->pos_toolbar = GNOME_APP_POS_TOP;
}

GtkWidget *
gnome_app_new(gchar *appname, char *title)
{
	GtkWidget *retval;
	GnomeApp  *app;
	char      *prefix;
		
	retval = gtk_type_new(gnome_app_get_type());
	app = GNOME_APP (retval);
	app->name = g_strdup (appname);
	app->prefix = g_copy_strings ("/", appname, "/", NULL);
	if (title)
		gtk_window_set_title(GTK_WINDOW(retval), title);
	
	return retval;
}

static void
gnome_app_destroy(GnomeApp *app)
{
	g_free (app->name);
	g_free (app->prefix);
	if(GTK_OBJECT_CLASS(parent_class)->destroy)
		(* GTK_OBJECT_CLASS(parent_class)->destroy)(GTK_OBJECT(app));
}

static void
gnome_app_do_menu_creation(GtkWidget *parent_widget,
			   GnomeMenuInfo *menuinfo)
{
	int i;
	for(i = 0; menuinfo[i].type != GNOME_APP_MENU_ENDOFINFO; i++)
	{
		menuinfo[i].widget = gtk_menu_item_new_with_label(menuinfo[i].label);
		gtk_widget_show(menuinfo[i].widget);
		gtk_menu_shell_append(GTK_MENU_SHELL(parent_widget),
				      menuinfo[i].widget);
		
		if(menuinfo[i].type == GNOME_APP_MENU_ITEM)
		{
			gtk_signal_connect(GTK_OBJECT(menuinfo[i].widget), "activate",
					   menuinfo[i].moreinfo, NULL);
		}
		else if(menuinfo[i].type == GNOME_APP_MENU_SUBMENU)
		{
			GtkWidget *submenu;
			submenu = gtk_menu_new();
			gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuinfo[i].widget),
						  submenu);
			gnome_app_do_menu_creation(submenu, menuinfo[i].moreinfo);
		}
	}
}

void
gnome_app_create_menus(GnomeApp *app,
		       GnomeMenuInfo *menuinfo)
{
	GtkWidget *hb, *menubar;
	
	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(app->menubar == NULL);

	menubar = gtk_menu_bar_new ();
	gnome_app_set_menus (app, GTK_MENU_BAR (menubar));
	
	if(menuinfo)
		gnome_app_do_menu_creation(app->menubar, menuinfo);
}

static void
gnome_app_do_toolbar_creation(GnomeApp *app,
			      GtkWidget *parent_widget,
			      GnomeToolbarInfo *tbinfo)
{
	int i;
	GtkWidget *pmap;
	
	if(!GTK_WIDGET(app)->window)
		gtk_widget_realize(GTK_WIDGET(app));
	
	for(i = 0; tbinfo[i].type != GNOME_APP_TOOLBAR_ENDOFINFO; i++)
	{
		if(tbinfo[i].type == GNOME_APP_TOOLBAR_ITEM)
		{
			if(tbinfo[i].pixmap_type == GNOME_APP_PIXMAP_DATA)
				pmap = gnome_create_pixmap_widget_d(GTK_WIDGET(app),
								    parent_widget,
								    (char **)tbinfo[i].pixmap_info);
			else if(tbinfo[i].pixmap_type == GNOME_APP_PIXMAP_FILENAME)
				pmap = gnome_create_pixmap_widget(GTK_WIDGET(app),
								  parent_widget,
								  (char *)tbinfo[i].pixmap_info);
			else
				pmap = NULL;
			gtk_toolbar_append_item(GTK_TOOLBAR(parent_widget),
						tbinfo[i].text,
						tbinfo[i].tooltip_text,
						GTK_PIXMAP(pmap),
						tbinfo[i].clicked_callback,
						NULL);
		}
		else if(tbinfo[i].type == GNOME_APP_TOOLBAR_SPACE)
		{
			gtk_toolbar_append_space(GTK_TOOLBAR(parent_widget));
		}
	}
}

void gnome_app_create_toolbar(GnomeApp *app,
			      GnomeToolbarInfo *toolbarinfo)
{
	GtkWidget *hb;
	
	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(app->toolbar == NULL);
	
	gnome_app_set_toolbar (app, GTK_TOOLBAR (
		gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH)));
	
	if(toolbarinfo)
		gnome_app_do_toolbar_creation(app, app->toolbar, toolbarinfo);
}

void
gnome_app_menu_set_position(GnomeApp *app, GnomeAppWidgetPositionType pos_menubar)
{
	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));

	g_return_if_fail(app->menubar != NULL);
	g_return_if_fail(pos_menubar == GNOME_APP_POS_TOP || pos_menubar == GNOME_APP_POS_BOTTOM);
	
	app->pos_menubar = pos_menubar;
	
	/* It's ->parent->parent because this is inside a GtkHandleBox */
	if(app->menubar->parent->parent)
		gtk_container_remove(GTK_CONTAINER(app->menubar->parent->parent),
				     app->menubar->parent);
	/* Obviously the menu bar can't have vertical orientation,
	   so we don't support POS_LEFT or POS_RIGHT. */
	gtk_table_attach_defaults(GTK_TABLE(app->table),
				  app->menubar->parent,
				  0, 3,
				  (pos_menubar == GNOME_APP_POS_TOP)?0:2,
				  (pos_menubar == GNOME_APP_POS_TOP)?1:3);
	
	/* Repack any contents of ours */
	if(app->contents) 
		gnome_app_set_contents(app, app->contents);

	/* Save the new setting */
	gnome_config_push_prefix (app->prefix);
	gnome_config_set_string ("Placement/Menu", locations [pos_menubar]);
	gnome_config_pop_prefix ();
	gnome_config_sync ();
}

void
gnome_app_toolbar_set_position(GnomeApp *app, GnomeAppWidgetPositionType pos_toolbar)
{
	g_return_if_fail(app->toolbar != NULL);
	
	app->pos_toolbar = pos_toolbar;
	
	/* It's ->parent->parent because this is inside a GtkHandleBox */
	if(app->toolbar->parent->parent)
		gtk_container_remove(GTK_CONTAINER(app->toolbar->parent->parent),
				     app->toolbar->parent);
	if(pos_toolbar == GNOME_APP_POS_LEFT || pos_toolbar == GNOME_APP_POS_RIGHT)
	{
		gtk_toolbar_set_orientation(GTK_TOOLBAR(app->toolbar),
					    GTK_ORIENTATION_VERTICAL);
		gtk_table_attach_defaults(GTK_TABLE(app->table),
					  app->toolbar->parent,
					  (pos_toolbar==GNOME_APP_POS_LEFT)?0:2,
					  (pos_toolbar==GNOME_APP_POS_LEFT)?1:3,
					  0, 3);
		}
	else
	{
		gint moffset;
		
		if(app->pos_menubar == pos_toolbar)
			moffset = (pos_toolbar == GNOME_APP_POS_TOP)?1:-1;
		else
			moffset = 0;
		/* assume GNOME_APP_POS_TOP || GNOME_APP_POS_BOTTOM */
		gtk_toolbar_set_orientation(GTK_TOOLBAR(app->toolbar),
					    GTK_ORIENTATION_HORIZONTAL);
		gtk_table_attach_defaults(GTK_TABLE(app->table),
					  app->toolbar->parent,
					  0, 3,
					  (pos_toolbar==GNOME_APP_POS_TOP)?0:2 + moffset,
					  (pos_toolbar==GNOME_APP_POS_TOP)?1:3 + moffset);
	}
	
	/* Repack any contents of ours */	
	if(app->contents)
		gnome_app_set_contents(app, app->contents);
	
	/* Save the new setting */
	gnome_config_push_prefix (app->prefix);
	gnome_config_set_string ("Placement/Toolbar", locations [pos_toolbar]);
	gnome_config_pop_prefix ();
	gnome_config_sync ();
}

/* These are used for knowing where to pack the contents into the
   table, so we don't have to recompute every time we set_contents */
gint startys[2][4] = {
	{2, 1, 1, 1},
	{1, 0, 0, 0}
};
gint startxs[2][4] = {
	{0, 0, 1, 0},
	{0, 0, 1, 0}
};
gint endys[2][4] = {
	{3, 2, 3, 3},
	{2, 1, 2, 2}
};
gint endxs[2][4] = {
	{3, 3, 3, 2},
	{3, 3, 3, 2}
};

void
gnome_app_set_contents(GnomeApp *app, GtkWidget *contents)
{
	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));

	if(app->contents != NULL)
		gtk_container_remove(GTK_CONTAINER(app->table), app->contents);
  
	/* Is this going to work at all? I'll wager not
	   XXX oops it worked, my mistake :) */
	gtk_table_attach_defaults(GTK_TABLE(app->table),
				  contents,
				  startxs[app->pos_menubar][app->pos_toolbar],
				  endxs[app->pos_menubar][app->pos_toolbar],
				  startys[app->pos_menubar][app->pos_toolbar],
				  endys[app->pos_menubar][app->pos_toolbar]);
	app->contents = contents;
	gtk_widget_show(contents);
}

static GtkWidget *rmb_menu = NULL, *clicked_widget = NULL;
static GtkWidget *menuitems[4];

static void make_rmb_menu(GnomeApp *app)
{
	int i;

	rmb_menu = gtk_menu_new();

	menuitems[0] = gtk_menu_item_new_with_label("Top");
	menuitems[1] = gtk_menu_item_new_with_label("Bottom");
	menuitems[2] = gtk_menu_item_new_with_label("Left");
	menuitems[3] = gtk_menu_item_new_with_label("Right");
	
	for(i = 0; i < 4; i++){
		gtk_widget_show(menuitems[i]);
		gtk_menu_append(GTK_MENU(rmb_menu), menuitems[i]);
		gtk_signal_connect(GTK_OBJECT(menuitems[i]), "activate",
				   GTK_SIGNAL_FUNC(gnome_app_setpos_activate), app);
	}
}

static void gnome_app_rightclick_event(GtkWidget *widget,
				       GdkEventButton *event,
				       GnomeApp *app)
{
	int i;
	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));

	if(rmb_menu == NULL) make_rmb_menu(app);

	if(GTK_WIDGET_VISIBLE(rmb_menu))
		gtk_menu_popdown(GTK_MENU(rmb_menu));

	if(event->button != 3)
		return;

	if(widget == app->menubar){
		for(i = 0; i < 4; i++)
			gtk_widget_set_sensitive(menuitems[i],
						 (i >= 2)?FALSE:(!(app->pos_menubar==i)));
	} else {
		for(i = 0; i < 4; i++)
			gtk_widget_set_sensitive(menuitems[0],
						 !(app->pos_toolbar==i));
	}
	gtk_menu_popup(GTK_MENU(rmb_menu),
		       NULL, NULL, NULL, NULL,
		       event->button,
		       GDK_CURRENT_TIME);
	clicked_widget = widget;
}

static void
gnome_app_setpos_activate(GtkMenuItem *menu_item,
			  GnomeApp *app)
{
	int i;
	g_return_if_fail(clicked_widget != NULL);
	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));

	gtk_menu_popdown(GTK_MENU(rmb_menu));
	if(clicked_widget == app->menubar){
		for(i = 0; i < 2; i++) /* We only go through the 1st two
					  since a menubar can only go top
					  or bottom */
			if((gpointer)menu_item == (gpointer)menuitems[i]){
				gnome_app_menu_set_position (app, i);
				break;
			}
	}
	else if(clicked_widget == app->toolbar){
		for(i = 0; i < 4; i++)
			if((gpointer)menu_item == (gpointer)menuitems[i]){
				gnome_app_toolbar_set_position(app, i);
				break;
			}
	}

	clicked_widget = NULL;
}

void gnome_app_set_menus     (GnomeApp *app,
			      GtkMenuBar *menubar)
{
	GnomeAppWidgetPositionType pos;
	GtkWidget *hb;
	char *location;

	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(app->menubar == NULL);
	g_return_if_fail(menubar != NULL);
	g_return_if_fail(GTK_IS_MENU_BAR(menubar));

	hb = gtk_handle_box_new();
	gtk_widget_show(hb);
	app->menubar = GTK_WIDGET(menubar);

	gtk_signal_connect(GTK_OBJECT(hb), "button_press_event",
			   GTK_SIGNAL_FUNC(gnome_app_rightclick_event), app);
	gtk_widget_show(app->menubar);
	gtk_container_add(GTK_CONTAINER(hb), app->menubar);

	/* Load the position from the configuration file */
	gnome_config_push_prefix (app->prefix);
	location = gnome_config_get_string ("Placement/Menu=top");
	pos = get_orientation (location);

	/* Menus can not go on left or right */
	if (pos == GNOME_APP_POS_LEFT || pos == GNOME_APP_POS_RIGHT)
		pos = GNOME_APP_POS_TOP;
	gnome_app_menu_set_position (app, pos);
	free (location);
	gnome_config_pop_prefix ();
}

void gnome_app_set_toolbar   (GnomeApp *app,
			      GtkToolbar *toolbar)
{
	GnomeAppWidgetPositionType pos;
	GtkWidget *hb;
	char *location;

	g_return_if_fail(app != NULL);
	g_return_if_fail(GNOME_IS_APP(app));
	g_return_if_fail(toolbar != NULL);
	g_return_if_fail(app->toolbar == NULL);
	g_return_if_fail(GTK_IS_TOOLBAR(toolbar));

	hb = gtk_handle_box_new();
	gtk_widget_show(hb);
	app->toolbar = GTK_WIDGET(toolbar);
	gtk_signal_connect(GTK_OBJECT(hb), "button_press_event",
			   GTK_SIGNAL_FUNC(gnome_app_rightclick_event), app);
	gtk_widget_show(app->toolbar);
	gtk_container_add(GTK_CONTAINER(hb), app->toolbar);

	/* Load the position from the configuration file */
	gnome_config_push_prefix (app->prefix);
	location = gnome_config_get_string ("Placement/Toolbar=top");
	pos = get_orientation (location);
	gnome_app_menu_set_position (app, pos);
	free (location);
	gnome_config_pop_prefix ();
}

