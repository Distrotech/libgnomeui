/*
 * GnomeApp widget (C) 1998 Red Hat Software, Miguel de Icaza, 
 * Federico Menu, Chris Toshok.
 * Originally by Elliot Lee, with hacking by Chris Toshok for *_data,
 * Marc Ewing added menu support, toggle and radio support,
 * and I don't know what you other people did :)
 * menu insertion/removal functions by Jaka Mocnik
 */
#include <config.h>
#include <gdk/gdkkeysyms.h>
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-config.h"
#include "libgnome/gnome-help.h"
/* Note that this file must include gnome-i18n, and not gnome-i18nP,
   so that _() is the same as the one seen by the application.  This
   is moderately bogus; we should just call gettext() directly here.  */
#include "libgnome/gnome-i18n.h"
#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-stock.h"
#include "gnome-pixmap.h"
#include "gnome-stock.h"
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <gtk/gtk.h>

void gnome_app_do_menu_creation        (GnomeApp *app,
					       GtkWidget *parent_widget,
					       gint pos,
					       GnomeUIInfo *menuinfo,
					       GnomeUIBuilderData *uidata);
void gnome_app_do_ui_signal_connect    (GnomeApp *app,
					       GnomeUIInfo *info_item,
					       gchar *signal_name,
					       GnomeUIBuilderData *uidata);
static void gnome_app_do_ui_accelerator_setup (GnomeApp *app,
					       gchar *signal_name,
				         GnomeUIInfo *menuinfo_item);
static void gnome_app_do_toolbar_creation     (GnomeApp *app,
					       GtkWidget *parent_widget,
					       GnomeUIInfo *tbinfo,
					       GnomeUIBuilderData *uidata);
static gint gnome_app_add_help_menu_entries   (GnomeApp *app,
					       GtkWidget *parent_widget,
					       gint pos,
                 GnomeUIInfo *menuinfo_item);
static gint gnome_app_add_radio_menu_entries  (GnomeApp *app,
					       GtkWidget *parent_widget,
					       gint pos,
					       GnomeUIInfo *menuinfo,
					       GnomeUIBuilderData *uidata);
static void gnome_app_add_radio_toolbar_entries(GnomeApp *app,
						GtkWidget *parent_widget,
						GnomeUIInfo *tbinfo,
						GnomeUIBuilderData *uidata);

