/* Copyright (C) 1998 Red Hat Software, Miguel de Icaza, Federico Mena,
 * Chris Toshok.
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */
/*
 * Originally by Elliot Lee, with hacking by Chris Toshok for *_data, Marc
 * Ewing added menu support, toggle and radio support, and I don't know what
 * you other people did :) menu insertion/removal functions by Jaka Mocnik.
 * Small fixes and documentation by Justin Maurer.
 *
 * Major cleanups and rearrangements by Federico Mena and Justin Maurer.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include <libgnome/gnome-util.h>
#include <libgnome/gnome-help.h>
#include <libgnome/gnome-config.h>

/* Note that this file must include gnome-i18n, and not gnome-i18nP, so that
 * _() is the same as the one seen by the application.  This is moderately
 * bogus; we should just call gettext() directly here.
 */

#include <libgnome/gnome-i18n.h>
#include <libgnome/gnome-program.h>
#include <libgnome/gnome-gconf.h>
#include <libgnome/gnome-init.h>
#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-uidefs.h"
#include "gnome-stock-icons.h"
#include "gnome-gconf-ui.h"

/* keys used for get/set_data */
static const char *gnome_app_helper_gconf_client = "gnome-app-helper-gconf-client";
static const char *gnome_app_helper_menu_hint = "gnome-app-helper:menu-hint";
static const char *gnome_app_helper_pixmap_type = "gnome-app-helper-pixmap-type";
static const char *gnome_app_helper_pixmap_info = "gnome-app-helper-pixmap-info";
static const char *apphelper_statusbar_hint = "apphelper_statusbar_hint";
static const char *apphelper_appbar_hint = "apphelper_appbar_hint";

/* prototypes */
static gint g_strncmp_ignore_char( const gchar *first, const gchar *second,
				   gint length, gchar ignored );


#ifdef NEVER_DEFINED
static const char strings [] = {
	N_("_File"),
	N_("_File/"),
	N_("_Edit"),
	N_("_Edit/"),
	N_("_View"),
	N_("_View/"),
	N_("_Settings"),
	N_("_Settings/"),
        N_("_New"),
	N_("_New/"),
	N_("Fi_les"),
	N_("Fi_les/"),
	N_("_Windows"),
	N_("_Game"),
	N_("_Help"),
	N_("_Windows/")
};

#endif

