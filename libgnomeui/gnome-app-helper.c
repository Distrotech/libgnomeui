/* GnomeApp widget (C) 1998 Red Hat Software, Miguel de Icaza, Federico Mena, 
 * Chris Toshok.
 *
 * Originally by Elliot Lee, with hacking by Chris Toshok for *_data, Marc 
 * Ewing added menu support, toggle and radio support, and I don't know what 
 * you other people did :) menu insertion/removal functions by Jaka Mocnik.
 * Small fixes and documentation by Justin Maurer.
 *
 * Major cleanups and rearrangements by Federico Mena and Justin Maurer.
 */

/* TO-DO list for GnomeAppHelper and friends:
 *
 * - Find out how to disable on-the-fly hotkey changing for menu items 
 * (GtkAccelGroup locking?).
 *
 * - Write a custom container for the GnomeApp window so that you can have 
 * multiple toolbars and such.
 *
 * - Fix GtkHandleBox so that it works right (i.e. is easy to re-dock) and so 
 * that it allows dragging into the top/bottom/left/right of the application 
 * window as well.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

/* AIX requires this to be the first thing in the file.  */
#ifndef __GNUC__
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca /* predefined by HP cc +Olibcalls */
char *alloca ();
#   endif
#  endif
# endif
#endif

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include "libgnome/libgnome.h"
#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-config.h"
#include "libgnome/gnome-help.h"

/* Note that this file must include gnome-i18n, and not gnome-i18nP, so that 
 * _() is the same as the one seen by the application.  This is moderately 
 * bogus; we should just call gettext() directly here.
 */

#include "libgnome/gnome-i18n.h"
#include "gnome-app.h"
#include "gnome-app-helper.h"
#include "gnome-uidefs.h"
#include "gnome-stock.h"
#include "gnome-pixmap.h"
#include "gnome-preferences.h"
#include "gnome-stock.h"
#include "gtkpixmapmenuitem.h"

/* prototypes */
static gint g_strncmp_ignore_char( gchar *first, gchar *second,
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
	
	N_("File"),
	N_("File/"),
	N_("Edit"),
	N_("Edit/"),
	N_("View"),
	N_("View/"),
	N_("Settings"),
	N_("Settings/"),
        N_("New"),
	N_("New/"),
	N_("Files"),
	N_("Files/"),
	N_("Windows"),
	N_("Game"), 
	N_("Help"), 
	N_("Windows/")
};

#endif