void
gnome_app_do_menu_creation(GnomeApp *app,
			   GtkWidget *parent_widget,
			   gint pos,
			   GnomeUIInfo *menuinfo,
			   GnomeUIBuilderData *uidata)
{
  int i;
  int has_stock_pixmaps = FALSE;
  int justify_right = FALSE;
  GnomeUIBuilderData *orig_uidata;

  /* store a pointer to the original uidata structure to use it for the
     subtrees */
  orig_uidata = uidata;

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

  case GNOME_APP_UI_BUILDER_DATA:
    /* set the builder data for the following entries. this builder data is used with all the
       following items in this GnomeUIInfo array. it does not apply to the subtrees nor other
       GnomeUIInfo arrays on the same level */
    uidata = (GnomeUIBuilderData *)menuinfo[i].moreinfo;
    break;

	case GNOME_APP_UI_JUSTIFY_RIGHT:
	  justify_right = TRUE;
	  break;

	case GNOME_APP_UI_HELP:
	  pos = gnome_app_add_help_menu_entries(app, parent_widget, pos, &menuinfo[i]);
	  break;

	case GNOME_APP_UI_RADIOITEMS:
	  pos = gnome_app_add_radio_menu_entries(app, parent_widget, pos,
                                           menuinfo[i].moreinfo, uidata);
	  break;
	  
	case GNOME_APP_UI_SEPARATOR:
	case GNOME_APP_UI_ITEM:
	case GNOME_APP_UI_TOGGLEITEM:
	case GNOME_APP_UI_SUBTREE:
	  {
	    if(menuinfo[i].type == GNOME_APP_UI_SEPARATOR)
	      menuinfo[i].widget = gtk_menu_item_new();
	    else if (menuinfo[i].type == GNOME_APP_UI_TOGGLEITEM)
	      {
		menuinfo[i].widget =
		  gtk_check_menu_item_new_with_label(_(menuinfo[i].label));
		gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(menuinfo[i].widget), TRUE);
		gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(menuinfo[i].widget), FALSE);
	      }
	    else {
		    GtkWidget *pmap = NULL;
		    
		    switch(menuinfo[i].pixmap_type) {
		     case GNOME_APP_PIXMAP_DATA:
			    pmap = gnome_pixmap_new_from_xpm_d(menuinfo[i].pixmap_info);
			    break;
		     case GNOME_APP_PIXMAP_FILENAME:
			    pmap = gnome_pixmap_new_from_file(menuinfo[i].pixmap_info);
			    break;
		     case GNOME_APP_PIXMAP_NONE:
		     case GNOME_APP_PIXMAP_STOCK:
			    if (has_stock_pixmaps)
			      menuinfo[i].widget =
				gnome_stock_menu_item (menuinfo[i].pixmap_info ?
						       menuinfo[i].pixmap_info :
						       GNOME_STOCK_MENU_BLANK,
						       _(menuinfo[i].label));
			    else
			      menuinfo[i].widget = gtk_menu_item_new_with_label(_(menuinfo[i].label));
			    break;
		     default:
			    g_warning("GNOME_APP_PIXMAP type unknown.");
			    break;
		    }
		    
		    if (pmap) {
			    GtkWidget *hbox, *w;
			    
			    hbox = gtk_hbox_new(FALSE, 2);
			    gtk_box_pack_start(GTK_BOX(hbox), pmap, 
					       FALSE, FALSE, 0);
			    menuinfo[i].widget = gtk_menu_item_new();
#ifdef	GTK_HAVE_FEATURES_1_1_0
			    w = gtk_accel_label_new (_(menuinfo[i].label));
			    gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (w),
							      menuinfo[i].widget);
#else	/* !GTK_HAVE_FEATURES_1_1_0 */
			    w = gtk_label_new(_(menuinfo[i].label));
#endif	/* !GTK_HAVE_FEATURES_1_1_0 */
			    gtk_misc_set_alignment (GTK_MISC (w), 0.0, 0.5);
			    gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 0);
			    gtk_container_add(GTK_CONTAINER(menuinfo[i].widget), hbox);
			    gtk_widget_show_all(hbox);

		    }
	    }
		  
	    gtk_widget_show(menuinfo[i].widget);

	    if(justify_right)
	      gtk_menu_item_right_justify(GTK_MENU_ITEM(menuinfo[i].widget));

	    gtk_menu_shell_insert(GTK_MENU_SHELL(parent_widget), menuinfo[i].widget, pos);
	    pos++;

	    /* Only connect the signal if the item is not a subtree */

	    if (menuinfo[i].type != GNOME_APP_UI_SUBTREE)
	      uidata->connect_func(app, &menuinfo[i], "activate", uidata);

	    gnome_app_do_ui_accelerator_setup(app, "activate", &menuinfo[i]);

	    if(menuinfo[i].type == GNOME_APP_UI_SUBTREE)
	      {
		GtkWidget *submenu = gtk_menu_new();
		gtk_menu_item_set_submenu(GTK_MENU_ITEM(menuinfo[i].widget), submenu);
		gnome_app_do_menu_creation(app,
					   submenu,
					   0,
					   menuinfo[i].moreinfo,
					   orig_uidata);
	      }
	  }
	  break;

	default:
	  g_error("Unknown UI element type %d\n", menuinfo[i].type);
	  break;
	}
    }

  gtk_widget_queue_resize (parent_widget);

  menuinfo[i].widget = parent_widget;
}