static GnomeUIInfo menu_defaults[] = {
        /* New */
        { GNOME_APP_UI_ITEM, NULL, NULL,
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GTK_STOCK_NEW,
          GNOME_KEY_NAME_NEW, GNOME_KEY_MOD_NEW, NULL },
        /* Open */
        { GNOME_APP_UI_ITEM, N_("_Open..."), N_("Open a file"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GTK_STOCK_OPEN,
          GNOME_KEY_NAME_OPEN, GNOME_KEY_MOD_OPEN, NULL },
	/* Save */
        { GNOME_APP_UI_ITEM, N_("_Save"), N_("Save the current file"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GTK_STOCK_SAVE,
          GNOME_KEY_NAME_SAVE, GNOME_KEY_MOD_SAVE, NULL },
	/* Save As */
        { GNOME_APP_UI_ITEM, N_("Save _As..."),
          N_("Save the current file with a different name"),
          NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GTK_STOCK_SAVE_AS,
          GNOME_KEY_NAME_SAVE_AS, GNOME_KEY_MOD_SAVE_AS, NULL },
	/* Revert */
        { GNOME_APP_UI_ITEM, N_("_Revert"),
          N_("Revert to a saved version of the file"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GTK_STOCK_REVERT_TO_SAVED,
          0,  (GdkModifierType) 0, NULL },
	/* Print */
        { GNOME_APP_UI_ITEM, N_("_Print..."), N_("Print the current file"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GTK_STOCK_PRINT,
          GNOME_KEY_NAME_PRINT,  GNOME_KEY_MOD_PRINT, NULL },
	/* Print Setup */
        { GNOME_APP_UI_ITEM, N_("Print S_etup..."),
          N_("Setup the page settings for your current printer"),
          NULL, NULL,
	  NULL, GNOME_APP_PIXMAP_STOCK, GTK_STOCK_PRINT,
          GNOME_KEY_NAME_PRINT_SETUP,  GNOME_KEY_MOD_PRINT_SETUP, NULL },
	/* Close */
        { GNOME_APP_UI_ITEM, N_("_Close"), N_("Close the current file"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GTK_STOCK_CLOSE,
          GNOME_KEY_NAME_CLOSE, GNOME_KEY_MOD_CLOSE, NULL },
	/* Exit */
        { GNOME_APP_UI_ITEM, N_("_Quit"), N_("Quit the application"),
          NULL, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GTK_STOCK_QUIT, GNOME_KEY_NAME_QUIT, GNOME_KEY_MOD_QUIT,
	    NULL },
	/*
	 * The "Edit" menu
	 */
	/* Cut */
        { GNOME_APP_UI_ITEM, N_("Cu_t"), N_("Cut the selection"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GTK_STOCK_CUT,
          GNOME_KEY_NAME_CUT, GNOME_KEY_MOD_CUT, NULL },
	/* 10 */
	/* Copy */
        { GNOME_APP_UI_ITEM, N_("_Copy"), N_("Copy the selection"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GTK_STOCK_COPY,
          GNOME_KEY_NAME_COPY, GNOME_KEY_MOD_COPY, NULL },
	/* Paste */
        { GNOME_APP_UI_ITEM, N_("_Paste"), N_("Paste the clipboard"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GTK_STOCK_PASTE,
          GNOME_KEY_NAME_PASTE, GNOME_KEY_MOD_PASTE, NULL },
	/* Clear */
        { GNOME_APP_UI_ITEM, N_("C_lear"), N_("Clear the selection"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GTK_STOCK_CLEAR,
          GNOME_KEY_NAME_CLEAR, GNOME_KEY_MOD_CLEAR, NULL },
	/* Undo */
        { GNOME_APP_UI_ITEM, N_("_Undo"), N_("Undo the last action"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GTK_STOCK_UNDO,
          GNOME_KEY_NAME_UNDO, GNOME_KEY_MOD_UNDO, NULL },
	/* Redo */
        { GNOME_APP_UI_ITEM, N_("_Redo"), N_("Redo the undone action"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GTK_STOCK_REDO,
          GNOME_KEY_NAME_REDO, GNOME_KEY_MOD_REDO, NULL },
	/* Find */
        { GNOME_APP_UI_ITEM, N_("_Find..."),  N_("Search for a string"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GTK_STOCK_FIND,
          GNOME_KEY_NAME_FIND, GNOME_KEY_MOD_FIND, NULL },
	/* Find Again */
        { GNOME_APP_UI_ITEM, N_("Find Ne_xt"),
          N_("Search again for the same string"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GTK_STOCK_FIND,
          GNOME_KEY_NAME_FIND_AGAIN, GNOME_KEY_MOD_FIND_AGAIN, NULL },
	/* Replace */
        { GNOME_APP_UI_ITEM, N_("R_eplace..."), N_("Replace a string"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GTK_STOCK_FIND_AND_REPLACE,
          GNOME_KEY_NAME_REPLACE, GNOME_KEY_MOD_REPLACE, NULL },
	/* Properties */
        { GNOME_APP_UI_ITEM, N_("_Properties"),
          N_("Modify the file's properties"),
          NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GTK_STOCK_PROPERTIES,
          0,  (GdkModifierType) 0, NULL },
	/*
	 * The Settings menu
	 */
	/* Settings */
        { GNOME_APP_UI_ITEM, N_("Prefere_nces"),
          N_("Configure the application"),
          NULL, NULL,
	  NULL, GNOME_APP_PIXMAP_STOCK, GTK_STOCK_PREFERENCES,
          0,  (GdkModifierType) 0, NULL },
	/* 20 */
	/*
	 * And the "Help" menu
	 */
	/* About */
        { GNOME_APP_UI_ITEM, N_("_About"),
          N_("About this application"), NULL, NULL,
	  NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_ABOUT,
          0,  (GdkModifierType) 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("Select _All"),
          N_("Select everything"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_NONE, NULL,
          GNOME_KEY_NAME_SELECT_ALL, GNOME_KEY_MOD_SELECT_ALL, NULL },

	/*
	 * Window menu
	 */
        { GNOME_APP_UI_ITEM, N_("Create New _Window"),
          N_("Create a new window"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_NONE, NULL,
          GNOME_KEY_NAME_NEW_WINDOW, GNOME_KEY_MOD_NEW_WINDOW, NULL },
        { GNOME_APP_UI_ITEM, N_("_Close This Window"),
          N_("Close the current window"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_NONE, NULL,
          GNOME_KEY_NAME_CLOSE_WINDOW, GNOME_KEY_MOD_CLOSE_WINDOW, NULL },

	/*
	 * The "Game" menu
	 */
        { GNOME_APP_UI_ITEM, N_("_New Game"),
          N_("Start a new game"),
	  NULL, NULL, NULL,
          GNOME_APP_PIXMAP_NONE, NULL,
          GNOME_KEY_NAME_NEW_GAME,  GNOME_KEY_MOD_NEW_GAME, NULL },
        { GNOME_APP_UI_ITEM, N_("_Pause Game"),
          N_("Pause the game"),
	  NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_TIMER_STOP,
          GNOME_KEY_NAME_PAUSE_GAME,  GNOME_KEY_MOD_PAUSE_GAME, NULL },
        { GNOME_APP_UI_ITEM, N_("_Restart Game"),
          N_("Restart the game"),
	  NULL, NULL, NULL,
          GNOME_APP_PIXMAP_NONE, NULL,
          0,  0, NULL },
        { GNOME_APP_UI_ITEM, N_("_Undo Move"),
          N_("Undo the last move"),
	  NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GTK_STOCK_UNDO,
          GNOME_KEY_NAME_UNDO_MOVE,  GNOME_KEY_MOD_UNDO_MOVE, NULL },
        { GNOME_APP_UI_ITEM, N_("_Redo Move"),
          N_("Redo the undone move"),
	  NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GTK_STOCK_REDO,
          GNOME_KEY_NAME_REDO_MOVE,  GNOME_KEY_MOD_REDO_MOVE, NULL },
        { GNOME_APP_UI_ITEM, N_("_Hint"),
          N_("Get a hint for your next move"),
	  NULL, NULL, NULL,
          GNOME_APP_PIXMAP_NONE, NULL,
          0,  0, NULL },
	/* 30 */
        { GNOME_APP_UI_ITEM, N_("_Scores..."),
          N_("View the scores"),
	  NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_SCORES,
          0,  (GdkModifierType) 0, NULL },
        { GNOME_APP_UI_ITEM, N_("_End Game"),
          N_("End the current game"),
	  NULL, NULL, NULL,
          GNOME_APP_PIXMAP_NONE, NULL,
          0,  (GdkModifierType) 0, NULL }
};

static gchar *menu_names[] =
{
  /* 0 */
  "new",
  "open",
  "save",
  "save-as",
  "revert",
  "print",
  "print-setup",
  "close",
  "quit",
  "cut",
  /* 10 */
  "copy",
  "paste",
  "clear",
  "undo",
  "redo",
  "find",
  "find-again",
  "replace",
  "properties",
  "preferences",
  /* 20 */
  "about",
  "select-all",
  "new-window",
  "close-window",
  "new-game",
  "pause-game",
  "restart-game",
  "undo-move",
  "redo-move",
  "hint",
  /* 30 */
  "scores",
  "end-game"
};


static GtkWidget *
scale_pixbuf (GdkPixbuf *pixbuf, GtkIconSize icon_size) {
	double pix_x, pix_y;
	int width, height;

	gtk_icon_size_lookup (icon_size, &width, &height);

        pix_x = gdk_pixbuf_get_width (pixbuf);
        pix_y = gdk_pixbuf_get_height (pixbuf);

	/* Only scale if the image doesn't match the required
	 * icon size 
	 */
        if (pix_x > width || pix_y > height) {
        	double greatest;
        	GdkPixbuf *scaled;
		GtkWidget *image;

                greatest = pix_x > pix_y ? pix_x : pix_y;
                scaled = gdk_pixbuf_scale_simple (pixbuf,
                                                  (width/ greatest) * pix_x,
                                                  (height/ greatest) * pix_y,
                                                   GDK_INTERP_BILINEAR);
                image = gtk_image_new_from_pixbuf (scaled);
                g_object_unref (G_OBJECT (scaled));
		return image;
	}
	
	return gtk_image_new_from_pixbuf (pixbuf);
}

/* Creates a pixmap appropriate for items.  */

static GtkWidget *
create_pixmap (GnomeUIPixmapType pixmap_type, gconstpointer pixmap_info,
	       GtkIconSize icon_size)
{
	GtkWidget *pixmap;
	char *name;

	pixmap = NULL;

	switch (pixmap_type) {
	case GNOME_APP_PIXMAP_STOCK:
		pixmap = gtk_image_new_from_stock (pixmap_info,
						   icon_size);
		break;

	case GNOME_APP_PIXMAP_DATA:
		if (pixmap_info != NULL) {
			GdkPixbuf *pixbuf = gdk_pixbuf_new_from_xpm_data
				((const char **)pixmap_info);
			if (pixbuf != NULL) {
				pixmap = scale_pixbuf (pixbuf, icon_size);
				g_object_unref (G_OBJECT (pixbuf));
			}
		}

		break;

	case GNOME_APP_PIXMAP_NONE:
		break;

	case GNOME_APP_PIXMAP_FILENAME:
		name = gnome_program_locate_file (gnome_program_get (),
						  GNOME_FILE_DOMAIN_PIXMAP,
						  pixmap_info, TRUE, NULL);

		if (!name)
			g_warning ("Could not find GNOME pixmap file %s",
					(char *) pixmap_info);
		else {
			GdkPixbuf *pixbuf;
			pixbuf = gdk_pixbuf_new_from_file (name, NULL);
			pixmap = scale_pixbuf (pixbuf, icon_size);
			g_object_unref (G_OBJECT (pixbuf));
			g_free (name);
		}

		break;

	default:
		g_assert_not_reached ();
		g_warning("Invalid pixmap_type %d", (int) pixmap_type);
	}

	return pixmap;
}

static void
showing_pixmaps_changed_notify(GConfClient            *client,
                               guint                   cnxn_id,
			       GConfEntry             *entry,
                               gpointer                user_data)
{
        gboolean new_setting = TRUE;
        GtkWidget *w = user_data;
        GtkImageMenuItem *mi = GTK_IMAGE_MENU_ITEM(w);
	GConfValue *value;

	value = gconf_entry_get_value (entry);

        if (value && value->type == GCONF_VALUE_BOOL) {
                new_setting = gconf_value_get_bool(value);
        }

	GDK_THREADS_ENTER();

        if (new_setting && (mi->image == NULL)) {
                GtkWidget *pixmap;
                GnomeUIPixmapType pixmap_type;
                gconstpointer pixmap_info;

                pixmap_type = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(mi),
								gnome_app_helper_pixmap_type));
                pixmap_info = g_object_get_data(G_OBJECT(mi),
						gnome_app_helper_pixmap_info);

                pixmap = create_pixmap (pixmap_type, pixmap_info,
					GTK_ICON_SIZE_MENU);

		if (pixmap != NULL) {
			gtk_widget_show(pixmap);

			gtk_image_menu_item_set_image
				(GTK_IMAGE_MENU_ITEM (mi), pixmap);
		}
        } else if (!new_setting && (mi->image != NULL)) {
		gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (mi), NULL);
        }

	GDK_THREADS_LEAVE();
}

/* Note that this function is also used for toolbars, don't assume
   obj is a menu item */
static void
remove_notify_cb(GtkObject *obj, gpointer data)
{
        guint notify_id;
        GConfClient *conf;

        notify_id = GPOINTER_TO_INT(data);

        conf = g_object_get_data (G_OBJECT (obj), gnome_app_helper_gconf_client);

        gconf_client_notify_remove(conf, notify_id);
}

static void
setup_image_menu_item (GtkWidget *mi, GnomeUIPixmapType pixmap_type,
		       gconstpointer pixmap_info)
{
        guint notify_id;
        GConfClient *conf;

        g_return_if_fail(GTK_IS_IMAGE_MENU_ITEM(mi));

        g_object_set_data(G_OBJECT(mi), gnome_app_helper_pixmap_type,
			  GINT_TO_POINTER(pixmap_type));

        g_object_set_data(G_OBJECT(mi), gnome_app_helper_pixmap_info,
			  (gpointer)pixmap_info);


	/* make sure that things are all ready for us */
	gnomeui_gconf_lazy_init ();

        conf = gconf_client_get_default();

        g_object_ref (G_OBJECT (conf));
        g_object_set_data_full(G_OBJECT(mi), gnome_app_helper_gconf_client,
			       conf, g_object_unref);

        if (gconf_client_get_bool(conf,
                                  "/desktop/gnome/interface/menus_have_icons",
                                  NULL)) {
                GtkWidget *pixmap;

                pixmap = create_pixmap (pixmap_type, pixmap_info,
					GTK_ICON_SIZE_MENU);

		if (pixmap != NULL) {
			gtk_widget_show (pixmap);
			gtk_image_menu_item_set_image
				(GTK_IMAGE_MENU_ITEM (mi), pixmap);
		}
        }

        notify_id = gconf_client_notify_add(conf,
                                            "/desktop/gnome/interface/menus_have_icons",
                                            showing_pixmaps_changed_notify,
                                            mi, NULL, NULL);

        g_signal_connect(mi, "destroy",
			 G_CALLBACK (remove_notify_cb),
			 GINT_TO_POINTER(notify_id));
}

/* Creates  a menu item label. */
static GtkWidget *
create_label (const char *label_text)
{
	GtkWidget *label;

	label = gtk_accel_label_new (label_text);
	gtk_label_set_use_underline (GTK_LABEL (label), TRUE);

	gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
	gtk_widget_show (label);

	return label;
}

/* Creates the accelerator for the specified uiinfo item's hotkey */
static void
setup_accelerator (GtkAccelGroup *accel_group, GnomeUIInfo *uiinfo,
		char *signal_name, int accel_flags)
{
	if (uiinfo->accelerator_key != 0)
		gtk_widget_add_accelerator (uiinfo->widget, signal_name,
					    accel_group, uiinfo->accelerator_key,
					    uiinfo->ac_mods, accel_flags);
}

/* Callback to display hint in the statusbar when a menu item is
 * activated. For GtkStatusbar.
 */

static void
put_hint_in_statusbar(GtkWidget* menuitem, gpointer data)
{
	gchar* hint = g_object_get_data(G_OBJECT(menuitem), apphelper_statusbar_hint);
	GtkWidget* bar = data;
	guint id;

	g_return_if_fail (hint != NULL);
	g_return_if_fail (bar != NULL);
	g_return_if_fail (GTK_IS_STATUSBAR(bar));

	id = gtk_statusbar_get_context_id(GTK_STATUSBAR(bar),
					  gnome_app_helper_menu_hint);

	gtk_statusbar_push(GTK_STATUSBAR(bar),id,hint);
}

/* Callback to remove hint when the menu item is deactivated.
 * For GtkStatusbar.
 */
static void
remove_hint_from_statusbar(GtkWidget* menuitem, gpointer data)
{
	GtkWidget* bar = data;
	guint id;

	g_return_if_fail (bar != NULL);
	g_return_if_fail (GTK_IS_STATUSBAR(bar));

	id = gtk_statusbar_get_context_id(GTK_STATUSBAR(bar),
					  gnome_app_helper_menu_hint);

	gtk_statusbar_pop(GTK_STATUSBAR(bar), id);
}

/* Install a hint for a menu item
 */
static void
install_menuitem_hint_to_statusbar(GnomeUIInfo* uiinfo, GtkStatusbar* bar)
{
  g_return_if_fail (uiinfo != NULL);
  g_return_if_fail (uiinfo->widget != NULL);
  g_return_if_fail (GTK_IS_MENU_ITEM(uiinfo->widget));

  /* This is mildly fragile; if someone destroys the appbar
     but not the menu, chaos will ensue. */

  if (uiinfo->hint)
    {
      g_object_set_data (G_OBJECT (uiinfo->widget),
			 apphelper_statusbar_hint,
			 (gpointer)(L_(uiinfo->hint)));

      g_signal_connect (G_OBJECT (uiinfo->widget),
			"select",
			G_CALLBACK(put_hint_in_statusbar),
			bar);

      g_signal_connect (G_OBJECT (uiinfo->widget),
			"deselect",
			G_CALLBACK(remove_hint_from_statusbar),
			bar);
    }
}

/**
 * gnome_app_install_statusbar_menu_hints
 * @bar: Pointer to #GtkStatusbar instance.
 * @uiinfo: #GnomeUIInfo for the menu to be changed.
 *
 * Description:
 * Install menu hints for the given status bar.
 */

void
gnome_app_install_statusbar_menu_hints (GtkStatusbar* bar,
                                        GnomeUIInfo* uiinfo)
{
	g_return_if_fail (bar != NULL);
	g_return_if_fail (uiinfo != NULL);
	g_return_if_fail (GTK_IS_STATUSBAR (bar));


	while (uiinfo->type != GNOME_APP_UI_ENDOFINFO)
	{
		switch (uiinfo->type) {
		case GNOME_APP_UI_INCLUDE:
			gnome_app_install_statusbar_menu_hints(bar, uiinfo->moreinfo);
			break;

		case GNOME_APP_UI_SUBTREE:
		case GNOME_APP_UI_SUBTREE_STOCK:
			gnome_app_install_statusbar_menu_hints(bar, uiinfo->moreinfo);
			break;
		case GNOME_APP_UI_ITEM:
		case GNOME_APP_UI_TOGGLEITEM:
		case GNOME_APP_UI_SEPARATOR:
		case GNOME_APP_UI_HELP:
			install_menuitem_hint_to_statusbar(uiinfo, bar);
			break;
		case GNOME_APP_UI_RADIOITEMS:
			gnome_app_install_statusbar_menu_hints(bar, uiinfo->moreinfo);
			break;
		default:
			;
			break;
		}

		++uiinfo;
	}
}


/* Callback to display hint in the statusbar when a menu item is
 * activated. For GnomeAppBar.
 */

static void
put_hint_in_appbar(GtkWidget* menuitem, gpointer data)
{
  gchar* hint = g_object_get_data (G_OBJECT(menuitem), "apphelper_appbar_hint");
  GtkWidget* bar = data;

  g_return_if_fail (hint != NULL);
  g_return_if_fail (bar != NULL);
  g_return_if_fail (GNOME_IS_APPBAR(bar));

  gnome_appbar_set_status (GNOME_APPBAR(bar), hint);
}

/* Callback to remove hint when the menu item is deactivated.
 * For GnomeAppBar.
 */
static void
remove_hint_from_appbar(GtkWidget* menuitem, gpointer data)
{
  GtkWidget* bar = data;

  g_return_if_fail (bar != NULL);
  g_return_if_fail (GNOME_IS_APPBAR(bar));

  gnome_appbar_refresh (GNOME_APPBAR(bar));
}

/* Install a hint for a menu item
 */
static void
install_menuitem_hint_to_appbar(GnomeUIInfo* uiinfo, GnomeAppBar* bar)
{
  g_return_if_fail (uiinfo != NULL);
  g_return_if_fail (uiinfo->widget != NULL);
  g_return_if_fail (GTK_IS_MENU_ITEM(uiinfo->widget));

  /* This is mildly fragile; if someone destroys the appbar
     but not the menu, chaos will ensue. */

  if (uiinfo->hint)
    {
      g_object_set_data (G_OBJECT(uiinfo->widget),
			 apphelper_appbar_hint,
			 (gpointer) L_(uiinfo->hint));

      g_signal_connect (uiinfo->widget,
			"select",
			G_CALLBACK(put_hint_in_appbar),
			bar);

      g_signal_connect (uiinfo->widget,
			"deselect",
			G_CALLBACK(remove_hint_from_appbar),
			bar);
    }
}


/**
 * gnome_app_ui_configure_configurable
 * @uiinfo: Pointer to GNOME UI menu/toolbar info
 *
 * Description:
 * Configure all user-configurable elements in the given UI info
 * structure.  This includes loading and setting previously-set options from
 * GNOME config files.
 *
 * Normally, gnome_app_create_menus() calls this function for the developer,
 * but if something needs to be altered afterwards, this function can be called
 * first. The main reason for this function being a public interface is so that
 * it can be called from gnome_popup_menu_new(), which clears a copy of the
 * pass in #GnomeUIInfo structures.
 */

void
gnome_app_ui_configure_configurable (GnomeUIInfo* uiinfo)
{
  if (uiinfo->type == GNOME_APP_UI_ITEM_CONFIGURABLE)
  {
        GnomeUIInfoConfigurableTypes type = (GnomeUIInfoConfigurableTypes) uiinfo->accelerator_key;

	gboolean accelerator_key_def;
	gchar *accelerator_key_string;
	gint accelerator_key;

	gboolean ac_mods_def;
	gchar *ac_mods_string;
	gint ac_mods;

	if ( type != GNOME_APP_CONFIGURABLE_ITEM_NEW ) {
#if 0
	        gboolean label_def;
		gchar *label_string;
		gchar *label;
	        gboolean hint_def;
		gchar *hint_string;
		gchar *hint;

		label_string = g_strdup_sprintf( "/Gnome/Menus/Menu-%s-label", menu_names[(int) type] );
		label = gnome_config_get_string_with_default( label_string, &label_def);
		if ( label_def )
		  uiinfo->label = label;
		else
		  {
#endif
		    uiinfo->label = menu_defaults[(int) type].label;
#if 0
		    g_free( label );
		  }
		g_free( label_string );

		hint_string = g_strdup_sprintf( "/Gnome/Menus/Menu-%s-hint", menu_names[(int) type] );
		hint = gnome_config_get_string_with_default( hint_string, &hint_def);
		if ( hint_def )
		  uiinfo->hint = hint;
		else
		  {
#endif
		    uiinfo->hint = menu_defaults[(int) type].hint;
#if 0
		    g_free( hint );
		  }
		g_free( hint_string );
#endif
	}
	uiinfo->pixmap_type = menu_defaults[(int) type].pixmap_type;
	uiinfo->pixmap_info = menu_defaults[(int) type].pixmap_info;

	accelerator_key_string = g_strdup_printf( "/Gnome/Menus/Menu-%s-accelerator-key", menu_names[(int) type] );
	accelerator_key = gnome_config_get_int_with_default( accelerator_key_string, &accelerator_key_def);
	if ( accelerator_key_def )
	  uiinfo->accelerator_key = menu_defaults[(int) type].accelerator_key;
	else
	  uiinfo->accelerator_key = accelerator_key;
	g_free( accelerator_key_string );

	ac_mods_string = g_strdup_printf( "/Gnome/Menus/Menu-%s-ac-mods", menu_names[(int) type] );
	ac_mods = gnome_config_get_int_with_default( ac_mods_string, &ac_mods_def);
	if ( ac_mods_def )
	  uiinfo->ac_mods = menu_defaults[(int) type].ac_mods;
	else
	  uiinfo->ac_mods = (GdkModifierType) ac_mods;
	g_free( ac_mods_string );


	uiinfo->type = GNOME_APP_UI_ITEM;
  }
}


/**
 * gnome_app_install_appbar_menu_hints
 * @appbar: An existing #GnomeAppBar instance.
 * @uiinfo: A #GnomeUIInfo array of a menu for which hints will be set.
 *
 * Description:
 * Install menu hints for the given @appbar object. This function cannot just
 * be called automatically, since it is impossible to reliably find the correct
 * @appbar.
 */

void
gnome_app_install_appbar_menu_hints (GnomeAppBar* appbar,
                                     GnomeUIInfo* uiinfo)
{
	g_return_if_fail (appbar != NULL);
	g_return_if_fail (uiinfo != NULL);
	g_return_if_fail (GNOME_IS_APPBAR (appbar));


	while (uiinfo->type != GNOME_APP_UI_ENDOFINFO)
	{

		/* Translate configurable menu items to normal menu items. */

		if ( uiinfo->type == GNOME_APP_UI_ITEM_CONFIGURABLE ) {
			gnome_app_ui_configure_configurable( uiinfo );
		}
		switch (uiinfo->type) {
		case GNOME_APP_UI_INCLUDE:
			gnome_app_install_appbar_menu_hints(appbar, uiinfo->moreinfo);
			break;

		case GNOME_APP_UI_SUBTREE:
		case GNOME_APP_UI_SUBTREE_STOCK:
			gnome_app_install_appbar_menu_hints(appbar, uiinfo->moreinfo);

		case GNOME_APP_UI_ITEM:
		case GNOME_APP_UI_TOGGLEITEM:
		case GNOME_APP_UI_SEPARATOR:
		case GNOME_APP_UI_HELP:
			install_menuitem_hint_to_appbar(uiinfo, appbar);
			break;
		case GNOME_APP_UI_RADIOITEMS:
			gnome_app_install_appbar_menu_hints(appbar, uiinfo->moreinfo);
			break;
		default:
			;
			break;
		}

		++uiinfo;
	}
}


/**
 * gnome_app_install_menu_hints
 * @app: An existing #GnomeAppBar instance.
 * @uiinfo: A #GnomeUIInfo array of a menu for which hints will be set.
 *
 * Description:
 * Set menu hints for the @app object's attached status bar.
 */

void
gnome_app_install_menu_hints (GnomeApp *app,
                              GnomeUIInfo *uiinfo) {
  g_return_if_fail (app != NULL);
  g_return_if_fail (uiinfo != NULL);
  g_return_if_fail (app->statusbar != NULL);
  g_return_if_fail (GNOME_IS_APP (app));

  if(GNOME_IS_APPBAR(app->statusbar))
    gnome_app_install_appbar_menu_hints(GNOME_APPBAR(app->statusbar), uiinfo);
  else if(GTK_IS_STATUSBAR(app->statusbar))
    gnome_app_install_statusbar_menu_hints(GTK_STATUSBAR(app->statusbar), uiinfo);
}

static gint
gnome_save_accels (gpointer data)
{
	gchar *file_name;

	file_name = g_build_filename (gnome_user_accels_dir_get (),
				      gnome_program_get_app_id (gnome_program_get()),
				      NULL);
	gtk_accel_map_save (file_name);
	g_free (file_name);

	return TRUE;
}

/**
 * gnome_accelerators_sync:
 *
 * Flush the accelerator definitions into the application specific
 * configuration file $HOME/.gnome2/accels/<app-id>.
 */
void
gnome_accelerators_sync (void)
{
  gnome_save_accels (NULL);
}

/* Creates a menu item appropriate for the SEPARATOR, ITEM, TOGGLEITEM, or
 * SUBTREE types.  If the item is inside a radio group, then a pointer to the
 * group's list must be specified as well (*radio_group must be NULL for the
 * first item!).  This function does *not* create the submenu of a subtree
 * menu item.
 */
static void
create_menu_item (GtkMenuShell       *menu_shell,
		  GnomeUIInfo        *uiinfo,
		  int                 is_radio,
		  GSList            **radio_group,
		  GnomeUIBuilderData *uibdata,
		  GtkAccelGroup      *accel_group,
		  gint		      pos)
{
	GtkWidget *label;
	int type;

	/* Translate configurable menu items to normal menu items. */

	if (uiinfo->type == GNOME_APP_UI_ITEM_CONFIGURABLE)
	        gnome_app_ui_configure_configurable( uiinfo );

	/* Create the menu item */

	switch (uiinfo->type) {
	case GNOME_APP_UI_SEPARATOR:
	        uiinfo->widget = gtk_separator_menu_item_new ();
		break;
	case GNOME_APP_UI_ITEM:
	case GNOME_APP_UI_SUBTREE:
	case GNOME_APP_UI_SUBTREE_STOCK:
		if (is_radio) {
			uiinfo->widget = gtk_radio_menu_item_new (*radio_group);
			*radio_group = gtk_radio_menu_item_get_group
				(GTK_RADIO_MENU_ITEM (uiinfo->widget));
		} else {

		        /* Create the pixmap */

			uiinfo->widget = gtk_image_menu_item_new ();

		        if (uiinfo->pixmap_type != GNOME_APP_PIXMAP_NONE) {
                                setup_image_menu_item (uiinfo->widget,
						       uiinfo->pixmap_type,
						       uiinfo->pixmap_info);
			}
		}
		break;

	case GNOME_APP_UI_TOGGLEITEM:
		uiinfo->widget = gtk_check_menu_item_new ();
		break;

	default:
		g_warning ("Invalid GnomeUIInfo type %d passed to "
				"create_menu_item()", (int) uiinfo->type);
		return;
	}

/*	if (!accel_group)
		gtk_widget_lock_accelerators (uiinfo->widget); */

	gtk_widget_show (uiinfo->widget);
	gtk_menu_shell_insert (menu_shell, uiinfo->widget, pos);

	/* If it is a separator, set it as insensitive so that it cannot be
	 * selected, and return -- there is nothing left to do.
	 */

	if (uiinfo->type == GNOME_APP_UI_SEPARATOR) {
		gtk_widget_set_sensitive (uiinfo->widget, FALSE);
		return;
	}

	/* Create the contents of the menu item */

	/* Don't use gettext on the empty string since gettext will map
	 * the empty string to the header at the beginning of the .pot file. */

	label = create_label ( uiinfo->label == NULL?
			       "":(uiinfo->type == GNOME_APP_UI_SUBTREE_STOCK ?
				   D_(uiinfo->label):L_(uiinfo->label)));

	gtk_container_add (GTK_CONTAINER (uiinfo->widget), label);

	gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label),
					  uiinfo->widget);

	/* install global accelerator
	 */
	{
		static guint save_accels_id = 0;
		GString *gstring;
		GtkWidget *widget;

		/* build up the menu item path */
		gstring = g_string_new (NULL);
		widget = uiinfo->widget;
		while (widget) {
			if (GTK_IS_MENU_ITEM (widget)) {
				GtkWidget *child = GTK_BIN (widget)->child;

				if (GTK_IS_LABEL (child)) {
					g_string_prepend (gstring, GTK_LABEL (child)->label);
					g_string_prepend_c (gstring, '/');
				}
				widget = widget->parent;
			} else if (GTK_IS_MENU (widget)) {
				widget = gtk_menu_get_attach_widget (GTK_MENU (widget));
				if (widget == NULL) {
					g_string_prepend (gstring, "/-Orphan");
					widget = NULL;
				}
			} else
				widget = widget->parent;
		}
		g_string_prepend_c (gstring, '>');
		g_string_prepend (gstring, gnome_program_get_app_id (gnome_program_get()));
		g_string_prepend_c (gstring, '<');

		/* g_print ("######## menu item path: %s\n", gstring->str); */

		/* associate the key combo with the accel path */
		if (uiinfo->accelerator_key)
			gtk_accel_map_add_entry (gstring->str,
						 uiinfo->accelerator_key,
						 uiinfo->ac_mods);

		/* associate the accel path with the menu item */
		if (GTK_IS_MENU (menu_shell))
			gtk_menu_item_set_accel_path(GTK_MENU_ITEM (uiinfo->widget),
						     gstring->str);

		g_string_free (gstring, TRUE);

		if (!save_accels_id)
			save_accels_id = gtk_quit_add (1, gnome_save_accels, NULL);
	}

	/* Set toggle information, if appropriate */

	if ((uiinfo->type == GNOME_APP_UI_TOGGLEITEM) || is_radio)
		gtk_check_menu_item_set_active
			(GTK_CHECK_MENU_ITEM (uiinfo->widget), FALSE);

	/* Connect to the signal and set user data */

	type = uiinfo->type;
	if (type == GNOME_APP_UI_SUBTREE_STOCK)
		type = GNOME_APP_UI_SUBTREE;

	if (type != GNOME_APP_UI_SUBTREE) {
	        g_object_set_data (G_OBJECT (uiinfo->widget),
				   GNOMEUIINFO_KEY_UIDATA,
				   uiinfo->user_data);

		g_object_set_data (G_OBJECT (uiinfo->widget),
				   GNOMEUIINFO_KEY_UIBDATA,
				   uibdata->data);

		(* uibdata->connect_func) (uiinfo, "activate", uibdata);
	}
}

/* Creates a group of radio menu items.  Returns the updated position parameter. */
static int
create_radio_menu_items (GtkMenuShell *menu_shell, GnomeUIInfo *uiinfo,
		GnomeUIBuilderData *uibdata, GtkAccelGroup *accel_group,
		gint pos)
{
	GSList *group;

	group = NULL;

	for (; uiinfo->type != GNOME_APP_UI_ENDOFINFO; uiinfo++)
		switch (uiinfo->type) {
		case GNOME_APP_UI_BUILDER_DATA:
			uibdata = uiinfo->moreinfo;
			break;

		case GNOME_APP_UI_ITEM:
			create_menu_item (menu_shell, uiinfo, TRUE,
					  &group, uibdata,
					  accel_group, pos);
			pos++;
			break;

		default:
			g_warning ("GnomeUIInfo element type %d is not valid "
					"inside a menu radio item group",
				   (int) uiinfo->type);
		}

	return pos;
}

static void
help_view_display_callback (GtkWidget *w, gpointer data)
{
	char *file_name = data;

	/* FIXME: handle errors somehow */
	gnome_help_display (file_name,
			    NULL,
			    NULL /* error */);
}

/* Creates the menu entries for help topics.  Returns the updated position
 * value. */
static int
create_help_entries (GtkMenuShell *menu_shell, GnomeUIInfo *uiinfo, gint pos)
{
	gchar *path;
	gchar *title = N_("_Contents");
		
	uiinfo->widget = gtk_image_menu_item_new_with_mnemonic (L_(title));
	uiinfo->hint = g_strdup (_("View help for this application"));

	setup_image_menu_item (uiinfo->widget, GNOME_APP_PIXMAP_STOCK, GTK_STOCK_HELP);

	g_signal_connect_data (uiinfo->widget, "activate",
			       G_CALLBACK (help_view_display_callback),
			       g_strdup (uiinfo->moreinfo), 
			       (GClosureNotify) g_free, 0);
	
	gtk_menu_shell_insert (menu_shell, uiinfo->widget, pos);

	/* Install global accelerator */

	path = g_strdup_printf("<%s>/help-contents-item", 
			       gnome_program_get_app_id (gnome_program_get()));
	
	/* Associate the key combo with the accel path */
	gtk_accel_map_add_entry (path, GDK_F1, 0);

	/* Associate the accel path with the menu item */
	if (GTK_IS_MENU (menu_shell))
		gtk_menu_item_set_accel_path (GTK_MENU_ITEM (uiinfo->widget),
					      path);

	g_free (path);
	
	gtk_widget_show_all (uiinfo->widget);

	return ++pos;
}

typedef struct
{
	GtkCallbackMarshal relay_func;
	GtkDestroyNotify destroy_func;
	gpointer user_data;
} SavedData;

static void
ui_relay_callback (GtkObject *object,
		   SavedData *saved_data)
{
	saved_data->relay_func (object, saved_data->user_data, 0, NULL);
}

static void
ui_destroy_callback (SavedData *saved_data)
{
	if (saved_data->user_data)
		saved_data->destroy_func (saved_data->user_data);
	g_free (saved_data);
}

/* Performs signal connection as appropriate for interpreters or native bindings */
static void
do_ui_signal_connect (GnomeUIInfo        *uiinfo,
		      const char         *signal_name,
		      GnomeUIBuilderData *uibdata)
{
	if (uibdata->is_interp) {
		SavedData *saved_data = g_new (SavedData, 1);
		saved_data->relay_func = uibdata->relay_func;
		saved_data->destroy_func = uibdata->destroy_func;
		saved_data->user_data = uibdata->data ? uibdata->data : uiinfo->user_data;

		g_signal_connect_data (uiinfo->widget, signal_name,
				       G_CALLBACK (ui_relay_callback),
				       saved_data,
				       (GClosureNotify) ui_destroy_callback,
				       0);
	} else if (uiinfo->moreinfo)
		g_signal_connect (uiinfo->widget,
				  signal_name,
				  G_CALLBACK (uiinfo->moreinfo),
				  uibdata->data ? uibdata->data : uiinfo->user_data);
}


/**
 * gnome_app_fill_menu
 * @menu_shell: A #GtkMenuShell instance (a menu bar).
 * @uiinfo: A pointer to the first element in an array of #GnomeUIInfo
 * structures. The last element of the array should have a type of
 * #GNOME_APP_UI_ENDOFINFO.
 * @accel_group: A #GtkAccelGroup.
 * @uline_accels: %TRUE if underline accelerators will be drawn for the menu
 * item labels.
 * @pos: The position in the menu bar at which to start inserting items.
 *
 * Description:
 * Fills the specified @menu_shell with items created from the specified
 * @uiinfo, inserting them from the item number @pos on.  The @accel_ group
 * will be used as the accel group for all newly created sub menus and serves
 * as the global accel group for all menu item hotkeys. If it is passed as
 * %NULL, global hotkeys will be disabled.
 **/

void
gnome_app_fill_menu (GtkMenuShell  *menu_shell,
		     GnomeUIInfo   *uiinfo,
		     GtkAccelGroup *accel_group,
		     gboolean       uline_accels,
		     gint           pos)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (menu_shell != NULL);
	g_return_if_fail (GTK_IS_MENU_SHELL (menu_shell));
	g_return_if_fail (uiinfo != NULL);
	g_return_if_fail (pos >= 0);

	uibdata.connect_func =  do_ui_signal_connect;
	uibdata.data = NULL;
	uibdata.is_interp = FALSE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	gnome_app_fill_menu_custom (menu_shell, uiinfo, &uibdata,
				    accel_group, uline_accels,
				    pos);
	return;
}


/**
 * gnome_app_fill_menu_with_data
 * @menu_shell: A #GtkMenuShell instance (a menu bar).
 * @uiinfo: A pointer to the first element in an array of #GnomeUIInfo
 * structures. The last element of the array should have a type of
 * #GNOME_APP_UI_ENDOFINFO.
 * @accel_group: A #GtkAccelGroup.
 * @uline_accels: %TRUE if underline accelerators will be drawn for the menu
 * item labels.
 * @pos: The position in the menu bar at which to start inserting items.
 * @user_data: Some application-specific data.
 *
 * Description:
 * This is the same as gnome_app_fill_menu(), except that all the user data
 * pointers are filled with the value of @user_data.
 **/

void
gnome_app_fill_menu_with_data (GtkMenuShell  *menu_shell,
			       GnomeUIInfo   *uiinfo,
			       GtkAccelGroup *accel_group,
			       gboolean       uline_accels,
			       gint	      pos,
			       gpointer       user_data)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (menu_shell != NULL);
	g_return_if_fail (GTK_IS_MENU_SHELL (menu_shell));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = user_data;
	uibdata.is_interp = FALSE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	gnome_app_fill_menu_custom (menu_shell, uiinfo, &uibdata,
				    accel_group, uline_accels,
				    pos);
}

static void
menus_have_tearoff_changed_notify(GConfClient            *client,
				  guint                   cnxn_id,
				  GConfEntry             *entry,
				  gpointer                user_data)
{
	GtkWidget *menu;
	
	if (entry->value->type != GCONF_VALUE_BOOL)
		return;

	GDK_THREADS_ENTER();

	menu = GTK_WIDGET (user_data);
	
	if (gconf_value_get_bool (entry->value)) {
		GtkWidget *tearoff;

		tearoff = g_object_get_data (G_OBJECT (menu), "gnome-app-tearoff");

		if (tearoff) {
			/* Do nothing */
			goto end;
		}
		
		/* Add the tearoff */
		tearoff = gtk_tearoff_menu_item_new ();
		gtk_widget_show (tearoff);
		g_object_set_data (G_OBJECT (menu), "gnome-app-tearoff", tearoff);
		gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), tearoff);
	}
	else {
		GtkWidget *tearoff;

		tearoff = g_object_get_data (G_OBJECT (user_data), "gnome-app-tearoff");

		if (!tearoff) {
			/* Do nothing */
			goto end;
		}
		
		/* Remove the tearoff */
		gtk_widget_destroy (tearoff);
		g_object_set_data (G_OBJECT (menu), "gnome-app-tearoff", NULL);
	}

 end:
	GDK_THREADS_LEAVE();
}


/**
 * gnome_app_fill_menu_custom
 * @menu_shell: A #GtkMenuShell instance (a menu bar).
 * @uiinfo: A pointer to the first element in an array of #GnomeUIInfo
 * structures. The last element of the array should have a type of
 * #GNOME_APP_UI_ENDOFINFO.
 * @uibdata: A #GnomeUIInfoBuilderData instance.
 * @accel_group: A #GtkAccelGroup.
 * @uline_accels: %TRUE if underline accelerators will be drawn for the menu
 * item labels.
 * @pos: The position in the menu bar at which to start inserting items.
 *
 * Description:
 * Fills the specified menu shell with items created from the specified
 * @uiinfo, inserting them from item number @pos on and using the specified
 * builder data (@uibdata) -- this is intended for language bindings. 
 *
 * The other parameters have the same meaning as in gnome_app_fill_menu().
 **/

void
gnome_app_fill_menu_custom (GtkMenuShell       *menu_shell,
			    GnomeUIInfo        *uiinfo,
			    GnomeUIBuilderData *uibdata,
			    GtkAccelGroup      *accel_group,
			    gboolean            uline_accels,
			    gint                pos)
{
	GnomeUIBuilderData *orig_uibdata;

	g_return_if_fail (menu_shell != NULL);
	g_return_if_fail (GTK_IS_MENU_SHELL (menu_shell));
	g_return_if_fail (uiinfo != NULL);
	g_return_if_fail (uibdata != NULL);
	g_return_if_fail (pos >= 0);

	/* Store a pointer to the original uibdata so that we can use it for
	 * the subtrees */

	orig_uibdata = uibdata;

	/* if it is a GtkMenu, make sure the accel group is associated
	 * with the menu */
	if (GTK_IS_MENU (menu_shell) &&
	    GTK_MENU (menu_shell)->accel_group == NULL)
			gtk_menu_set_accel_group (GTK_MENU (menu_shell),
						  accel_group);

	for (; uiinfo->type != GNOME_APP_UI_ENDOFINFO; uiinfo++)
		switch (uiinfo->type) {
		case GNOME_APP_UI_BUILDER_DATA:
			/* Set the builder data for subsequent entries in the
			 * current uiinfo array */
			uibdata = uiinfo->moreinfo;
			break;

		case GNOME_APP_UI_HELP:
			/* Create entries for the help topics */
			pos = create_help_entries (menu_shell, uiinfo, pos);
			break;

		case GNOME_APP_UI_RADIOITEMS:
			/* Create the radio item group */
			pos = create_radio_menu_items (menu_shell,
					uiinfo->moreinfo, uibdata, accel_group,
					pos);
			break;

		case GNOME_APP_UI_SEPARATOR:
		case GNOME_APP_UI_ITEM:
		case GNOME_APP_UI_ITEM_CONFIGURABLE:
		case GNOME_APP_UI_TOGGLEITEM:
		case GNOME_APP_UI_SUBTREE:
		case GNOME_APP_UI_SUBTREE_STOCK:
			if (uiinfo->type == GNOME_APP_UI_SUBTREE_STOCK)
				create_menu_item (menu_shell, uiinfo, FALSE,
						  NULL, uibdata,
						  accel_group, pos);
			else
				create_menu_item (menu_shell, uiinfo, FALSE,
						  NULL, uibdata,
						  accel_group, pos);

			if (uiinfo->type == GNOME_APP_UI_SUBTREE ||
			    uiinfo->type == GNOME_APP_UI_SUBTREE_STOCK) {
				/* Create the subtree for this item */

				GtkWidget *menu;
				GtkWidget *tearoff;
				guint notify_id;
				GConfClient *client;
				
				menu = gtk_menu_new ();
				gtk_menu_item_set_submenu
					(GTK_MENU_ITEM(uiinfo->widget), menu);
				gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);
				gnome_app_fill_menu_custom
					(GTK_MENU_SHELL (menu),
					 uiinfo->moreinfo, orig_uibdata,
					 accel_group, uline_accels, 0);

				if (gnome_gconf_get_bool ("/desktop/gnome/interface/menus_have_tearoff")) {
					tearoff = gtk_tearoff_menu_item_new ();
					gtk_widget_show (tearoff);
					g_object_set_data (G_OBJECT (menu), "gnome-app-tearoff", tearoff);
					gtk_menu_shell_prepend (GTK_MENU_SHELL (menu), tearoff);
				}

				client = gconf_client_get_default ();
				g_object_set_data_full(G_OBJECT(menu), gnome_app_helper_gconf_client,
						       client, g_object_unref);

				notify_id = gconf_client_notify_add (client,
								     "/desktop/gnome/interface/menus_have_tearoff",
								     menus_have_tearoff_changed_notify,
								     menu, NULL, NULL);
				g_signal_connect(menu, "destroy",
						 G_CALLBACK(remove_notify_cb),
						 GINT_TO_POINTER(notify_id));

			}
			pos++;
			break;

		case GNOME_APP_UI_INCLUDE:
		        gnome_app_fill_menu_custom
			  (menu_shell,
			   uiinfo->moreinfo, orig_uibdata,
			   accel_group, uline_accels, pos);
			break;

		default:
			g_warning ("Invalid GnomeUIInfo element type %d\n",
					(int) uiinfo->type);
		}

	/* Make the end item contain a pointer to the parent menu shell */

	uiinfo->widget = GTK_WIDGET (menu_shell);
}


/**
 * gnome_app_create_menus
 * @app: A #GnomeApp instance representing the current application.
 * @uiinfo: The first in an array #GnomeUIInfo instances containing the menu
 * data.
 *
 * Description:
 * Constructs a menu bar and attaches it to the specified application
 * window.
 **/

void
gnome_app_create_menus (GnomeApp *app, GnomeUIInfo *uiinfo)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = NULL;
	uibdata.is_interp = FALSE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	gnome_app_create_menus_custom (app, uiinfo, &uibdata);
}


/**
 * gnome_app_create_menus_interp
 * @app: A #GnomeApp instance, representing the current application.
 * @uiinfo: The first item in an array of #GnomeUIInfo structures describing
 * the menu bar.
 * @relay_func: A marshaller for the signal callbacks.
 * @data: Application specific data passed to the signal callback functions.
 * @destroy_func: The function to call when the menu bar is destroyed.
 *
 * Identical to gnome_app_create_menus(), except that extra functions and data
 * can be passed in for finer control of the destruction and marshalling.
 **/

void
gnome_app_create_menus_interp (GnomeApp *app, GnomeUIInfo *uiinfo,
		GtkCallbackMarshal relay_func, gpointer data,
		GtkDestroyNotify destroy_func)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = data;
	uibdata.is_interp = TRUE;
	uibdata.relay_func = relay_func;
	uibdata.destroy_func = destroy_func;

	gnome_app_create_menus_custom (app, uiinfo, &uibdata);
}


/**
 * gnome_app_create_menus_with_data
 * @app: A #GnomeApp instance representing the current application.
 * @uiinfo: The first in an array #GnomeUIInfo instances containing the menu
 * data.
 * @user_data: Application-specific data that is passed to every callback
 * function.
 *
 * Identical to gnome_app_create_menus(), except that @user_data is passed to
 * all the callback functions when signals are emitted.
 **/

void
gnome_app_create_menus_with_data (GnomeApp *app, GnomeUIInfo *uiinfo,
				  gpointer user_data)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = user_data;
	uibdata.is_interp = FALSE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	gnome_app_create_menus_custom (app, uiinfo, &uibdata);
}

static void
gnome_app_set_tearoff_menu_titles(GnomeApp *app, GnomeUIInfo *uiinfo,
				  char *above)
{
	int i;
	char *ctmp = NULL, *ctmp2;

	g_return_if_fail(above);

	for(i = 0; uiinfo[i].type != GNOME_APP_UI_ENDOFINFO; i++) {
		int type;

		type = uiinfo[i].type;

		if(type == GNOME_APP_UI_INCLUDE)
		  {
		    gnome_app_set_tearoff_menu_titles (app, uiinfo[i].moreinfo, above);
		    continue;
		  }

		if (type == GNOME_APP_UI_SUBTREE_STOCK)
			type = GNOME_APP_UI_SUBTREE;

		if(type != GNOME_APP_UI_SUBTREE
		   || !uiinfo[i].widget)
			continue;

		if(!ctmp)
			ctmp = g_alloca(strlen(above) + sizeof(" : ") + strlen(uiinfo[i].label)
					+ 75 /* eek! Hope noone uses huge menu item names! */);
		strcpy(ctmp, above);
		strcat(ctmp, " : ");
		strcat(ctmp, uiinfo[i].label);

		ctmp2 = ctmp;
		while((ctmp2 = strchr(ctmp2, '_')))
			g_memmove(ctmp2, ctmp2+1, strlen(ctmp2+1)+1);

		gtk_menu_set_title(GTK_MENU(GTK_MENU_ITEM(uiinfo[i].widget)->submenu), ctmp);

		gnome_app_set_tearoff_menu_titles(app, uiinfo[i].moreinfo, ctmp);
	}
}


/**
 * gnome_app_create_menus_custom
 * @app: A #GnomeApp instance representing the current application.
 * @uiinfo: The first in an array #GnomeUIInfo instances containing the menu
 * data.
 * @uibdata: An appropriate #GnomeUIBuilderData instance.
 *
 * Identical to gnome_app_create_menus(), except that @uibdata is also
 * specified for creating the signal handlers. Mostly for use by language
 * bindings.
 **/

void
gnome_app_create_menus_custom (GnomeApp *app, GnomeUIInfo *uiinfo,
			       GnomeUIBuilderData *uibdata)
{
	GtkWidget *menubar;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);
	g_return_if_fail (uibdata != NULL);

	menubar = gtk_menu_bar_new ();
	gnome_app_set_menus (app, GTK_MENU_BAR (menubar));
	gnome_app_fill_menu_custom (GTK_MENU_SHELL (menubar), uiinfo, uibdata,
				    app->accel_group, TRUE, 0);

	/* FIXME: make this runtime configurable */
	if (gnome_gconf_get_bool ("/desktop/gnome/interface/menus_have_tearoff")) {
		gchar *app_name;

		app_name = GTK_WINDOW (app)->title;
		if (!app_name)
			app_name = GNOME_APP (app)->name;
		gnome_app_set_tearoff_menu_titles (app, uiinfo, app_name);
	}
}

/* Creates a toolbar item appropriate for the SEPARATOR, ITEM, or TOGGLEITEM
 * types.  If the item is inside a radio group, then a pointer to the group's
 * trailing widget must be specified as well (*radio_group must be NULL for
 * the first item!).
 */
static void
create_toolbar_item (GtkToolbar *toolbar, GnomeUIInfo *uiinfo, int is_radio,
		GtkWidget **radio_group, GnomeUIBuilderData *uibdata,
		GtkAccelGroup *accel_group)
{
	GtkWidget *pixmap;
	GtkToolbarChildType type;

	switch (uiinfo->type) {
	case GNOME_APP_UI_SEPARATOR:
		gtk_toolbar_append_space (toolbar);
		uiinfo->widget = NULL; /* no meaningful widget for a space */
		break;

	case GNOME_APP_UI_ITEM:
	case GNOME_APP_UI_TOGGLEITEM:
		/* Create the icon */

		pixmap = create_pixmap (uiinfo->pixmap_type, uiinfo->pixmap_info,
					/* FIXME: what about small toolbar */
					GTK_ICON_SIZE_LARGE_TOOLBAR);

		/* Create the toolbar item */

		if (is_radio)
			type = GTK_TOOLBAR_CHILD_RADIOBUTTON;
		else if (uiinfo->type == GNOME_APP_UI_ITEM)
			type = GTK_TOOLBAR_CHILD_BUTTON;
		else
			type = GTK_TOOLBAR_CHILD_TOGGLEBUTTON;

		uiinfo->widget =
			gtk_toolbar_append_element (toolbar,
						    type,
						    is_radio ?
						    *radio_group : NULL,
						    L_(uiinfo->label),
						    L_(uiinfo->hint),
						    NULL,
						    pixmap,
						    NULL,
						    NULL);

		if (is_radio)
			*radio_group = uiinfo->widget;

		break;

	default:
		g_warning ("Invalid GnomeUIInfo type %d passed to "
			   "create_toolbar_item()", (int) uiinfo->type);
		return;
	}

	if (uiinfo->type == GNOME_APP_UI_SEPARATOR)
		return; /* nothing more to do */

	/* Set the accelerator and connect to the signal */

	if (is_radio) {
		setup_accelerator (accel_group, uiinfo, "toggled", 0);
		(* uibdata->connect_func) (uiinfo, "toggled", uibdata);
	} else {
		setup_accelerator (accel_group, uiinfo, "clicked", 0);
		(* uibdata->connect_func) (uiinfo, "clicked", uibdata);
	}
}

static void
create_radio_toolbar_items (GtkToolbar *toolbar, GnomeUIInfo *uiinfo,
		GnomeUIBuilderData *uibdata, GtkAccelGroup *accel_group)
{
	GtkWidget *group;
	gpointer orig_uibdata = uibdata;

	group = NULL;

	for (; uiinfo->type != GNOME_APP_UI_ENDOFINFO; uiinfo++)
		switch (uiinfo->type) {
		case GNOME_APP_UI_BUILDER_DATA:
			uibdata = uiinfo->moreinfo;
			break;

		case GNOME_APP_UI_ITEM:
			create_toolbar_item (toolbar, uiinfo, TRUE, &group,
					uibdata, accel_group);
			break;

		case GNOME_APP_UI_INCLUDE:
		        create_radio_toolbar_items (toolbar, uiinfo->moreinfo, orig_uibdata, accel_group);
		        break;

		default:
			g_warning ("GnomeUIInfo element type %d is not valid "
				   "inside a toolbar radio item group",
				   (int) uiinfo->type);
		}
}


/**
 * gnome_app_fill_toolbar
 * @toolbar: A #GtkToolbar instance.
 * @uiinfo: An array of #GnomeUIInfo structures containing the items for the
 * toolbar.
 * @accel_group: A #GtkAccelGroup for holding the accelerator keys of the items
 * (or %NULL).
 *
 * Fills @toolbar with buttons specified in @uiinfo. If @accel_group is not
 * %NULL, the items' accelrator keys are put into it.
 **/

void
gnome_app_fill_toolbar (GtkToolbar *toolbar, GnomeUIInfo *uiinfo,
		GtkAccelGroup *accel_group)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (GTK_IS_TOOLBAR (toolbar));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = NULL;
	uibdata.is_interp = FALSE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	gnome_app_fill_toolbar_custom (toolbar, uiinfo, &uibdata, accel_group);
}


/**
 * gnome_app_fill_toolbar_with_data
 * @toolbar: A #GtkToolbar instance.
 * @uiinfo: An array of #GnomeUIInfo structures containing the items for the
 * toolbar.
 * @accel_group: A #GtkAccelGroup for holding the accelerator keys of the items
 * (or %NULL).
 * @user_data: Application specific data.
 *
 * The same as gnome_app_fill_toolbar(), except that the user data pointers in
 * the signal handlers are set to @user_data.
 **/

void
gnome_app_fill_toolbar_with_data (GtkToolbar *toolbar, GnomeUIInfo *uiinfo,
				  GtkAccelGroup *accel_group, gpointer user_data)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (GTK_IS_TOOLBAR (toolbar));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = user_data;
	uibdata.is_interp = FALSE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	gnome_app_fill_toolbar_custom (toolbar, uiinfo, &uibdata, accel_group);
}


/**
 * gnome_app_fill_toolbar_custom
 * @toolbar: A #GtkToolbar instance.
 * @uiinfo: An array of #GnomeUIInfo structures containing the items for the
 * toolbar.
 * @uibdata: The #GnomeUIBuilderData data for the toolbar.
 * @accel_group: A #GtkAccelGroup for holding the accelerator keys of the items
 * (or %NULL).
 *
 * The same as gnome_app_fill_toolbar(), except that the sepcified @uibdata
 * instance is used. This is mostly for the benefit of language bindings.
 **/

void
gnome_app_fill_toolbar_custom (GtkToolbar *toolbar, GnomeUIInfo *uiinfo,
		GnomeUIBuilderData *uibdata, GtkAccelGroup *accel_group)
{
	g_return_if_fail (toolbar != NULL);
	g_return_if_fail (GTK_IS_TOOLBAR (toolbar));
	g_return_if_fail (uiinfo != NULL);
	g_return_if_fail (uibdata != NULL);

	for (; uiinfo->type != GNOME_APP_UI_ENDOFINFO; uiinfo++)
		switch (uiinfo->type) {
		case GNOME_APP_UI_INCLUDE:
		        gnome_app_fill_toolbar_custom (toolbar, uiinfo->moreinfo, uibdata, accel_group);
		        break;

		case GNOME_APP_UI_BUILDER_DATA:
			/* Set the builder data for subsequent entries in the
			 * current uiinfo array */
			uibdata = uiinfo->moreinfo;
			break;

		case GNOME_APP_UI_RADIOITEMS:
			/* Create the radio item group */
			create_radio_toolbar_items (toolbar, uiinfo->moreinfo,
					uibdata, accel_group);
			break;

		case GNOME_APP_UI_SEPARATOR:
		case GNOME_APP_UI_ITEM:
		case GNOME_APP_UI_TOGGLEITEM:
			create_toolbar_item (toolbar, uiinfo, FALSE, NULL,
					uibdata, accel_group);
			break;

		default:
			g_warning ("Invalid GnomeUIInfo element type %d\n",
					(int) uiinfo->type);
		}

	/* Make the end item contain a pointer to the parent toolbar */

	uiinfo->widget = GTK_WIDGET (toolbar);

        gnome_app_setup_toolbar(toolbar, NULL);
}

/**
 * gnome_app_create_toolbar
 * @app: A #GnomeApp instance.
 * @uiinfo: A #GnomeUIInfo array specifying the contents of the toolbar.
 *
 * Description:
 * Constructs a toolbar and attaches it to the specified application
 * window.
 **/

void
gnome_app_create_toolbar (GnomeApp *app, GnomeUIInfo *uiinfo)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = NULL;
	uibdata.is_interp = FALSE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	gnome_app_create_toolbar_custom (app, uiinfo, &uibdata);
}