static GnomeUIInfo menu_defaults[] = {
        /* New */
        { GNOME_APP_UI_ITEM, NULL, NULL,
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_NEW,
          GNOME_KEY_NAME_NEW, GNOME_KEY_MOD_NEW, NULL },
        /* Open */
        { GNOME_APP_UI_ITEM, N_("_Open..."), N_("Open a file"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_OPEN,
          GNOME_KEY_NAME_OPEN, GNOME_KEY_MOD_OPEN, NULL },
	/* Save */
        { GNOME_APP_UI_ITEM, N_("_Save"), N_("Save the current file"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE,
          GNOME_KEY_NAME_SAVE, GNOME_KEY_MOD_SAVE, NULL },
	/* Save As */
        { GNOME_APP_UI_ITEM, N_("Save _As..."),
          N_("Save the current file with a different name"),
          NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE_AS,
          GNOME_KEY_NAME_SAVE_AS, GNOME_KEY_MOD_SAVE_AS, NULL },
	/* Revert */
        { GNOME_APP_UI_ITEM, N_("_Revert"),
          N_("Revert to a saved version of the file"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REVERT,
          0,  (GdkModifierType) 0, NULL },
	/* Print */
        { GNOME_APP_UI_ITEM, N_("_Print"), N_("Print the current file"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PRINT,
          GNOME_KEY_NAME_PRINT,  GNOME_KEY_MOD_PRINT, NULL },
	/* Print Setup */
        { GNOME_APP_UI_ITEM, N_("Print S_etup..."),
          N_("Setup the page settings for your current printer"),
          NULL, NULL,
	  NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PRINT,
          GNOME_KEY_NAME_PRINT_SETUP,  GNOME_KEY_MOD_PRINT_SETUP, NULL },
	/* Close */
        { GNOME_APP_UI_ITEM, N_("_Close"), N_("Close the current file"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CLOSE,
          GNOME_KEY_NAME_CLOSE, GNOME_KEY_MOD_CLOSE, NULL },
	/* Exit */
        { GNOME_APP_UI_ITEM, N_("E_xit"), N_("Exit the program"),
          NULL, NULL, NULL, GNOME_APP_PIXMAP_STOCK,
	  GNOME_STOCK_MENU_EXIT, GNOME_KEY_NAME_EXIT, GNOME_KEY_MOD_EXIT,
	    NULL },
	/*
	 * The "Edit" menu
	 */
	/* Cut */
        { GNOME_APP_UI_ITEM, N_("C_ut"), N_("Cut the selection"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_CUT,
          GNOME_KEY_NAME_CUT, GNOME_KEY_MOD_CUT, NULL },
	/* 10 */
	/* Copy */
        { GNOME_APP_UI_ITEM, N_("_Copy"), N_("Copy the selection"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_COPY,
          GNOME_KEY_NAME_COPY, GNOME_KEY_MOD_COPY, NULL },
	/* Paste */
        { GNOME_APP_UI_ITEM, N_("_Paste"), N_("Paste the clipboard"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PASTE,
          GNOME_KEY_NAME_PASTE, GNOME_KEY_MOD_PASTE, NULL },
	/* Clear */
        { GNOME_APP_UI_ITEM, N_("C_lear"), N_("Clear the selection"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
          GNOME_KEY_NAME_CLEAR, GNOME_KEY_MOD_CLEAR, NULL },
	/* Undo */
        { GNOME_APP_UI_ITEM, N_("_Undo"), N_("Undo the last action"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_UNDO,
          GNOME_KEY_NAME_UNDO, GNOME_KEY_MOD_UNDO, NULL },
	/* Redo */
        { GNOME_APP_UI_ITEM, N_("_Redo"), N_("Redo the undone action"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REDO,
          GNOME_KEY_NAME_REDO, GNOME_KEY_MOD_REDO, NULL },
	/* Find */
        { GNOME_APP_UI_ITEM, N_("_Find..."),  N_("Search for a string"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH,
          GNOME_KEY_NAME_FIND, GNOME_KEY_MOD_FIND, NULL },
	/* Find Again */
        { GNOME_APP_UI_ITEM, N_("Find _Again"),
          N_("Search again for the same string"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SEARCH,
          GNOME_KEY_NAME_FIND_AGAIN, GNOME_KEY_MOD_FIND_AGAIN, NULL },
	/* Replace */
        { GNOME_APP_UI_ITEM, N_("_Replace..."), N_("Replace a string"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SRCHRPL,
          GNOME_KEY_NAME_REPLACE, GNOME_KEY_MOD_REPLACE, NULL },
	/* Properties */
        { GNOME_APP_UI_ITEM, N_("_Properties..."),
          N_("Modify the file's properties"),
          NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PROP,
          0,  (GdkModifierType) 0, NULL },
	/*
	 * The Settings menu
	 */
	/* Settings */
        { GNOME_APP_UI_ITEM, N_("_Preferences..."),
          N_("Configure the application"),
          NULL, NULL,
	  NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PREF,
          0,  (GdkModifierType) 0, NULL },
	/* 20 */
	/*
	 * And the "Help" menu
	 */
	/* About */
        { GNOME_APP_UI_ITEM, N_("_About..."),
          N_("About this application"), NULL, NULL,
	  NULL, GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT,
          0,  (GdkModifierType) 0, NULL },
	{ GNOME_APP_UI_ITEM, N_("_Select All"),
          N_("Select everything"),
          NULL, NULL, NULL,
          GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
          GNOME_KEY_NAME_SELECT_ALL, GNOME_KEY_MOD_SELECT_ALL, NULL },

	/*
	 * Window menu
	 */
        { GNOME_APP_UI_ITEM, N_("Create New _Window"),
          N_("Create a new window"),
          NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
          GNOME_KEY_NAME_NEW_WINDOW, GNOME_KEY_MOD_NEW_WINDOW, NULL },
        { GNOME_APP_UI_ITEM, N_("_Close This Window"),
          N_("Close the current window"),
          NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
          GNOME_KEY_NAME_CLOSE_WINDOW, GNOME_KEY_MOD_CLOSE_WINDOW, NULL },

	/*
	 * The "Game" menu
	 */
        { GNOME_APP_UI_ITEM, N_("_New game"),
          N_("Start a new game"),
	  NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
          GNOME_KEY_NAME_NEW_GAME,  GNOME_KEY_MOD_NEW_GAME, NULL },
        { GNOME_APP_UI_ITEM, N_("_Pause game"),
          N_("Pause the game"), 
	  NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_TIMER_STOP,
          GNOME_KEY_NAME_PAUSE_GAME,  GNOME_KEY_MOD_PAUSE_GAME, NULL },
        { GNOME_APP_UI_ITEM, N_("_Restart game"),
          N_("Restart the game"),
	  NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
          0,  0, NULL },
        { GNOME_APP_UI_ITEM, N_("_Undo move"),
          N_("Undo the last move"),
	  NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_UNDO,
          GNOME_KEY_NAME_UNDO_MOVE,  GNOME_KEY_MOD_UNDO_MOVE, NULL },
        { GNOME_APP_UI_ITEM, N_("_Redo move"),
          N_("Redo the undone move"),
	  NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_REDO,
          GNOME_KEY_NAME_REDO_MOVE,  GNOME_KEY_MOD_REDO_MOVE, NULL },
        { GNOME_APP_UI_ITEM, N_("_Hint"),
          N_("Get a hint for your next move"),
	  NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
          0,  0, NULL },
	/* 30 */
        { GNOME_APP_UI_ITEM, N_("_Scores..."),
          N_("View the scores"),
	  NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SCORES,
          0,  (GdkModifierType) 0, NULL },
        { GNOME_APP_UI_ITEM, N_("_End game"),
          N_("End the current game"),
	  NULL, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_BLANK,
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
  "exit",
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


/* Creates a pixmap appropriate for items.  The window parameter is required 
 * by gnome-stock (bleah) */

static GtkWidget *
create_pixmap (GtkWidget *window, GnomeUIPixmapType pixmap_type, 
		gconstpointer pixmap_info)
{
	GtkWidget *pixmap;
	char *name;

	pixmap = NULL;

	switch (pixmap_type) {
	case GNOME_APP_PIXMAP_STOCK:
		pixmap = gnome_stock_pixmap_widget (window, pixmap_info);
		break;

	case GNOME_APP_PIXMAP_DATA:
		if (pixmap_info)
			pixmap = gnome_pixmap_new_from_xpm_d ((char**)pixmap_info);

		break;

	case GNOME_APP_PIXMAP_NONE:
		break;

	case GNOME_APP_PIXMAP_FILENAME:
		name = gnome_pixmap_file (pixmap_info);

		if (!name)
			g_warning ("Could not find GNOME pixmap file %s", 
					(char *) pixmap_info);
		else {
			pixmap = gnome_pixmap_new_from_file (name);
			g_free (name);
		}

		break;

	default:
		g_assert_not_reached ();
		g_warning("Invalid pixmap_type %d", (int) pixmap_type); 
	}

	return pixmap;
}

/* Creates  a menu item label. It will also return the underlined 
 * letter's keyval if keyval is not NULL. */
static GtkWidget *
create_label (char *label_text, guint *keyval)
{
	guint kv;
	GtkWidget *label;

	label = gtk_accel_label_new (label_text);

	kv = gtk_label_parse_uline (GTK_LABEL (label), label_text);
	if (keyval)
		*keyval = kv;

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

#ifndef	GTK_CHECK_VERSION
GtkAccelGroup*
gtk_menu_ensure_uline_accel_group (GtkMenu *menu)
{
  GtkAccelGroup *accel_group;

  g_return_val_if_fail (GTK_IS_MENU (menu), NULL);

  accel_group = gtk_object_get_data (GTK_OBJECT (menu), "GtkMenu-uline-accel-group");
  if (!accel_group)
    {
      accel_group = gtk_accel_group_new ();
      gtk_accel_group_attach (accel_group, GTK_OBJECT (menu));
      gtk_object_set_data_full (GTK_OBJECT (menu),
				"GtkMenu-uline-accel-group",
				accel_group,
				(GtkDestroyNotify) gtk_accel_group_unref);
    }

  return accel_group;
}
void
gtk_item_factory_add_foreign (GtkWidget      *accel_widget,
			      const gchar    *full_path,
			      GtkAccelGroup  *accel_group,
			      guint           keyval,
			      GdkModifierType modifiers)
{
  if (accel_group)
    gtk_widget_add_accelerator (accel_widget,
				"activate",
				accel_group,
				keyval,
				modifiers,
				GTK_ACCEL_VISIBLE);
}
#endif

/* Creates the accelerators for the underlined letter in a menu item's label. 
 * The keyval is what gtk_label_parse_uline() returned.  If accel_group is not 
 * NULL, then the keyval will be put with MOD1 as modifier in it (i.e. for 
 * Alt-F in the _File menu).
 */
static void
setup_uline_accel (GtkMenuShell  *menu_shell,
		   GtkAccelGroup *accel_group,
		   GtkWidget     *menu_item,
		   guint          keyval)
{
	if (keyval != GDK_VoidSymbol) {
		if (GTK_IS_MENU (menu_shell))
			gtk_widget_add_accelerator (menu_item,
						    "activate_item",
						    gtk_menu_ensure_uline_accel_group (GTK_MENU (menu_shell)),
						    keyval, 0,
						    0);
		if (GTK_IS_MENU_BAR (menu_shell) && accel_group)
			gtk_widget_add_accelerator (menu_item,
						    "activate_item", 
						    accel_group,
						    keyval, GDK_MOD1_MASK,
						    0);
	}
}

/* Callback to display hint in the statusbar when a menu item is 
 * activated. For GtkStatusbar.
 */

static void
put_hint_in_statusbar(GtkWidget* menuitem, gpointer data)
{
	gchar* hint = gtk_object_get_data(GTK_OBJECT(menuitem),
					  "apphelper_statusbar_hint");
	GtkWidget* bar = data;
	guint id;
	
	g_return_if_fail (hint != NULL);
	g_return_if_fail (bar != NULL);
	g_return_if_fail (GTK_IS_STATUSBAR(bar));
	
	id = gtk_statusbar_get_context_id(GTK_STATUSBAR(bar),
					  "gnome-app-helper:menu-hint");
	
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
					  "gnome-app-helper:menu-hint");
	
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
      gtk_object_set_data (GTK_OBJECT(uiinfo->widget),
                           "apphelper_statusbar_hint",
                           L_(uiinfo->hint));

      gtk_signal_connect (GTK_OBJECT (uiinfo->widget),
                          "select",
                          GTK_SIGNAL_FUNC(put_hint_in_statusbar),
                          bar);
      
      gtk_signal_connect (GTK_OBJECT (uiinfo->widget),
                          "deselect",
                          GTK_SIGNAL_FUNC(remove_hint_from_statusbar),
                          bar);
    }
}


/**
 * gnome_app_install_statusbar_menu_hints
 * @bar: Pointer to Gtk+ status bar object
 * @uiinfo: Gnome UI info for the menu to be changed
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
		case GNOME_APP_UI_SUBTREE:
		case GNOME_APP_UI_SUBTREE_STOCK:
			gnome_app_install_statusbar_menu_hints(bar, uiinfo->moreinfo);
			
		case GNOME_APP_UI_ITEM:
		case GNOME_APP_UI_TOGGLEITEM:
		case GNOME_APP_UI_SEPARATOR:
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
  gchar* hint = gtk_object_get_data (GTK_OBJECT(menuitem),
                                     "apphelper_appbar_hint");
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
      gtk_object_set_data (GTK_OBJECT(uiinfo->widget),
                           "apphelper_appbar_hint",
                           L_(uiinfo->hint));

      gtk_signal_connect (GTK_OBJECT (uiinfo->widget),
                          "select",
                          GTK_SIGNAL_FUNC(put_hint_in_appbar),
                          bar);
      
      gtk_signal_connect (GTK_OBJECT (uiinfo->widget),
                          "deselect",
                          GTK_SIGNAL_FUNC(remove_hint_from_appbar),
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
 * @appbar: Pointer to GNOME app bar object.
 * @uiinfo: GNOME UI info for menu
 *
 * Description:
 * Install menu hints for the given GNOME app bar object.
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
		case GNOME_APP_UI_SUBTREE:
		case GNOME_APP_UI_SUBTREE_STOCK:
			gnome_app_install_appbar_menu_hints(appbar, uiinfo->moreinfo);
			
		case GNOME_APP_UI_ITEM:
		case GNOME_APP_UI_TOGGLEITEM:
		case GNOME_APP_UI_SEPARATOR:
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
 * @app: Pointer to GNOME app object
 * @uiinfo: GNOME UI menu for which hints will be set
 *
 * Description:
 * Set menu hints for the GNOME app object's attached status bar.
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

	file_name = g_concat_dir_and_file (gnome_user_accels_dir, gnome_app_id);
	gtk_item_factory_dump_rc (file_name, NULL, TRUE);
	g_free (file_name);

	return TRUE;
}

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
		  gboolean	      uline_accels,
		  gint		      pos)
{
	GtkWidget *label;
	GtkWidget *pixmap;
	guint keyval;
	int type;
	
	/* Translate configurable menu items to normal menu items. */

	if (uiinfo->type == GNOME_APP_UI_ITEM_CONFIGURABLE)
	        gnome_app_ui_configure_configurable( uiinfo );

	/* Create the menu item */

	switch (uiinfo->type) {
	case GNOME_APP_UI_SEPARATOR:
	        uiinfo->widget = gtk_menu_item_new ();
		break;
	case GNOME_APP_UI_ITEM:
	case GNOME_APP_UI_SUBTREE:
	case GNOME_APP_UI_SUBTREE_STOCK:
		if (is_radio) {
			uiinfo->widget = gtk_radio_menu_item_new (*radio_group);
			*radio_group = gtk_radio_menu_item_group
				(GTK_RADIO_MENU_ITEM (uiinfo->widget));
		} else {

		        /* Create the pixmap */

		        /* FIXME: this should later allow for on-the-fly configuration of 
			 * whether pixmaps are displayed or not ???
			 */

		        if ((uiinfo->pixmap_type != GNOME_APP_PIXMAP_NONE) &&
			    gnome_preferences_get_menus_have_icons()) {
			        uiinfo->widget = gtk_pixmap_menu_item_new ();
				pixmap = create_pixmap (uiinfo->widget, uiinfo->pixmap_type, 
							uiinfo->pixmap_info);
				if (pixmap) {
				        gtk_widget_show(pixmap);
					gtk_pixmap_menu_item_set_pixmap(GTK_PIXMAP_MENU_ITEM(uiinfo->widget),
									pixmap);
				}
			} else
			        uiinfo->widget = gtk_menu_item_new ();
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

	if (!accel_group)
		gtk_widget_lock_accelerators (uiinfo->widget);

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

	label = create_label ( uiinfo->label [0] == '\0'?
			       "":(uiinfo->type == GNOME_APP_UI_SUBTREE_STOCK ?
				   D_(uiinfo->label):L_(uiinfo->label)),
			       &keyval);

	gtk_container_add (GTK_CONTAINER (uiinfo->widget), label);

	gtk_accel_label_set_accel_widget (GTK_ACCEL_LABEL (label), 
					  uiinfo->widget);
	
	/* setup underline accelerators
	 */
	if (uline_accels)
		setup_uline_accel (menu_shell,
				   accel_group,
				   uiinfo->widget,
				   keyval);

	/* install global accelerator
	 */
	{
		static guint save_accels_id = 0;
		GString *gstring;
		GtkWidget *widget;
		
		/* build up the menu item path */
		gstring = g_string_new ("");
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
		g_string_prepend (gstring, gnome_app_id);
		g_string_prepend_c (gstring, '<');

		/* g_print ("######## menu item path: %s\n", gstring->str); */
		
		/* the item factory cares about installing the correct accelerator */
		
		gtk_item_factory_add_foreign (uiinfo->widget,
					      gstring->str,
					      accel_group,
					      uiinfo->accelerator_key,
					      uiinfo->ac_mods);
		g_string_free (gstring, TRUE);

		if (!save_accels_id)
			save_accels_id = gtk_quit_add (1, gnome_save_accels, NULL);
	}
	
	/* Set toggle information, if appropriate */
	
	if ((uiinfo->type == GNOME_APP_UI_TOGGLEITEM) || is_radio) {
		gtk_check_menu_item_set_show_toggle
			(GTK_CHECK_MENU_ITEM(uiinfo->widget), TRUE);
		gtk_check_menu_item_set_active
			(GTK_CHECK_MENU_ITEM (uiinfo->widget), FALSE);
	}
	
	/* Connect to the signal and set user data */
	
	type = uiinfo->type;
	if (type == GNOME_APP_UI_SUBTREE_STOCK)
		type = GNOME_APP_UI_SUBTREE;
	
	if (type != GNOME_APP_UI_SUBTREE) {
	        gtk_object_set_data (GTK_OBJECT (uiinfo->widget),
				     GNOMEUIINFO_KEY_UIDATA,
				     uiinfo->user_data);

		gtk_object_set_data (GTK_OBJECT (uiinfo->widget),
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
			create_menu_item (menu_shell, uiinfo, TRUE, &group, uibdata, 
					  accel_group, FALSE, pos);
			pos++;
			break;

		default:
			g_warning ("GnomeUIInfo element type %d is not valid "
					"inside a menu radio item group",
				   (int) uiinfo->type);
		}

	return pos;
}

/* Frees a help menu entry when its corresponding menu item is destroyed */
static void
free_help_menu_entry (GtkWidget *widget, gpointer data)
{
	GnomeHelpMenuEntry *entry;

	entry = data;

	g_free (entry->name);
	g_free (entry->path);
	g_free (entry);
}

/* Creates the menu entries for help topics.  Returns the updated position 
 * value. */
static int
create_help_entries (GtkMenuShell *menu_shell, GnomeUIInfo *uiinfo, gint pos)
{
	char buf[1024];
	char *topic_file;
	char *s;
	FILE *file;
	GnomeHelpMenuEntry *entry;
	GtkWidget *item;
	GtkWidget *label;
	guint keyval;

	if (!uiinfo->moreinfo) {
		g_warning ("GnomeUIInfo->moreinfo cannot be NULL for "
				"GNOME_APP_UI_HELP");
		return pos;
	}

	/* Try to open help topics file */

	topic_file = gnome_help_file_find_file (uiinfo->moreinfo, "topic.dat");

	if (!topic_file || !(file = fopen (topic_file, "rt"))) {
		g_warning ("Could not open help topics file %s", 
				topic_file ? topic_file : "NULL");

		if (topic_file)
			g_free (topic_file);

		return pos;
	}
	g_free (topic_file);
	
	/* Read in the help topics and create menu items for them */

	while (fgets (buf, sizeof (buf), file)) {
		/* Format of lines is "help_file_name whitespace* menu_title" */

		for (s = buf; *s && !isspace (*s); s++)
			;

		*s++ = '\0';

		for (; *s && isspace (*s); s++)
			;

		if (s[strlen (s) - 1] == '\n')
			s[strlen (s) - 1] = '\0';

		/* Create help menu entry */

		entry = g_new (GnomeHelpMenuEntry, 1);
		entry->name = g_strdup (uiinfo->moreinfo);
		entry->path = g_strdup (buf);

		item = gtk_menu_item_new ();
		label = create_label (s, &keyval);
		gtk_container_add (GTK_CONTAINER (item), label);
		setup_uline_accel (menu_shell, NULL, item, keyval);
		gtk_widget_lock_accelerators (item);
		
		gtk_signal_connect (GTK_OBJECT (item), "activate",
				    (GtkSignalFunc) gnome_help_display, entry);
		gtk_signal_connect (GTK_OBJECT (item), "destroy",
				    (GtkSignalFunc) free_help_menu_entry, 
				    entry);

		gtk_menu_shell_insert (menu_shell, item, pos);
		pos++;

		gtk_widget_show (item);
	}

	fclose (file);
	uiinfo->widget = NULL; /* No relevant widget, as we may have created 
				  several of them */

	return pos;
}

/* Performs signal connection as appropriate for interpreters or native bindings */
static void
do_ui_signal_connect (GnomeUIInfo *uiinfo, gchar *signal_name, 
		GnomeUIBuilderData *uibdata)
{
	if (uibdata->is_interp)
		gtk_signal_connect_full (GTK_OBJECT (uiinfo->widget), 
				signal_name, NULL, uibdata->relay_func, 
				uibdata->data ? 
				uibdata->data : uiinfo->user_data,
				uibdata->destroy_func, FALSE, FALSE);
	
	else if (uiinfo->moreinfo)
		gtk_signal_connect (GTK_OBJECT (uiinfo->widget), 
				signal_name, uiinfo->moreinfo, uibdata->data ? 
				uibdata->data : uiinfo->user_data);
}


/**
 * gnome_app_fill_menu
 * @menu_shell:
 * @uiinfo:
 * @accel_group:
 * @uline_accels:
 * @pos:
 *
 * Description:
 * Fills the specified menu shell with items created from the specified
 * info, inserting them from the item no. pos on.
 * The accel group will be used as the accel group for all newly created
 * sub menus and serves as the global accel group for all menu item
 * hotkeys. If it is passed as NULL, global hotkeys will be disabled.
 * The uline_accels argument determines whether underline accelerators
 * will be featured from the menu item labels.
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
	
	uibdata.connect_func = do_ui_signal_connect;
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
 * @menu_shell:
 * @uiinfo:
 * @accel_group:
 * @uline_accels:
 * @pos:
 * @user_data:
 *
 * Description:
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


/**
 * gnome_app_fill_menu_custom
 * @menu_shell:
 * @uiinfo:
 * @uibdata:
 * @accel_group:
 * @uline_accels:
 * @pos:
 *
 * Description:
 * Fills the specified menu shell with items created from the specified
 * info, inserting them from item no. pos on and using the specified
 * builder data -- this is intended for language bindings.
 * The accel group will be used as the accel group for all newly created
 * sub menus and serves as the global accel group for all menu item
 * hotkeys. If it is passed as NULL, global hotkeys will be disabled.
 * The uline_accels argument determines whether underline accelerators
 * will be featured from the menu item labels.
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
				create_menu_item (menu_shell, uiinfo, FALSE, NULL, uibdata, 
						  accel_group, uline_accels,
						  pos);
			else
				create_menu_item (menu_shell, uiinfo, FALSE, NULL, uibdata, 
						  accel_group, uline_accels,
						  pos);
			
			if (uiinfo->type == GNOME_APP_UI_SUBTREE ||
			    uiinfo->type == GNOME_APP_UI_SUBTREE_STOCK) {
				/* Create the subtree for this item */

				GtkWidget *menu;
				GtkWidget *tearoff;

				menu = gtk_menu_new ();
				gtk_menu_item_set_submenu
					(GTK_MENU_ITEM(uiinfo->widget), menu);
				gtk_menu_set_accel_group (GTK_MENU (menu), accel_group);
				gnome_app_fill_menu_custom
					(GTK_MENU_SHELL (menu), 
					 uiinfo->moreinfo, orig_uibdata, 
					 accel_group, uline_accels, 0);
				if (gnome_preferences_get_menus_have_tearoff ()) {
					tearoff = gtk_tearoff_menu_item_new ();
					gtk_widget_show (tearoff);
					gtk_menu_prepend (GTK_MENU (menu), tearoff);
				}
			}
			pos++;
			break;

		default:
			g_warning ("Invalid GnomeUIInfo element type %d\n", 
					(int) uiinfo->type);
		}

	/* Make the end item contain a pointer to the parent menu shell */

	uiinfo->widget = GTK_WIDGET (menu_shell);

	/* Configure menu to gnome preferences, if possible.
	 * (sync to gnome-app.c:gnome_app_set_menus) */
	if (!gnome_preferences_get_menubar_relief () && GTK_IS_MENU_BAR (menu_shell))
		gtk_menu_bar_set_shadow_type (GTK_MENU_BAR (menu_shell), GTK_SHADOW_NONE);
}


/**
 * gnome_app_create_menus
 * @app: Pointer to GNOME app object.
 * @uiinfo:
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
 * @app: Pointer to GNOME app object.
 * @uiinfo:
 * @relay_func:
 * @data:
 * @destroy_func:
 *
 * Description:
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
 * @app: Pointer to GNOME app object.
 * @uiinfo:
 * @user_data:
 *
 * Description:
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
	
	for(i = 0; uiinfo[i].type != GNOME_APP_UI_ENDOFINFO; i++) {
		int type;
		
		type = uiinfo[i].type;
		if (type == GNOME_APP_UI_SUBTREE_STOCK)
			type = GNOME_APP_UI_SUBTREE;
				
		if(type != GNOME_APP_UI_SUBTREE)
			continue;
		
		if(!ctmp)
			ctmp = alloca(strlen(above) + sizeof(" : ") + strlen(uiinfo[i].label)
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
 * @app: Pointer to GNOME app object.
 * @uiinfo:
 * @uibdata:
 *
 * Description:
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

	if (gnome_preferences_get_menus_have_tearoff ()) {
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

		pixmap = create_pixmap (GTK_WIDGET (toolbar), 
				uiinfo->pixmap_type, uiinfo->pixmap_info);

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

	if (is_radio || (uiinfo->type == GNOME_APP_UI_TOGGLEITEM)) {
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

		default:
			g_warning ("GnomeUIInfo element type %d is not valid "
				   "inside a toolbar radio item group",
				   (int) uiinfo->type);
		}
}


/**
 * gnome_app_fill_toolbar
 * @toolbar:
 * @uiinfo:
 * @accel_group:
 *
 * Description:
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
 * @toolbar:
 * @uiinfo:
 * @accel_group:
 * @user_data:
 *
 * Description:
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
 * @toolbar:
 * @uiinfo:
 * @uibdata:
 * @accel_group:
 *
 * Description:
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

	/* Configure toolbar to gnome preferences, if possible.
	 * (sync to gnome_app.c:gnome_app_set_toolbar) */	
	if (gnome_preferences_get_toolbar_lines ()) {
		gtk_toolbar_set_space_style (toolbar, GTK_TOOLBAR_SPACE_LINE);
		gtk_toolbar_set_space_size (toolbar, GNOME_PAD * 2);
	} else
		gtk_toolbar_set_space_size (toolbar, GNOME_PAD);

	if (!gnome_preferences_get_toolbar_relief_btn ())
		gtk_toolbar_set_button_relief (toolbar, GTK_RELIEF_NONE);

	if (!gnome_preferences_get_toolbar_labels ())
		gtk_toolbar_set_style (toolbar, GTK_TOOLBAR_ICONS);
}

/**
 * gnome_app_create_toolbar
 * @app: Pointer to GNOME app object.
 * @uiinfo:
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
 * @app: Pointer to GNOME app object.
 * @uiinfo:
 * @relay_func:
 * @data:
 * @destroy_func:
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
 * @app: Pointer to GNOME app object.
 * @uiinfo:
 * @user_data:
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
 * @app: Pointer to GNOME app object.
 * @uiinfo:
 * @uibdata:
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

	toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_BOTH);
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
g_strncmp_ignore_char( gchar *first, gchar *second, gint length, gchar ignored )
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
 * the path argument should be in the form "File/.../.../Something". "" will 
 * insert the item as the first one in the menubar "File/" will insert it as 
 * the first one in the File menu "File/Settings" will insert it after the 
 * Setting item in the File menu use of "File/<Separator>" should be obvious. 
 * However this stops after the first separator. I hope this explains use of 
 * the insert/remove functions well enough.
 */

/**
 * gnome_app_find_menu_pos
 * @parent: Root menu shell widget containing menu items to be searched
 * @path: Specifies the target menu item by menu path
 * @pos: (output) returned item position
 *
 * Description:
 * finds menu item described by path starting
 * in the GtkMenuShell top and returns its parent GtkMenuShell and the
 * position after this item in pos:  gtk_menu_shell_insert(p, w, pos)
 * would then insert widget w in GtkMenuShell p right after the menu item
 * described by path.
 **/

GtkWidget *
gnome_app_find_menu_pos (GtkWidget *parent, const gchar *path, gint *pos)
{
	GtkBin *item;
	gchar *label = NULL;
	GList *children;
	gchar *name_end;
	gchar *part, *transl;
	gint p;
	int  path_len;
	int  stripped_path_len;
	
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

	stripped_path_len = path_len;
	for ( p = 0; p < path_len; p++ )
	        if( transl[p] == '_' )
		        stripped_path_len--;
		
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
		if (label && (stripped_path_len == strlen (label)) &&
		    (g_strncmp_ignore_char (transl, label, path_len, '_') == 0)){
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
 * @app: Pointer to GNOME app object.
 * @path:
 * @items:
 *
 * Description: removes num items from the existing app's menu structure
 * beginning with item described by path
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
 * @app: Pointer to GNOME app object.
 * @path:
 * @start:
 * @items:
 *
 * Description:
 * Same as the gnome_app_remove_menus, except it removes the specified number
 * of @items from the existing app's menu structure begining with item described
 * by path, plus the number specified by @start - very useful for adding and
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
 * @app: Pointer to GNOME app object.
 * @path:
 * @uiinfo:
 * @uibdata:
 *
 * Description: inserts menus described by @uiinfo in existing app's menu
 * structure right after the item described by @path.
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
 * @app: Pointer to GNOME app object.
 * @path:
 * @menuinfo:
 *
 * Description:
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
 * @app: Pointer to GNOME app object.
 * @path:
 * @menuinfo:
 * @data:
 *
 * Description:
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
 * @app: Pointer to GNOME app object.
 * @path:
 * @menuinfo:
 * @relay_func:
 * @data:
 * @destroy_func:
 *
 * Description:
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

#ifdef ENABLE_NLS
gchar *
gnome_app_helper_gettext (const gchar *str)
{
	char *s;

        s = gettext (str);
	if ( s == str )
	        s = dgettext (PACKAGE, str);

	return s;
}
#endif