static gint
gnome_app_add_radio_menu_entries(GnomeApp *app,
				 GtkWidget *parent_widget,
				 gint pos,
				 GnomeUIInfo *menuinfo,
				 GnomeUIBuilderData *uidata)
{
  GSList *group = NULL;

  g_return_val_if_fail(menuinfo != NULL, -1);
  while (menuinfo->type != GNOME_APP_UI_ENDOFINFO)
    {
      if (menuinfo->type == GNOME_APP_UI_SEPARATOR) {
        menuinfo->widget = gtk_menu_item_new();
        gtk_widget_show(menuinfo->widget);
        gtk_menu_shell_insert(GTK_MENU_SHELL(parent_widget), menuinfo->widget, pos);
        pos++;
        menuinfo++;
      }
      else if (menuinfo->type == GNOME_APP_UI_BUILDER_DATA)
        uidata = (GnomeUIBuilderData *)menuinfo->moreinfo;
      else {
        menuinfo->widget = gtk_radio_menu_item_new_with_label(group, _(menuinfo->label));
        group = gtk_radio_menu_item_group(GTK_RADIO_MENU_ITEM(menuinfo->widget));
	
        gtk_check_menu_item_set_show_toggle(GTK_CHECK_MENU_ITEM(menuinfo->widget), TRUE);
        gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(menuinfo->widget), FALSE);
        gtk_widget_show(menuinfo->widget);
        gtk_menu_shell_insert(GTK_MENU_SHELL(parent_widget), menuinfo->widget, pos);
        pos++;

        uidata->connect_func(app, menuinfo, "activate", uidata);
        gnome_app_do_ui_accelerator_setup(app, "activate", menuinfo);

        menuinfo++;
      }
    }

  return pos;
}

static gint
gnome_app_add_help_menu_entries(GnomeApp *app,
				GtkWidget *parent_widget,
				gint pos,
				GnomeUIInfo *menuinfo_item)
{
  char buf[BUFSIZ];
  char *topicFile, *s;
  GnomeHelpMenuEntry *entry;
  FILE *f;

  if(!menuinfo_item->moreinfo)
    menuinfo_item->moreinfo = app->name;

  topicFile = gnome_help_file_find_file(menuinfo_item->moreinfo, "topic.dat");

  if (!topicFile || !(f = fopen (topicFile, "r")))
    {
      /* XXX should throw up a dialog, or perhaps default to */
      /* some standard help page                             */
/*      fprintf(stderr, "Unable to open %s\n", (topicFile) ? topicFile : "(null)"); */
      g_free(topicFile);
      return -1;
    }
  g_free(topicFile);

  menuinfo_item->accelerator_key = GDK_KP_F1;
  menuinfo_item->ac_mods = 0;

  while (fgets(buf, sizeof(buf), f))
    {
      s = buf;
      while (*s && !isspace(*s))
	s++;

      *s++ = '\0';

      while (*s && isspace(*s))
	s++;

      s[strlen(s) - 1] = '\0';

      entry = g_malloc(sizeof(*entry));
      entry->name = g_strdup(menuinfo_item->moreinfo);
      entry->path = g_strdup(buf);

      menuinfo_item->widget = gtk_menu_item_new_with_label(s);
      gtk_widget_show(menuinfo_item->widget);
      if(menuinfo_item->accelerator_key)
        {
          gnome_app_do_ui_accelerator_setup(app, "activate", menuinfo_item);
          menuinfo_item->accelerator_key = 0;
        }
      gtk_menu_shell_insert(GTK_MENU_SHELL(parent_widget), menuinfo_item->widget, pos);
      pos++;

      gtk_signal_connect(GTK_OBJECT (menuinfo_item->widget), "activate",
			 (gpointer) gnome_help_display,
                         (gpointer) entry);
      }
  menuinfo_item->widget = NULL; /* We have a bunch of widgets,
				   so this is useless ;-) */
    
  fclose(f);

  return pos;
}

