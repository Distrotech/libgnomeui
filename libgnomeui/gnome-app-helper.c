/*
 * GnomeApp widget (C) 1998 Red Hat Software, Miguel de Icaza, 
 * Federico Menu, Chris Toshok.
 * Originally by Elliot Lee, with hacking by Chris Toshok for *_data,
 * and I don't know what you other people did :)
 */
#include <config.h>
#include <gdk/gdkkeysyms.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-config.h"
#include "libgnome/gnome-help.h"
#include "libgnome/gnome-i18n.h"
#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-stock.h"
#include "gnome-pixmap.h"
#include "gnome-stock.h"
#include <string.h>
#include <ctype.h>
#include <gtk/gtk.h>

struct _UIBuilderData {
  gpointer data;
  gboolean is_interp;
  GtkCallbackMarshal relay_func;
  GtkDestroyNotify destroy_func;
};
typedef struct _UIBuilderData *UIBuilderData;

static void gnome_app_do_menu_creation        (GnomeApp *app,
					       GtkWidget *parent_widget,
					       GnomeUIInfo *menuinfo,
					       UIBuilderData uidata);
static void gnome_app_do_ui_signal_connect    (GnomeApp *app,
					       GnomeUIInfo *info_item,
					       GtkWidget *widget,
					       gchar *signal_name,
					       UIBuilderData uidata);
static void gnome_app_do_ui_accelerator_setup (GnomeApp *app,
					       gchar *signal_name,
					       GtkWidget *widget,
				               GnomeUIInfo *menuinfo_item);
static void gnome_app_do_toolbar_creation     (GnomeApp *app,
					       GtkWidget *parent_widget,
					       GnomeUIInfo *tbinfo,
					       UIBuilderData uidata);
static void gnome_app_add_help_menu_entries   (GnomeApp *app,
					       GtkWidget *parent_widget,
				               GnomeUIInfo *menuinfo_item);

static void
gnome_app_do_menu_creation(GnomeApp *app,
			   GtkWidget *parent_widget,
			   GnomeUIInfo *menuinfo,
			   UIBuilderData uidata)
{
  int i;
  int has_stock_pixmaps = FALSE;

  /* first check if any of them use the stock pixmaps, because if
     they do we need to use gnome_stock_menu_item for all of them
     so the menu text will be justified properly. */
  for (i = 0; menuinfo[i].type != GNOME_APP_UI_ENDOFINFO; i++)
    {
      if (menuinfo[i].pixmap_type == GNOME_APP_PIXMAP_STOCK)
	{
	  has_stock_pixmaps = TRUE;
	  break;
	}
    }

  for(i = 0; menuinfo[i].type != GNOME_APP_UI_ENDOFINFO; i++)
    {
      switch(menuinfo[i].type)
	{

	case GNOME_APP_UI_HELP:
	  gnome_app_add_help_menu_entries(app, parent_widget, &menuinfo[i]);
	  break;

	case GNOME_APP_UI_SEPARATOR:
	case GNOME_APP_UI_ITEM:
	case GNOME_APP_UI_SUBTREE:
	  {
	    if(menuinfo[i].type == GNOME_APP_UI_SEPARATOR)
	      menuinfo[i].widget = gtk_menu_item_new();
	    else if (has_stock_pixmaps
		     || menuinfo[i].pixmap_type == GNOME_APP_PIXMAP_STOCK)
	      menuinfo[i].widget =
		gnome_stock_menu_item((menuinfo[i].pixmap_info 
				       ? menuinfo[i].pixmap_info 
				       : GNOME_STOCK_MENU_BLANK),
				      _(menuinfo[i].label));
	    else
	      menuinfo[i].widget =
		gtk_menu_item_new_with_label(_(menuinfo[i].label));

	    gtk_widget_show(menuinfo[i].widget);
	    gtk_menu_shell_append(GTK_MENU_SHELL(parent_widget),
				  menuinfo[i].widget);

	    gnome_app_do_ui_signal_connect(app, 
					   &menuinfo[i],
					   menuinfo[i].widget,
					   "activate",
					   uidata);
	    gnome_app_do_ui_accelerator_setup(app,
					      "activate",
					      menuinfo[i].widget,
					      &menuinfo[i]);

	    if(menuinfo[i].type == GNOME_APP_UI_SUBTREE)
	      {
		GtkWidget *submenu = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuinfo[i].widget),
					  submenu);
		gnome_app_do_menu_creation(app,
					   submenu,
					   menuinfo[i].moreinfo,
					   uidata);
	      }
	  }
	  break;

	default:
	  g_error("Unknown UI element type %d\n", menuinfo[i].type);
	  break;
	}
    }
  menuinfo[i].widget = parent_widget;
}

