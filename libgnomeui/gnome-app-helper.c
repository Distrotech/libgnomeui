/*
 * GnomeApp widget by Elliot Lee
 */
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-config.h"
#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-pixmap.h"
#include <string.h>
#include <gtk/gtk.h>

static void gnome_app_do_menu_creation        (GtkWidget *parent_widget,
					       GnomeMenuInfo *menuinfo);
static void gnome_app_do_toolbar_creation     (GnomeApp *app,
					       GtkWidget *parent_widget,
					       GnomeToolbarInfo *tbinfo);

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