/**
 * gnome_app_create_toolbar_interp
 * @app: A #GnomeApp instance.
 * @uiinfo: A #GnomeUIInfo array specifying the contents of the toolbar.
 * @relay_func: Argument marshalling function.
 * @data: Application specific data to pass to signal callbacks.
 * @destroy_func: The function to call when the toolbar is destroyed.
 *
 * Description:
 * Constructs a toolbar and attaches it to the specified application
 * window -- this version is intended for language bindings.
 **/

void
gnome_app_create_toolbar_interp (GnomeApp *app, GnomeUIInfo *uiinfo,
				 GtkCallbackMarshal relay_func, gpointer data,
				 GtkDestroyNotify destroy_func)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = data;
	uibdata.is_interp = TRUE;
	uibdata.relay_func = relay_func;
	uibdata.destroy_func = destroy_func;

	gnome_app_create_toolbar_custom (app, uiinfo, &uibdata);
}

/**
 * gnome_app_create_toolbar_with_data
 * @app: A #GnomeApp instance.
 * @uiinfo: A #GnomeUIInfo array specifying the contents of the toolbar.
 * @user_data: Application specific data to be sent to each signal callback
 * function.
 *
 * Description:
 * Constructs a toolbar, sets all the user data pointers to
 * @user_data, and attaches it to @app.
 **/

