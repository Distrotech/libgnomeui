#include "libgnome/gnome-defs.h"
#include "gnome-app.h"
#include "gnome-pixmap.h"

static void gnome_app_class_init(GnomeAppClass *klass);
static void gnome_app_init(GnomeApp *app);
static void gnome_app_do_menu_creation(GtkWidget *parent_widget,
				       GnomeMenuInfo *menuinfo);
static void gnome_app_do_toolbar_creation(GnomeApp *app,
					  GtkWidget *parent_widget,
					  GnomeToolbarInfo *tbinfo);

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
  }
  return gnomeapp_type;
}

static void
gnome_app_class_init(GnomeAppClass *klass)
{
}

static void
gnome_app_init(GnomeApp *app)
{
  app->menubar = app->toolbar = app->contents = NULL;

  app->vtable = gtk_table_new(3, 1, FALSE);
  gtk_widget_show(app->vtable);
  app->htable = gtk_table_new(1, 3, FALSE);
  gtk_widget_show(app->htable);
}

GtkWidget *
gnome_app_new(gchar *appname)
{
  GtkWidget *retval;

  retval = gtk_type_new(gnome_app_get_type());
  if(appname)
    gtk_window_set_title(GTK_WINDOW(retval), appname);

  return retval;
}

static void
gnome_app_do_menu_creation(GtkWidget *parent_widget,
			   GnomeMenuInfo *menuinfo)
{
  int i;
  for(i = 0; menuinfo[i].type != MI_ENDOFINFO; i++)
    {
      menuinfo[i].widget = gtk_menu_item_new_with_label(menuinfo[i].label);
      gtk_widget_show(menuinfo[i].widget);
      gtk_menu_shell_append(GTK_MENU_SHELL(parent_widget),
			    menuinfo[i].widget);

      if(menuinfo[i].type == MI_ITEM)
	{
	  gtk_signal_connect(GTK_OBJECT(menuinfo[i].widget), "activate",
			     menuinfo[i].moreinfo, NULL);
	}
      else if(menuinfo[i].type == MI_SUBMENU)
	{
	  GtkWidget *submenu;
	  submenu = gtk_menu_new();
	  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuinfo[i].widget),
				    submenu);
	  gtk_widget_show(submenu);
	  gnome_app_do_menu_creation(submenu, menuinfo[i].moreinfo);
	}
    }
}

void
gnome_app_create_menu(GnomeApp *app,
		      GnomeMenuInfo *menuinfo)
{
  GtkWidget *hb;
  int i;

  g_return_if_fail(app != NULL);
  g_return_if_fail(GNOME_IS_APP(app));
  g_return_if_fail(app->menubar == NULL);

  hb = gtk_handle_box_new();
  gtk_widget_show(hb);
  app->menubar = gtk_menu_bar_new();
  gtk_widget_show(app->menubar);
  gtk_container_add(GTK_CONTAINER(hb), app->menubar);

  if(menuinfo)
    gnome_app_do_menu_creation(app->menubar, menuinfo);

  gnome_app_set_positions(app,
			  POS_TOP,
			  POS_NOCHANGE);
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

  for(i = 0; tbinfo[i].type != TBI_ENDOFINFO; i++)
    {
      if(tbinfo[i].type == TBI_ITEM)
	{
	  if(tbinfo[i].pixmap_type == PMAP_DATA)
	    pmap = gnome_create_pixmap_widget_d(GTK_WIDGET(app),
						parent_widget,
						(char **)tbinfo[i].pixmap_info);
	  else if(tbinfo[i].pixmap_type == PMAP_FILENAME)
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
      else if(tbinfo[i].type == TBI_SPACE)
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

  hb = gtk_handle_box_new();
  gtk_widget_show(hb);
  app->toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL,
				 GTK_TOOLBAR_BOTH);
  gtk_widget_show(app->toolbar);
  gtk_container_add(GTK_CONTAINER(hb), app->toolbar);

  if(toolbarinfo)
    gnome_app_do_toolbar_creation(app, app->toolbar, toolbarinfo);

  gnome_app_set_positions(app,
			  POS_NOCHANGE,
			  POS_BOTTOM);
}

void
gnome_app_set_positions(GnomeApp *app,
			GnomeAppWidgetPositionType pos_menubar,
			GnomeAppWidgetPositionType pos_toolbar)
{
  g_return_if_fail(app != NULL);
  g_return_if_fail(GNOME_IS_APP(app));

  if(pos_menubar != POS_NOCHANGE)
    {
      g_return_if_fail(app->menubar != NULL);

      /* It's ->parent->parent because this is inside a GtkHandleBox */
      if(app->menubar->parent->parent)
	gtk_container_remove(GTK_CONTAINER(app->menubar->parent->parent),
			     app->menubar);
      /* Obviously the menu bar can't have vertical orientation,
	 so we don't support POS_LEFT or POS_RIGHT. */
      gtk_table_attach_defaults(GTK_TABLE(app->vtable),
				app->menubar->parent,
				FALSE,
				FALSE,
				pos_menubar==POS_TOP?TRUE:FALSE,
				pos_menubar==POS_BOTTOM?TRUE:FALSE);

    }

  if(pos_toolbar != POS_NOCHANGE)
    {
      g_return_if_fail(app->toolbar != NULL);

      /* It's ->parent->parent because this is inside a GtkHandleBox */
      if(app->toolbar->parent->parent)
	gtk_container_remove(GTK_CONTAINER(app->toolbar->parent->parent),
			     app->toolbar);
      if(pos_toolbar == POS_LEFT
	 || pos_toolbar == POS_RIGHT)
	{
	  gtk_table_attach_defaults(GTK_TABLE(app->htable),
				    app->toolbar->parent,
				    pos_toolbar==POS_LEFT?TRUE:FALSE,
				    pos_toolbar==POS_RIGHT?TRUE:FALSE,
				    FALSE,
				    FALSE);
	  gtk_toolbar_set_orientation(GTK_TOOLBAR(app->toolbar),
				      GTK_ORIENTATION_VERTICAL);
	}
      else
	{
	  /* assume POS_TOP || POS_BOTTOM */
	  gtk_table_attach_defaults(GTK_TABLE(app->vtable),
				    app->toolbar->parent,
				    FALSE,
				    FALSE,
				    pos_toolbar==POS_TOP?TRUE:FALSE,
				    pos_toolbar==POS_BOTTOM?TRUE:FALSE);
	  gtk_toolbar_set_orientation(GTK_TOOLBAR(app->toolbar),
				      GTK_ORIENTATION_HORIZONTAL);
	}
    }
}

void
gnome_app_set_contents(GnomeApp *app, GtkWidget *contents)
{
  g_return_if_fail(app != NULL);
  g_return_if_fail(GNOME_IS_APP(app));

  if(app->contents != NULL)
    gtk_container_remove(GTK_CONTAINER(app->htable), app->contents);

  /* Is this going to work at all? I'll wager not */
  gtk_table_attach_defaults(GTK_TABLE(app->htable),
			    contents,
			    FALSE, FALSE,
			    FALSE, FALSE);
  app->contents = contents;
  gtk_widget_show(contents);
}