void
gnome_app_create_menus_custom (GnomeApp *app,
			       GnomeUIInfo *menuinfo,
			       GnomeUIBuilderData *uibdata)
{
  GtkWidget *menubar;
#ifdef GTK_HAVE_FEATURES_1_1_0
  GtkAccelGroup *ag;
#else
  GtkAcceleratorTable *at;
#endif
  int set_accel;

  g_return_if_fail(app != NULL);
  g_return_if_fail(GNOME_IS_APP(app));
  g_return_if_fail(app->menubar == NULL);

  menubar = gtk_menu_bar_new ();
  gnome_app_set_menus (app, GTK_MENU_BAR (menubar));
#ifdef GTK_HAVE_FEATURES_1_1_0	
  ag = gtk_object_get_data(GTK_OBJECT(app), "GtkAccelGroup");
  set_accel = (ag == NULL);
#else
  at = gtk_object_get_data(GTK_OBJECT(app), "GtkAcceleratorTable");
  set_accel = (at == NULL);
#endif
  if (menuinfo)
    gnome_app_do_menu_creation(app, app->menubar, 0, menuinfo, uibdata);
  if (set_accel) {
#ifdef GTK_HAVE_FEATURES_1_1_0
    ag = gtk_object_get_data(GTK_OBJECT(app), "GtkAccelGroup");
    if (ag)
      gtk_window_add_accel_group(GTK_WINDOW(app), ag);
#else
    at = gtk_object_get_data(GTK_OBJECT(app), "GtkAcceleratorTable");
    if (at)
      gtk_window_add_accelerator_table(GTK_WINDOW(app), at);
#endif
  }
}

void
gnome_app_create_menus(GnomeApp *app,
		       GnomeUIInfo *menuinfo)
{
  GnomeUIBuilderData uidata = { GNOME_UISIGFUNC(gnome_app_do_ui_signal_connect),
                                NULL, FALSE, NULL, NULL};

  gnome_app_create_menus_custom(app, menuinfo, &uidata);
}

void
gnome_app_create_menus_with_data(GnomeApp *app,
				 GnomeUIInfo *menuinfo,
				 gpointer data)
{
  GnomeUIBuilderData uidata = { GNOME_UISIGFUNC(gnome_app_do_ui_signal_connect),
                                NULL, FALSE, NULL, NULL };
	
  uidata.data = data;

  gnome_app_create_menus_custom(app, menuinfo, &uidata);
}

void
gnome_app_create_menus_interp (GnomeApp *app,
			       GnomeUIInfo *menuinfo,
			       GtkCallbackMarshal relay_func,
			       gpointer data,
			       GtkDestroyNotify destroy_func)
{
  GnomeUIBuilderData uidata = { GNOME_UISIGFUNC(gnome_app_do_ui_signal_connect),
                                NULL, FALSE, NULL, NULL };

  uidata.data = data;
  uidata.is_interp = TRUE;
  uidata.relay_func = relay_func;
  uidata.destroy_func = destroy_func;

  gnome_app_create_menus_custom(app, menuinfo, &uidata);
}