void
gnome_app_create_toolbar_with_data (GnomeApp *app, GnomeUIInfo *uiinfo,
		gpointer user_data)
{
	GnomeUIBuilderData uibdata;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);

	uibdata.connect_func = do_ui_signal_connect;
	uibdata.data = user_data;
	uibdata.is_interp = FALSE;
	uibdata.relay_func = NULL;
	uibdata.destroy_func = NULL;

	gnome_app_create_toolbar_custom (app, uiinfo, &uibdata);
}

/**
 * gnome_app_create_toolbar_custom
 * @app: A #GnomeApp instance.
 * @uiinfo: A #GnomeUIInfo array specifying the contents of the toolbar.
 * @uibdata: A #GnomeUIBuilderData instance specifying the handlers to use for
 * the toolbar.
 *
 * Description:
 * Constructs a toolbar and attaches it to the @app window,
 * using @uibdata builder data -- intended for language bindings.
 **/

void
gnome_app_create_toolbar_custom (GnomeApp *app, GnomeUIInfo *uiinfo, GnomeUIBuilderData *uibdata)
{
	GtkWidget *toolbar;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));
	g_return_if_fail (uiinfo != NULL);
	g_return_if_fail (uibdata != NULL);

	toolbar = gtk_toolbar_new ();
	gtk_toolbar_set_orientation (GTK_TOOLBAR (toolbar),
				     GTK_ORIENTATION_HORIZONTAL);
	/* FIXME: isn't this set by some of that gtk+2->gconf magic goo? */
	gtk_toolbar_set_style (GTK_TOOLBAR (toolbar),
			       GTK_TOOLBAR_BOTH);
	gnome_app_fill_toolbar_custom(GTK_TOOLBAR (toolbar), uiinfo, uibdata,
			app->accel_group);
	gnome_app_set_toolbar (app, GTK_TOOLBAR (toolbar));
}