static void
gnome_app_add_help_menu_entries(GnomeApp *app,
				GtkWidget *parent_widget,
				GnomeUIInfo *menuinfo_item)
{
  char buf[BUFSIZ];
  char *topicFile, *s;
  GnomeHelpMenuEntry *entry;
  FILE *f;
  if(!menuinfo_item->moreinfo)
    menuinfo_item->moreinfo = app->name;
  topicFile = gnome_help_file_path(menuinfo_item->moreinfo, "topic.dat");
  if (!(f = fopen (topicFile, "r"))) {
    /* XXX should throw up a dialog, or perhaps default to */
    /* some standard help page                             */
    fprintf(stderr, "Unable to open %s\n", topicFile);
    g_free(topicFile);
    return;
  }
  g_free(topicFile);

  menuinfo_item->accelerator_key = GDK_KP_F1;
  menuinfo_item->ac_mods = 0;

  while (fgets(buf, sizeof(buf), f)) {
    s = buf;
    while (*s && !isspace(*s)) {
      s++;
    }
    *s++ = '\0';
    while (*s && isspace(*s)) {
      s++;
    }
    s[strlen(s) - 1] = '\0';

    entry = g_malloc(sizeof(*entry));
    entry->name = g_strdup(menuinfo_item->moreinfo);
    entry->path = g_strdup(buf);

    menuinfo_item->widget = gtk_menu_item_new_with_label(s);
    gtk_widget_show(menuinfo_item->widget);
    if(menuinfo_item->accelerator_key)
      {
	gnome_app_do_ui_accelerator_setup(app, "activate",
					  menuinfo_item->widget,
					  menuinfo_item);
	menuinfo_item->accelerator_key = 0;
      }
    gtk_menu_shell_append(GTK_MENU_SHELL(parent_widget),
			  menuinfo_item->widget);
    gtk_signal_connect(GTK_OBJECT(menuinfo_item->widget), "activate",
		       (gpointer)gnome_help_display, (gpointer)entry);
  }
  menuinfo_item->widget = NULL; /* We have a bunch of widgets,
				   so this is useless ;-) */
    
  fclose(f);
}

void
gnome_app_create_menus(GnomeApp *app,
		       GnomeUIInfo *menuinfo)
{
  GtkWidget *menubar;
  struct _UIBuilderData uidata = { NULL, FALSE, NULL, NULL};
	
  g_return_if_fail(app != NULL);
  g_return_if_fail(GNOME_IS_APP(app));
  g_return_if_fail(app->menubar == NULL);

  menubar = gtk_menu_bar_new ();
  gnome_app_set_menus (app, GTK_MENU_BAR (menubar));
	
  if(menuinfo)
    gnome_app_do_menu_creation(app, app->menubar, menuinfo, &uidata);
}

void
gnome_app_create_menus_with_data(GnomeApp *app,
				 GnomeUIInfo *menuinfo,
				 gpointer data)
{
  GtkWidget *menubar;
  struct _UIBuilderData uidata = { NULL, FALSE, NULL, NULL };
	
  g_return_if_fail(app != NULL);
  g_return_if_fail(GNOME_IS_APP(app));
  g_return_if_fail(app->menubar == NULL);
  uidata.data = data;

  menubar = gtk_menu_bar_new ();
  gnome_app_set_menus (app, GTK_MENU_BAR (menubar));
	
  if(menuinfo)
    gnome_app_do_menu_creation(app, app->menubar, menuinfo, &uidata);
}

void gnome_app_create_menus_interp      (GnomeApp *app,
					 GnomeUIInfo *menuinfo,
					 GtkCallbackMarshal relay_func,
					 gpointer data,
					 GtkDestroyNotify destroy_func)
{
  GtkWidget *menubar;
  struct _UIBuilderData uidata = { NULL, FALSE, NULL, NULL };
	
  g_return_if_fail(app != NULL);
  g_return_if_fail(GNOME_IS_APP(app));
  g_return_if_fail(app->menubar == NULL);
  uidata.data = data;
  uidata.is_interp = TRUE;
  uidata.relay_func = relay_func;
  uidata.destroy_func = destroy_func;

  menubar = gtk_menu_bar_new ();
  gnome_app_set_menus (app, GTK_MENU_BAR (menubar));
	
  if(menuinfo)
    gnome_app_do_menu_creation(app, app->menubar, menuinfo, &uidata);
}