static void
gnome_app_do_toolbar_creation(GnomeApp *app,
			      GtkWidget *parent_widget,
			      GnomeUIInfo *tbinfo,
			      GnomeUIBuilderData *uidata)
{
  int i, set_accel;
  GtkWidget *pmap;
#ifdef GTK_HAVE_FEATURES_1_1_0
  GtkAccelGroup *ag;

  set_accel = (NULL == gtk_object_get_data(GTK_OBJECT(app),
					   "GtkAccelGroup"));
#else
  GtkAcceleratorTable *at;

  set_accel = (NULL == gtk_object_get_data(GTK_OBJECT(app),
                                           "GtkAcceleratorTable"));
#endif

  if(!GTK_WIDGET(app)->window)
    gtk_widget_realize(GTK_WIDGET(app));
	
  for(i = 0; tbinfo[i].type != GNOME_APP_UI_ENDOFINFO; i++)
    {
      switch(tbinfo[i].type)
	{
  case GNOME_APP_UI_BUILDER_DATA:
    /* set the builder data for the following entries. this builder data is used with all the
       following items in this GnomeUIInfo array. it does not apply to the subtrees nor other
       GnomeUIInfo arrays on the same level */
    uidata = (GnomeUIBuilderData *)tbinfo[i].moreinfo;
    break;

	case GNOME_APP_UI_RADIOITEMS:
	  gnome_app_add_radio_toolbar_entries(app, parent_widget,
					      tbinfo[i].moreinfo, uidata);
	  break;

	case GNOME_APP_UI_ITEM:
	case GNOME_APP_UI_TOGGLEITEM:
	  {
	    switch(tbinfo[i].pixmap_type)
	      {
	      case GNOME_APP_PIXMAP_DATA:
		pmap = gnome_pixmap_new_from_xpm_d(tbinfo[i].pixmap_info);
		break;
	      case GNOME_APP_PIXMAP_FILENAME:
		pmap = gnome_pixmap_new_from_file(tbinfo[i].pixmap_info);
		break;
	      case GNOME_APP_PIXMAP_STOCK:
		pmap = gnome_stock_pixmap_widget_new(GTK_WIDGET(app), tbinfo[i].pixmap_info);
		break;
	      default:
		pmap = NULL; break;
	      }
	    
	    tbinfo[i].widget =
	      gtk_toolbar_append_element(GTK_TOOLBAR(parent_widget),
					 tbinfo[i].type == GNOME_APP_UI_ITEM ?
					 GTK_TOOLBAR_CHILD_BUTTON :
					 GTK_TOOLBAR_CHILD_TOGGLEBUTTON,
					 NULL,
					 _(tbinfo[i].label),
					 _(tbinfo[i].hint),
					 NULL,
					 pmap,
					 NULL, NULL);

	    uidata->connect_func(app, &tbinfo[i], "clicked", uidata);
	    gnome_app_do_ui_accelerator_setup(app, "clicked", &tbinfo[i]);
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
  if (set_accel) {
#ifdef GTK_HAVE_FEATURES_1_1_0
    ag = gtk_object_get_data(GTK_OBJECT(app), "GtkAccelGroup");
    if (ag)
      gtk_window_add_accel_group(GTK_WINDOW(app), ag);
#else
    at = gtk_object_get_data(GTK_OBJECT(app), "GtkAcceleratorTable");
    if (at)
      gtk_window_add_accelerator_table(GTK_WINDOW(app), at);
#endif
  }
}

static void
gnome_app_add_radio_toolbar_entries(GnomeApp *app,
				    GtkWidget *parent_widget,
				    GnomeUIInfo *tbinfo,
				    GnomeUIBuilderData *uidata)
{
  GtkWidget *group = NULL;
  GtkWidget *pmap;

  g_return_if_fail(tbinfo != NULL);
    
  while (tbinfo->type != GNOME_APP_UI_ENDOFINFO)
    {
      if(tbinfo->type == GNOME_APP_UI_BUILDER_DATA)
         uidata = (GnomeUIBuilderData *)tbinfo->moreinfo;
      else {
        switch(tbinfo->pixmap_type) {
        case GNOME_APP_PIXMAP_DATA:
          pmap = gnome_pixmap_new_from_xpm_d(tbinfo->pixmap_info);
          break;

        case GNOME_APP_PIXMAP_FILENAME:
          pmap = gnome_pixmap_new_from_file((char *)tbinfo->pixmap_info);
          break;

        case GNOME_APP_PIXMAP_STOCK:
          pmap = gnome_stock_pixmap_widget_new(GTK_WIDGET(app), tbinfo->pixmap_info);
          break;

        default:
          pmap = NULL; break;
        }

        group = tbinfo->widget =
          gtk_toolbar_append_element(GTK_TOOLBAR(parent_widget),
                                     GTK_TOOLBAR_CHILD_RADIOBUTTON,
                                     group, _(tbinfo->label),
                                     _(tbinfo->hint), NULL,
                                     pmap, NULL, NULL);

        uidata->connect_func(app, tbinfo, "clicked", uidata);

        gnome_app_do_ui_accelerator_setup(app, "clicked", tbinfo);
	
        tbinfo++;
      }
    }
}

void
gnome_app_create_toolbar_custom (GnomeApp *app,
				 GnomeUIInfo *tbinfo,
				 GnomeUIBuilderData *uibdata)
{
  GtkWidget *tb;

  g_return_if_fail(app != NULL);
  g_return_if_fail(GNOME_IS_APP(app));
  g_return_if_fail(app->toolbar == NULL);
	
  tb = gtk_toolbar_new (GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
  gnome_app_set_toolbar (app, GTK_TOOLBAR (tb));

  if(tbinfo)
    gnome_app_do_toolbar_creation(app, tb, tbinfo, uibdata);
}

void
gnome_app_create_toolbar(GnomeApp *app,
			 GnomeUIInfo *toolbarinfo)
{
  GnomeUIBuilderData uidata = { GNOME_UISIGFUNC(gnome_app_do_ui_signal_connect),
                                NULL, FALSE, NULL, NULL };

  gnome_app_create_toolbar_custom(app, toolbarinfo, &uidata);
}

void
gnome_app_create_toolbar_with_data(GnomeApp *app,
				   GnomeUIInfo *toolbarinfo,
				   gpointer data)
{
  GnomeUIBuilderData uidata = { GNOME_UISIGFUNC(gnome_app_do_ui_signal_connect),
                                NULL, FALSE, NULL, NULL };

  uidata.data = data;
  gnome_app_create_toolbar_custom(app, toolbarinfo, &uidata);
}

void
gnome_app_create_toolbar_interp (GnomeApp *app,
				 GnomeUIInfo *tbinfo,
				 GtkCallbackMarshal relay_func,
				 gpointer data,
				 GtkDestroyNotify destroy_func)
{
  GnomeUIBuilderData uidata = { GNOME_UISIGFUNC(gnome_app_do_ui_signal_connect),
                                NULL, FALSE, NULL, NULL };

  uidata.data = data;
  uidata.is_interp = TRUE;
  uidata.relay_func = relay_func;
  uidata.destroy_func = destroy_func;

  gnome_app_create_toolbar_custom(app, tbinfo, &uidata);
}

void
gnome_app_do_ui_signal_connect (GnomeApp *app,
				GnomeUIInfo *info_item,
				gchar *signal_name,
				GnomeUIBuilderData *uidata)
{
  if(uidata->is_interp)
    gtk_signal_connect_interp(GTK_OBJECT(info_item->widget),
			      signal_name,
			      uidata->relay_func,
			      uidata->data?uidata->data:info_item->user_data,
			      uidata->destroy_func,
			      TRUE);
  else
    gtk_signal_connect(GTK_OBJECT(info_item->widget), signal_name,
		       info_item->moreinfo,
		       uidata->data?uidata->data:info_item->user_data);
}

static void
gnome_app_do_ui_accelerator_setup (GnomeApp *app,
				   gchar *signal_name,
				   GnomeUIInfo *menuinfo_item)
{
#ifdef GTK_HAVE_FEATURES_1_1_0
  GtkAccelGroup *ag;
#else
  GtkAcceleratorTable *at;
#endif
  if (menuinfo_item->accelerator_key == 0)
    return;
#ifdef GTK_HAVE_FEATURES_1_1_0
  ag = gtk_object_get_data(GTK_OBJECT(app),
			   "GtkAccelGroup");
  if (ag == NULL)
    {
      ag = gtk_accel_group_new();
      gtk_object_set_data(GTK_OBJECT(app),
			  "GtkAccelGroup",
			  (gpointer)ag);
    }

  gtk_widget_add_accelerator(GTK_WIDGET(menuinfo_item->widget),
				   signal_name,
				   ag,
				   menuinfo_item->accelerator_key,
				   menuinfo_item->ac_mods,
				   GTK_ACCEL_VISIBLE);
#else
  at = gtk_object_get_data(GTK_OBJECT(app),
                           "GtkAcceleratorTable");
  if (at == NULL)
    {
      at = gtk_accelerator_table_new();
      gtk_object_set_data(GTK_OBJECT(app),
                          "GtkAcceleratorTable",
                          (gpointer)at);
    }

  gtk_widget_install_accelerator(GTK_WIDGET(menuinfo_item->widget), at,
                                   signal_name,
                                   menuinfo_item->accelerator_key,
                                   menuinfo_item->ac_mods);
#endif
}

/*
 * menu insertion/removal functions
 * <jaka.mocnik@kiss.uni-lj.si>
 *
 * the path argument should be in the form "File/.../.../Something".
 * "" will insert the item as the first one in the menubar
 * "File/" will insert it as the first one in the File menu
 * "File/Settings" will insert it after the Setting item in the File menu
 * use of  "File/<Separator>" should be obvious. however this stops after the first separator.
 * I hope this explains use of the insert/remove functions well enough.
 */

/*
 * p = gnome_app_find_menu_pos(top, path, &pos)
 * finds menu item described by path (see below for details) starting in the GtkMenuShell top
 * and returns its parent GtkMenuShell and the position after this item in pos:
 * gtk_menu_shell_insert(p, w, pos) would then insert widget w in GtkMenuShell p right after
 * the menu item described by path.
 */
GtkWidget *
gnome_app_find_menu_pos (GtkWidget *parent,
			 gchar *path,
			 gint *pos)
{
  GtkBin *item;
  gchar *label = NULL;
  GList *children;
  gchar *name_end;
  gint p, path_len;

  g_return_val_if_fail(parent != NULL, NULL);
  g_return_val_if_fail(path != NULL, NULL);
  g_return_val_if_fail(pos != NULL, NULL);

  children = GTK_MENU_SHELL(parent)->children;

  name_end = strchr(path, '/');
  if(name_end == NULL)
    path_len = strlen(path);
  else
    path_len = name_end - path;

  if (path_len == 0) {
    *pos = 0;
    return parent;
  }

  p = 0;

  while( children ) {
    p++;
    item = GTK_BIN(children->data);

    if(!item->child)                    /* this is a separator, right? */
      label = "<Separator>";
    else if(GTK_IS_LABEL(item->child))  /* a simple item with a label */
      label = GTK_LABEL(item->child)->label;
    else if(GTK_IS_HBOX(item->child))   /* an item with a hbox (pixmap + label) */
      label = GTK_LABEL(g_list_next(gtk_container_children(GTK_CONTAINER(item->child)))->data)->label;
    else                                /* something that we just can't handle */
      label = NULL;

    if( label && (strncmp(path, label, path_len) == 0) ) {
      if(name_end == NULL) {
        *pos = p;
        return parent;
      }
      else if(GTK_MENU_ITEM(item)->submenu)
        return gnome_app_find_menu_pos(GTK_MENU_ITEM(item)->submenu, (gchar *)(name_end + 1), pos);
      else
        return NULL;
    }

    children = g_list_next(children);
  }

  return NULL;
}

/*
 * gnome_app_remove_menus(app, path, num) removes num items from the existing app's menu structure
 * begining with item described by path
 */
void
gnome_app_remove_menus(GnomeApp *app,
		       gchar *path,
		       gint items)
{
  GtkWidget *parent, *child;
  GList *children;
  gint pos;

  g_return_if_fail(app != NULL);
  g_return_if_fail(GNOME_IS_APP(app));

  /* find the first item (which is actually at position pos-1) to remove */
  parent = gnome_app_find_menu_pos(app->menubar, path, &pos);

  /* in case of path ".../" remove the first item */
  if(pos == 0)
    pos = 1;

  if( parent == NULL ) {
    g_warning("gnome_app_remove_menus: couldn't find first item to remove!");
    return;
  }

  /* remove items */
  children = g_list_nth(GTK_MENU_SHELL(parent)->children, pos - 1);
  while(children && items > 0) {
    child = GTK_WIDGET(children->data);
    /* children = g_list_next(children); */
    gtk_container_remove(GTK_CONTAINER(parent), child);
    children = g_list_nth(GTK_MENU_SHELL(parent)->children, pos - 1);
    items--;
  }

  gtk_widget_queue_resize(parent);
}

/*
 * gnome_app_insert_menus_custom(app, path, info, uibdata) inserts menus described by info
 * in existing app's menu structure right after the item described by path.
 */
void
gnome_app_insert_menus_custom (GnomeApp *app,
			       gchar *path,
			       GnomeUIInfo *menuinfo,
			       GnomeUIBuilderData *uibdata)
{
  GtkWidget *parent;
#ifndef GTK_HAVE_FEATURES_1_1_0
  GtkAcceleratorTable *at;
#endif
  gint pos;

  g_return_if_fail(app != NULL);
  g_return_if_fail(GNOME_IS_APP(app));
  g_return_if_fail(app->menubar != NULL);

  /* find the parent menushell and position for insertion of menus */
  parent = gnome_app_find_menu_pos(app->menubar, path, &pos);
  if(parent == NULL) {
    g_warning("gnome_app_insert_menus_custom: couldn't find insertion point for menus!");
    return;
  }

  /* create menus and insert them */
  gnome_app_do_menu_creation(app, parent, pos, menuinfo, uibdata);

  /* for the moment we don't set the accelerators */
#if 0
  at = gtk_object_get_data(GTK_OBJECT(app), "GtkAcceleratorTable");
  if (at)
    gtk_window_add_accelerator_table(GTK_WINDOW(app), at);
#endif
}

void
gnome_app_insert_menus(GnomeApp *app,
		       gchar *path,
		       GnomeUIInfo *menuinfo)
{
  GnomeUIBuilderData uidata = { GNOME_UISIGFUNC(gnome_app_do_ui_signal_connect),
                                NULL, FALSE, NULL, NULL};

  gnome_app_insert_menus_custom(app, path, menuinfo, &uidata);
}

void
gnome_app_insert_menus_with_data(GnomeApp *app,
				 gchar *path,
				 GnomeUIInfo *menuinfo,
				 gpointer data)
{
  GnomeUIBuilderData uidata = { GNOME_UISIGFUNC(gnome_app_do_ui_signal_connect),
                                NULL, FALSE, NULL, NULL };
	
  uidata.data = data;

  gnome_app_insert_menus_custom(app, path, menuinfo, &uidata);
}

void
gnome_app_insert_menus_interp (GnomeApp *app,
			       gchar *path,
			       GnomeUIInfo *menuinfo,
			       GtkCallbackMarshal relay_func,
			       gpointer data,
			       GtkDestroyNotify destroy_func)
{
  GnomeUIBuilderData uidata = { GNOME_UISIGFUNC(gnome_app_do_ui_signal_connect),
                                NULL, FALSE, NULL, NULL };

  uidata.data = data;
  uidata.is_interp = TRUE;
  uidata.relay_func = relay_func;
  uidata.destroy_func = destroy_func;

  gnome_app_insert_menus_custom(app, path, menuinfo, &uidata);
}