/**
 * g_strncmp_ignore_char
 * @first: The first string to compare
 * @second: The second string to compare
 * @length: The length of the first string to consider (the length of
 *          which string you're considering matters because this
 *          includes copies of the ignored character)
 * @ignored: The character to ignore
 *
 * Description:
 * Does a strcmp, only considering the first length characters of
 * first, and ignoring the ignored character in both strings.
 * Returns -1 if first comes before second in lexicographical order,
 * Returns 0 if they're equivalent, and
 * Returns 1 if the second comes before the first in lexicographical order.
 **/

static gint
g_strncmp_ignore_char( const gchar *first, const gchar *second, gint length, gchar ignored )
{
        gint i, j;
	for ( i = 0, j = 0; i < length; i++, j++ )
	{
                while ( first[i] == ignored && i < length ) i++;
		while ( second[j] == ignored ) j++;
		if ( i == length )
		        return 0;
		if ( first[i] < second[j] )
		        return -1;
		if ( first[i] > second[j] )
		        return 1;
	}
	return 0;
}

/* menu insertion/removal functions
 * <jaka.mocnik@kiss.uni-lj.si>
 *
 */

/**
 * gnome_app_find_menu_pos
 * @parent: Root menu shell widget containing menu items to be searched.
 * @path: Specifies the target menu item by menu path.
 * @pos: Used to hold the returned menu items' position.
 *
 * Description:
 * Finds a menu item described by path starting in the #GtkMenuShell top and
 * returns its parent #GtkMenuShell and the position after this item in @pos.
 * The meaning of @pos is that a subsequent call to gtk_menu_shell_insert(p, w,
 * pos) would then insert widget w in GtkMenuShell p right after the menu item
 * described by path.
 *
 * Returns: The parent menu shell of @path.
 **/

