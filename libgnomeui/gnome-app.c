#include "libgnome/gnome-defs.h"
#include "gnome-app.h"
#include "gnome-pixmap.h"
#include <string.h>

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
  app->table = gtk_table_new(3, 3, FALSE);
  gtk_widget_show(app->table);
  gtk_container_add(GTK_CONTAINER(app), app->table);

  app->pos_menubar = app->pos_toolbar = POS_TOP;
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

  /*  hb = gtk_handle_box_new(); */
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
      g_return_if_fail(pos_menubar == POS_TOP
		       || pos_menubar == POS_BOTTOM);

      app->pos_menubar = pos_menubar;

      /* It's ->parent->parent because this is inside a GtkHandleBox */
      if(app->menubar->parent->parent)
	gtk_container_remove(GTK_CONTAINER(app->menubar->parent->parent),
			     app->menubar->parent);
      /* Obviously the menu bar can't have vertical orientation,
	 so we don't support POS_LEFT or POS_RIGHT. */
      g_print("Menu bar goes from %d x %d to %d x %d\n",
	      0,
	      (pos_menubar==POS_TOP)?0:2,
	      3,
	      (pos_menubar==POS_TOP)?1:3);	      
      gtk_table_attach_defaults(GTK_TABLE(app->table),
				app->menubar->parent,
				0, 3,
				(pos_menubar==POS_TOP)?0:2,
				(pos_menubar==POS_TOP)?1:3);
    }

  if(pos_toolbar != POS_NOCHANGE)
    {
      g_return_if_fail(app->toolbar != NULL);

      app->pos_toolbar = pos_toolbar;

      /* It's ->parent->parent because this is inside a GtkHandleBox */
      if(app->toolbar->parent->parent)
	gtk_container_remove(GTK_CONTAINER(app->toolbar->parent->parent),
			     app->toolbar->parent);
      if(pos_toolbar == POS_LEFT
	 || pos_toolbar == POS_RIGHT)
	{
	  gtk_toolbar_set_orientation(GTK_TOOLBAR(app->toolbar),
				      GTK_ORIENTATION_VERTICAL);
	  gtk_table_attach_defaults(GTK_TABLE(app->table),
				    app->toolbar->parent,
				    (pos_toolbar==POS_LEFT)?0:2,
				    (pos_toolbar==POS_LEFT)?1:3,
				    0, 3);
	}
      else
	{
	  gint moffset;

	  if(app->pos_menubar == pos_toolbar)
	    moffset = (pos_toolbar == POS_TOP)?1:-1;
	  else
	    moffset = 0;
	  /* assume POS_TOP || POS_BOTTOM */
	  gtk_toolbar_set_orientation(GTK_TOOLBAR(app->toolbar),
				      GTK_ORIENTATION_HORIZONTAL);
	  gtk_table_attach_defaults(GTK_TABLE(app->table),
				    app->toolbar->parent,
				    0, 3,
				    (pos_toolbar==POS_TOP)?0:2 + moffset,
				    (pos_toolbar==POS_TOP)?1:3 + moffset);
	}
    }
  if(app->contents) /* Repack any contents of ours */
    gnome_app_set_contents(app, app->contents);
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
  g_print("Contents go from %d x %d to %d x %d\n",
			    startxs[app->pos_menubar][app->pos_toolbar],
			    startys[app->pos_menubar][app->pos_toolbar],
			    endxs[app->pos_menubar][app->pos_toolbar],
			    endys[app->pos_menubar][app->pos_toolbar]);
  gtk_table_attach_defaults(GTK_TABLE(app->table),
			    contents,
			    startxs[app->pos_menubar][app->pos_toolbar],
			    endxs[app->pos_menubar][app->pos_toolbar],
			    startys[app->pos_menubar][app->pos_toolbar],
			    endys[app->pos_menubar][app->pos_toolbar]);
  app->contents = contents;
  gtk_widget_show(contents);
}