static void
gnome_app_do_toolbar_creation(GnomeApp *app,
			      GtkWidget *parent_widget,
			      GnomeUIInfo *tbinfo,
			      UIBuilderData uidata)
{
  int i;
  GtkWidget *pmap;
	
  if(!GTK_WIDGET(app)->window)
    gtk_widget_realize(GTK_WIDGET(app));
	
  for(i = 0; tbinfo[i].type != GNOME_APP_UI_ENDOFINFO; i++)
    {
      switch(tbinfo[i].type)
	{
	case GNOME_APP_UI_ITEM:
	  {
	    switch(tbinfo[i].pixmap_type)
	      {
	      case GNOME_APP_PIXMAP_DATA:
		pmap = gnome_create_pixmap_widget_d(GTK_WIDGET(app),
						    parent_widget,
						    (char **)tbinfo[i].pixmap_info);
		break;
	      case GNOME_APP_PIXMAP_FILENAME:
		pmap = gnome_create_pixmap_widget(GTK_WIDGET(app),
						  parent_widget,
						  (char *)tbinfo[i].pixmap_info);
		break;
	      case GNOME_APP_PIXMAP_STOCK:
		pmap = gnome_stock_pixmap_widget_new(GTK_WIDGET(app),
						     (char *)tbinfo[i].pixmap_info);
		break;
	      default:
		pmap = NULL; break;
	      }

	    tbinfo[i].widget =
	      gtk_toolbar_append_item(GTK_TOOLBAR(parent_widget),
				      _(tbinfo[i].label),
				      _(tbinfo[i].hint),
				      pmap,
				      NULL, NULL);
	    gnome_app_do_ui_accelerator_setup(app,
					      "clicked",
					      tbinfo[i].widget,
					      &tbinfo[i]);
	    gnome_app_do_ui_signal_connect(app, 
					   &tbinfo[i],
					   tbinfo[i].widget,
					   "clicked",
					   uidata);
	  }
	  break;
	case GNOME_APP_UI_SEPARATOR:
	  gtk_toolbar_append_space(GTK_TOOLBAR(parent_widget));
	  break;
	default:
	  g_error("Unknown or unsupported toolbar item type %d\n",
		  tbinfo[i].type);
	  break;
	}
    }
  tbinfo[i].widget = parent_widget;
}

void gnome_app_create_toolbar(GnomeApp *app,
			      GnomeUIInfo *toolbarinfo)
{
  GtkWidget *tb;
  struct _UIBuilderData uidata = {NULL, FALSE, NULL, NULL};

  g_return_if_fail(app != NULL);
  g_return_if_fail(GNOME_IS_APP(app));
  g_return_if_fail(app->toolbar == NULL);
	
  tb = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
  gnome_app_set_toolbar (app, GTK_TOOLBAR (tb));
	
  if(toolbarinfo)
    gnome_app_do_toolbar_creation(app, tb, toolbarinfo, &uidata);
}

void gnome_app_create_toolbar_with_data(GnomeApp *app,
					GnomeUIInfo *toolbarinfo,
					gpointer data)
{
  struct _UIBuilderData uidata = {NULL, FALSE, NULL, NULL};
  GtkWidget *tb;

  g_return_if_fail(app != NULL);
  g_return_if_fail(GNOME_IS_APP(app));
  g_return_if_fail(app->toolbar == NULL);
  uidata.data = data;
	
  tb = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
  gnome_app_set_toolbar (app, GTK_TOOLBAR (tb));
	
  if(toolbarinfo)
    gnome_app_do_toolbar_creation(app, app->toolbar, toolbarinfo, &uidata);
}

void gnome_app_create_toolbar_interp    (GnomeApp *app,
					 GnomeUIInfo *tbinfo,
					 GtkCallbackMarshal relay_func,
					 gpointer data,
					 GtkDestroyNotify destroy_func)
{
  struct _UIBuilderData uidata = {NULL, FALSE, NULL, NULL};
  GtkWidget *tb;

  g_return_if_fail(app != NULL);
  g_return_if_fail(GNOME_IS_APP(app));
  g_return_if_fail(app->toolbar == NULL);
  uidata.data = data;
  uidata.is_interp = TRUE;
  uidata.relay_func = relay_func;
  uidata.destroy_func = destroy_func;
	
  tb = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
  gnome_app_set_toolbar (app, GTK_TOOLBAR (tb));
	
  if(tbinfo)
    gnome_app_do_toolbar_creation(app, app->toolbar, tbinfo, &uidata);
}

static void gnome_app_do_ui_signal_connect    (GnomeApp *app,
					       GnomeUIInfo *info_item,
					       GtkWidget *widget,
					       gchar *signal_name,
					       UIBuilderData uidata)
{
  if(uidata->is_interp)
    gtk_signal_connect_interp(GTK_OBJECT(widget),
			      signal_name,
			      uidata->relay_func,
			      uidata->data?uidata->data:info_item->moreinfo,
			      uidata->destroy_func,
			      TRUE);
  else
    gtk_signal_connect(GTK_OBJECT(widget), signal_name,
		       info_item->moreinfo,
		       uidata->data);
}

static void gnome_app_do_ui_accelerator_setup (GnomeApp *app,
					       gchar *signal_name,
					       GtkWidget *widget,
				               GnomeUIInfo *menuinfo_item)
{
  GtkAcceleratorTable *at;

  if(menuinfo_item->accelerator_key == 0)
    return;

  at = gtk_object_get_data(GTK_OBJECT(app),
			   "GtkAcceleratorTable");
  if(at == NULL)
    {
      at = gtk_accelerator_table_new();
      gtk_object_set_data(GTK_OBJECT(app),
			  "GtkAcceleratorTable",
			  (gpointer)at);
    }

  gtk_accelerator_table_install(at, GTK_OBJECT(widget),
				signal_name,
				menuinfo_item->accelerator_key,
				menuinfo_item->ac_mods);
}