GtkWidget *
gnome_app_find_menu_pos (GtkWidget *parent, const gchar *path, gint *pos)
{
	GtkBin *item;
	gchar *label = NULL;
	GList *children;
	gchar *name_end;
	gchar *part;
	const gchar *transl;
	gint p;
	int  path_len;

	g_return_val_if_fail (parent != NULL, NULL);
	g_return_val_if_fail (path != NULL, NULL);
	g_return_val_if_fail (pos != NULL, NULL);

	children = GTK_MENU_SHELL (parent)->children;

	name_end = strchr(path, '/');
	if(name_end == NULL)
		path_len = strlen(path);
	else
		path_len = name_end - path;

	if (path_len == 0) {

	        if (children && GTK_IS_TEAROFF_MENU_ITEM(children->data))
		        /* consider the position after the tear off item as the topmost one. */
			*pos = 1;
		else
			*pos = 0;
		return parent;
	}

	/* this ugly thing should fix the localization problems */
	part = g_malloc(path_len + 1);
	if(!part)
	        return NULL;
	strncpy(part, path, path_len);
	part[path_len] = '\0';
	transl = L_(part);
	path_len = strlen(transl);

	p = 0;

	while (children){
		item = GTK_BIN (children->data);
		children = children->next;
		label = NULL;
		p++;

		if (GTK_IS_TEAROFF_MENU_ITEM(item))
			label = NULL;
		else if (!item->child)          /* this is a separator, right? */
			label = "<Separator>";
		else if (GTK_IS_LABEL (item->child))  /* a simple item with a label */
			label = GTK_LABEL (item->child)->label;
		else
			label = NULL; /* something that we just can't handle */
		if (label && (g_strncmp_ignore_char (transl, label, path_len, '_') == 0)){
			if (name_end == NULL) {
				*pos = p;
				g_free(part);
				return parent;
			}
			else if (GTK_MENU_ITEM (item)->submenu) {
			        g_free(part);
				return gnome_app_find_menu_pos
					(GTK_MENU_ITEM (item)->submenu,
					 (gchar *)(name_end + 1), pos);
			}
			else {
			        g_free(part);
				return NULL;
			}
		}
	}

	g_free(part);
	return NULL;
}


/**
 * gnome_app_remove_menus
 * @app: A #GnomeApp instance.
 * @path: A path to the menu item concerned.
 * @items: The number of items to remove.
 *
 * Description: Removes @items items from the existing @app's menu structure,
 * beginning with item described by @path.
 *
 * The @path argument should be in the form "File/.../.../Something". "" will
 * insert the item as the first one in the menubar, "File/" will insert it as
 * the first one in the File menu, "File/Settings" will insert it after the
 * Setting item in the File menu use of "File/<Separator>" should be obvious.
 * However, the use of "<Seperator>" stops after the first separator.
 **/

void
gnome_app_remove_menus(GnomeApp *app, const gchar *path, gint items)
{
	GtkWidget *parent, *child;
	GList *children;
	gint pos;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP(app));

	/* find the first item (which is actually at position pos-1) to
	 * remove */
	parent = gnome_app_find_menu_pos(app->menubar, path, &pos);

	/* in case of path ".../" remove the first item */
  if(path[strlen(path) - 1] == '/')
    pos++;

	if( parent == NULL ) {
		g_warning("gnome_app_remove_menus: couldn't find first item to"
			  " remove!");
		return;
	}

	/* remove items */
	children = g_list_nth(GTK_MENU_SHELL(parent)->children, pos - 1);
	while(children && items > 0) {
		child = GTK_WIDGET(children->data);
		children = children->next;
    /* if this item contains a gtkaccellabel, we have to set its
       accel_widget to NULL so that the item gets unrefed. */
    if(GTK_IS_ACCEL_LABEL(GTK_BIN(child)->child))
      gtk_accel_label_set_accel_widget(GTK_ACCEL_LABEL(GTK_BIN(child)->child), NULL);

		gtk_container_remove(GTK_CONTAINER(parent), child);
		items--;
	}

	gtk_widget_queue_resize(parent);
}


/**
 * gnome_app_remove_menu_range
 * @app: A #GnomeApp instance.
 * @path: A path to the menu item concerned.
 * @start: An offset beyond the start of @path at which to begin removing.
 * @items: The number of items to remove.
 *
 * Description:
 * Same as the gnome_app_remove_menus(), except it removes the specified number
 * of @items from the existing @app's menu structure begining with item
 * described by (@path plus @start). This is very useful for adding and
 * removing Recent document items in the File menu.
 **/

void
gnome_app_remove_menu_range (GnomeApp *app, const gchar *path, gint start, gint items)
{
	GtkWidget *parent, *child;
	GList *children;
	gint pos;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP (app));

	/* find the first item (which is actually at position pos-1) to remove */
	parent = gnome_app_find_menu_pos (app->menubar, path, &pos);

	/* in case of path ".../" remove the first item */
	if (path [strlen (path) - 1] == '/')
		pos++;

	pos += start;

	if (parent == NULL){
		g_warning("gnome_app_remove_menus: couldn't find first item to remove!");
		return;
	}

	/* remove items */
	children = g_list_nth (GTK_MENU_SHELL (parent)->children, pos - 1);
	while (children && items > 0) {
		child = GTK_WIDGET (children->data);
		children = children->next;
		/*
		 * if this item contains a gtkaccellabel, we have to set its
		 * accel_widget to NULL so that the item gets unrefed.
		 */
		if (GTK_IS_ACCEL_LABEL (GTK_BIN (child)->child))
			gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (GTK_BIN (child)->child), NULL);
		gtk_container_remove (GTK_CONTAINER (parent), child);
		items--;
	}

	gtk_widget_queue_resize(parent);
}

/**
 * gnome_app_insert_menus_custom
 * @app: A #GnomeApp instance.
 * @path: A path to the menu item concerned.
 * @uiinfo: A #GnomeUIInfo array describing the menus.
 * @uibdata: A #GnomeUIBuilderData instance describing the functions to attach
 * as the menu's callbacks.
 *
 * Description: Inserts menus described by @uiinfo in existing @app's menu
 * structure right after the item described by @path. The @uibdata parameter
 * makes this, again, most useful for language bindings.
 **/

void
gnome_app_insert_menus_custom (GnomeApp *app, const gchar *path,
		GnomeUIInfo *uiinfo, GnomeUIBuilderData *uibdata)
{
	GtkWidget *parent;
	gint pos;

	g_return_if_fail (app != NULL);
	g_return_if_fail (GNOME_IS_APP(app));
	g_return_if_fail (app->menubar != NULL);

	/* find the parent menushell and position for insertion of menus */
	parent = gnome_app_find_menu_pos(app->menubar, path, &pos);
	if(parent == NULL) {
		g_warning("gnome_app_insert_menus_custom: couldn't find "
			  "insertion point for menus!");
		return;
	}

	/* create menus and insert them */
	gnome_app_fill_menu_custom (GTK_MENU_SHELL (parent), uiinfo, uibdata,
			app->accel_group, TRUE, pos);
}


/**
 * gnome_app_insert_menus
 * @app: A #GnomeApp instance.
 * @path: A path to the menu item concerned.
 * @menuinfo: A #GnomeUIInfo array describing the menus.
 *
 * Insert the menus given by @menuinfo beginning at @path into the pre-existing
 * @app.
 **/

void
gnome_app_insert_menus (GnomeApp *app,
			const gchar *path,
			GnomeUIInfo *menuinfo)
{
	GnomeUIBuilderData uidata =
	{
		do_ui_signal_connect,
		NULL, FALSE, NULL, NULL
	};

	gnome_app_insert_menus_custom (app, path, menuinfo, &uidata);
}


/**
 * gnome_app_insert_menus_with_data
 * @app: A #GnomeApp instance.
 * @path: A path to the menu item concerned.
 * @menuinfo: A #GnomeUIInfo array describing the menus.
 * @data: Application specific data to send to each signal callback.
 *
 * This is the same as gnome_app_insert_menus(), except that the specified
 * @data is passed to each signal callback.
 **/

void
gnome_app_insert_menus_with_data (GnomeApp *app, const gchar *path,
		GnomeUIInfo *menuinfo, gpointer data)
{
	GnomeUIBuilderData uidata =
	{
		do_ui_signal_connect,
		NULL, FALSE, NULL, NULL
	};

	uidata.data = data;

	gnome_app_insert_menus_custom (app, path, menuinfo, &uidata);
}


/**
 * gnome_app_insert_menus_interp
 * @app: A #GnomeApp instance.
 * @path: A path to the menu item concerned.
 * @menuinfo: A #GnomeUIInfo array describing the menus.
 * @relay_func: A custom marshallar for signal data.,
 * @data: Application-specific data to send to each signal callback.
 * @destroy_func: The function to call when the menu item is destroyed.
 *
 * THe same as gnome_app_insert_menus(), except that the given functions are
 * attached to each menu item. Mostly of use for language bindings.
 **/

void
gnome_app_insert_menus_interp (GnomeApp *app, const gchar *path,
		GnomeUIInfo *menuinfo, GtkCallbackMarshal relay_func,
		gpointer data, GtkDestroyNotify destroy_func)
{
	GnomeUIBuilderData uidata =
	{
		do_ui_signal_connect,
		NULL, FALSE, NULL, NULL
	};

	uidata.data = data;
	uidata.is_interp = TRUE;
	uidata.relay_func = relay_func;
	uidata.destroy_func = destroy_func;

	gnome_app_insert_menus_custom(app, path, menuinfo, &uidata);
}

const gchar *
gnome_app_helper_gettext (const gchar *str)
{
	char *s;

        s = gettext (str);
	if ( s == str )
	        s = dgettext (GETTEXT_PACKAGE, str);

	return s;
}

static GConfEnumStringPair toolbar_styles[] = {
        { GTK_TOOLBAR_TEXT, "text" },
        { GTK_TOOLBAR_ICONS, "icons" },
        { GTK_TOOLBAR_BOTH, "both" },
        { GTK_TOOLBAR_BOTH_HORIZ, "both_horiz" },
	{ GTK_TOOLBAR_BOTH_HORIZ, "both-horiz" },
	{ -1, NULL }
};

static void
per_app_toolbar_style_changed_notify(GConfClient            *client,
                                     guint                   cnxn_id,
				     GConfEntry             *entry,
                                     gpointer                user_data)
{
        GtkToolbarStyle style = GTK_TOOLBAR_BOTH;
        GtkWidget *w = user_data;
        GtkToolbar *toolbar = GTK_TOOLBAR(w);
        gboolean got_it = FALSE;
	GConfValue *value = gconf_entry_get_value (entry);

        if (value &&
            value->type == GCONF_VALUE_STRING &&
            gconf_value_get_string(value) != NULL) {

                if (gconf_string_to_enum(toolbar_styles,
                                         gconf_value_get_string(value),
                                         (gint*)&style)) {
                        got_it = TRUE;
                }
        }

        /* Fall back to global setting */
        if (!got_it) {
                gchar *str;

                str = gconf_client_get_string(client,
                                              "/desktop/gnome/interface/toolbar_style",
                                              NULL);

                if (str) {
			if (!gconf_string_to_enum (toolbar_styles,
						   str,
						   (gint *)&style))
				style = GTK_TOOLBAR_BOTH;

                        g_free(str);
                }
        }

	GDK_THREADS_ENTER();
        gtk_toolbar_set_style(toolbar, style);
	GDK_THREADS_LEAVE();
}

static void
toolbar_style_changed_notify(GConfClient            *client,
                             guint                   cnxn_id,
			     GConfEntry             *entry,
                             gpointer                user_data)
{
        GtkToolbarStyle style = GTK_TOOLBAR_BOTH;
        GtkWidget *w = user_data;
        GtkToolbar *toolbar = GTK_TOOLBAR(w);
        gchar *per_app_key;
        gchar *str;
        gboolean got_it = FALSE;
	GConfValue *value;

        /* Check for app-specific override */
        per_app_key = gnome_gconf_get_gnome_libs_settings_relative("toolbar_style");
        str = gconf_client_get_string(client, per_app_key, NULL);
        g_free(per_app_key);

        if (str) {
                if (gconf_string_to_enum(toolbar_styles,
                                         str,
                                         (gint*)&style)) {
                        got_it = TRUE;
                }
                g_free(str);
        }

	value = gconf_entry_get_value (entry);

        /* If no per-app setting use this new global setting */
        if (!got_it &&
            value &&
            value->type == GCONF_VALUE_STRING &&
            gconf_value_get_string(value) != NULL) {
		if (!gconf_string_to_enum (toolbar_styles,
					   gconf_value_get_string (value),
					   (gint *)&style))
			style = GTK_TOOLBAR_BOTH;
        }

	GDK_THREADS_ENTER();
        gtk_toolbar_set_style(toolbar, style);
	GDK_THREADS_LEAVE();
}

static void
style_menu_item_activated (GtkWidget *item, GtkToolbarStyle style)
{
	GConfClient *conf;
	char *key;
	int i;

	key = gnome_gconf_get_gnome_libs_settings_relative("toolbar_style");
	conf = gconf_client_get_default();

	/* Set our per-app toolbar setting */
	for (i = 0; i < G_N_ELEMENTS (toolbar_styles); i++) {
		if (toolbar_styles[i].enum_value == style) {
			gconf_client_set_string (conf, key, toolbar_styles[i].str, NULL);
			break;
		}
	}
	g_free (key);
}

static void
global_menu_item_activated (GtkWidget *item)
{
	GConfClient *conf;
	char *key;

	key = gnome_gconf_get_gnome_libs_settings_relative("toolbar_style");
	conf = gconf_client_get_default();

	/* Unset the per-app toolbar setting */
	gconf_client_unset (conf, key, NULL);
	g_free (key);

}

/* Welcome to Hack City, population: gnome-app-helper */
/* Hack into the hack for the AIX pre-compiler. It barfs when stuff gets defined twice */
#ifdef _
#undef _
#endif
#define _(x) dgettext (GETTEXT_PACKAGE, x)

static void
create_and_popup_toolbar_menu (GdkEventButton *event)
{
	GtkWidget *menu;
	GtkWidget *item, *both_item, *both_horiz_item, *icons_item, *text_item, *global_item;
	GSList *group;
	char *both, *both_horiz, *icons, *text, *global;
	char *str, *key;
	GtkToolbarStyle toolbar_style;
	GConfClient *conf;

	group = NULL;
	toolbar_style = GTK_TOOLBAR_BOTH;
	menu = gtk_menu_new ();

	both = _("Text Below Icons");
	both_horiz = _("Priority Text Beside Icons");
	icons = _("Icons Only");
	text = _("Text Only");

	both_item = gtk_radio_menu_item_new_with_label (group, both);
	g_signal_connect (both_item, "activate",
			  G_CALLBACK (style_menu_item_activated), GINT_TO_POINTER (GTK_TOOLBAR_BOTH));
	group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (both_item));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), both_item);

	both_horiz_item = gtk_radio_menu_item_new_with_label (group, both_horiz);
	g_signal_connect (both_horiz_item, "activate",
			  G_CALLBACK (style_menu_item_activated), GINT_TO_POINTER (GTK_TOOLBAR_BOTH_HORIZ));
	group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (both_horiz_item));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), both_horiz_item);

	icons_item = gtk_radio_menu_item_new_with_label (group, icons);
	g_signal_connect (icons_item, "activate",
			  G_CALLBACK (style_menu_item_activated), GINT_TO_POINTER (GTK_TOOLBAR_ICONS));
	group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (icons_item));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), icons_item);

	text_item = gtk_radio_menu_item_new_with_label (group, text);
	g_signal_connect (text_item, "activate",
			  G_CALLBACK (style_menu_item_activated), GINT_TO_POINTER (GTK_TOOLBAR_TEXT));

	group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (text_item));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), text_item);

	item = gtk_separator_menu_item_new ();
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), item);

	/* Get global setting */
	conf = gconf_client_get_default();
	str = gconf_client_get_string(conf,
				      "/desktop/gnome/interface/toolbar_style",
				      NULL);

	if (str != NULL) {
		if (!gconf_string_to_enum (toolbar_styles,
					   str,
					   (gint*)&toolbar_style))
			toolbar_style = GTK_TOOLBAR_BOTH;
		g_free(str);
	}

	switch (toolbar_style) {
	case GTK_TOOLBAR_BOTH:
		str = both;
		break;
	case GTK_TOOLBAR_BOTH_HORIZ:
		str = both_horiz;
		break;
	case GTK_TOOLBAR_ICONS:
		str = icons;
		break;
	case GTK_TOOLBAR_TEXT:
		str = text;
		break;
	default:
		g_assert_not_reached ();
	}

	global = g_strdup_printf (_("Use Desktop Default (%s)"),
				  str);
	global_item = gtk_radio_menu_item_new_with_label (group, global);
	g_signal_connect (global_item, "activate",
			  G_CALLBACK (global_menu_item_activated), NULL);
	group = gtk_radio_menu_item_get_group (GTK_RADIO_MENU_ITEM (global_item));
	g_free (global);
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), global_item);

	gtk_widget_show_all (menu);

	/* Now select the correct menu according to our preferences */
	key = gnome_gconf_get_gnome_libs_settings_relative("toolbar_style");
	str = gconf_client_get_string(conf, key, NULL);

	if (str == NULL) {
		/* We have no per-app setting, so the global one must be right. */
		gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (global_item), TRUE);
	}
	else {
		if (!gconf_string_to_enum(toolbar_styles,
					  str,
					  (gint*)&toolbar_style))
			toolbar_style = GTK_TOOLBAR_BOTH;

		/* We have a per-app setting, find out which one it is */
		switch (toolbar_style) {
		case GTK_TOOLBAR_BOTH:
			gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (both_item), TRUE);
			break;
		case GTK_TOOLBAR_BOTH_HORIZ:
			gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (both_horiz_item), TRUE);
			break;
		case GTK_TOOLBAR_ICONS:
			gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (icons_item), TRUE);
			break;
		case GTK_TOOLBAR_TEXT:
			gtk_check_menu_item_set_active (GTK_CHECK_MENU_ITEM (text_item), TRUE);
			break;
		default:
			g_assert_not_reached ();
		}

		g_free (str);
	}

	g_free (key);
	gtk_menu_popup (GTK_MENU (menu), NULL, NULL, NULL, NULL, event->button, event->time);

}

/* Now leaving Hack City */
#ifdef _
#undef _
#endif
#define _(x) gettext (x)

static int
button_press (GtkWidget *dock, GdkEventButton *event, gpointer user_data)
{
	if (event->button == 3) {
		create_and_popup_toolbar_menu (event);
	}

	return FALSE;
}

/**
 * gnome_app_setup_toolbar
 * @toolbar: Pointer to a #GtkToolbar instance.
 * @dock_item: Pointer to the #BonoboDockItem the toolbar is inside, or %NULL
 * for none.
 *
 * Description:
 * Sets up a toolbar to use GNOME user preferences.
 **/

void
gnome_app_setup_toolbar (GtkToolbar *toolbar,
                         BonoboDockItem *dock_item)
{
        GConfClient *conf;

	/* make sure that things are all ready for us */
	gnomeui_gconf_lazy_init ();


        conf = gconf_client_get_default();

        g_object_ref (G_OBJECT (conf));
        g_object_set_data_full(G_OBJECT(toolbar), gnome_app_helper_gconf_client,
			       conf, (GDestroyNotify)g_object_unref);

        /* Attach GConf settings */

        if (dock_item != NULL) { /* Dock item bevel */
		g_object_ref (G_OBJECT (conf));
		g_object_set_data_full(G_OBJECT(dock_item), gnome_app_helper_gconf_client,
				       conf, (GDestroyNotify)g_object_unref);

		g_signal_connect (dock_item,
				  "button_press_event",
				  G_CALLBACK (button_press),
				  NULL);

        }

        { /* Toolbar Style */
                /* This one is a lot more complex because
                   we have a per-app setting that overrides
                   the default global setting */

                GtkToolbarStyle toolbar_style = GTK_TOOLBAR_BOTH;
                guint notify_id;
                gchar *str;
                gchar *per_app_key;
                gboolean got_it = FALSE;

                /* Try per-app key */
                per_app_key = gnome_gconf_get_gnome_libs_settings_relative("toolbar_style");

                str = gconf_client_get_string(conf,
                                              per_app_key,
                                              NULL);

                if (str &&
                    gconf_string_to_enum(toolbar_styles,
                                         str,
                                         (gint*)&toolbar_style)) {
                        got_it = TRUE;
                }

                g_free(str);

                /* Try global default */
                if (!got_it) {
                        str = gconf_client_get_string(conf,
                                                      "/desktop/gnome/interface/toolbar_style",
                                                      NULL);

                        if (str != NULL) {
                                if (!gconf_string_to_enum(toolbar_styles,
							  str,
							  (gint*)&toolbar_style))
					toolbar_style = GTK_TOOLBAR_BOTH;
                                g_free(str);
                        }
                }

                notify_id = gconf_client_notify_add(conf,
                                                    "/desktop/gnome/interface/toolbar_style",
                                                    toolbar_style_changed_notify,
                                                    toolbar, NULL, NULL);

                g_signal_connect(toolbar, "destroy",
				 G_CALLBACK(remove_notify_cb),
				 GINT_TO_POINTER(notify_id));

                notify_id = gconf_client_notify_add(conf,
                                                    per_app_key,
                                                    per_app_toolbar_style_changed_notify,
                                                    toolbar, NULL, NULL);

                g_signal_connect(toolbar, "destroy",
				 G_CALLBACK(remove_notify_cb),
				 GINT_TO_POINTER(notify_id));

                g_free(per_app_key);

                gtk_toolbar_set_style (toolbar, toolbar_style);
        }
}

