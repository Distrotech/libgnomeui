/*-*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 * GNOME GUI Library: testgnome.c
 * Copyright (C) 1998 Free Software Foundation
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "libgnome/libgnome.h"
#include "libgnomeui.h"

#define APPNAME "testgnome"
#define COPYRIGHT_NOTICE _("Copyright 1998, under the GNU General Public License.")

#ifndef VERSION
#define VERSION "0.0.0"
#endif

static GtkWidget * app;


static void popup_about()
{
  GtkWidget * ga;
  gchar * authors[] = { "The GNOME Team <gnome-list@gnome.org>",
                        NULL };

  ga = gnome_about_new (APPNAME,
                        VERSION, 
                        COPYRIGHT_NOTICE,
                        authors,
                        0,
                        0 );
  
  gtk_widget_show(ga);
}

/************************************
  Tests callbacks 
  *********************************/
static void gnome_dialog_test(GtkWidget * w, gpointer data)
{
  GtkWidget * dialog;

  dialog = gnome_dialog_new("Dialog Title", GNOME_STOCK_BUTTON_OK, NULL);
 /* Clicking any button closes. */
  gnome_dialog_set_close(GNOME_DIALOG(dialog), TRUE);
  gtk_widget_set_usize(dialog, 400, -1);

  gtk_widget_show(dialog);
}

/************************************
  Callbacks 
  *********************************/

static gint delete_event_cb(GtkWidget * w, gpointer data)
{
  gtk_main_quit();
  return TRUE; /* hmm, look up what's supposed to be here. */
}

static void about_cb(GtkWidget * w, gpointer data)
{
  popup_about();
}

static void save_cb(GtkWidget * w, gpointer data)
{
  g_warning("Save not implemented yet\n");
}

static void preferences_cb(GtkWidget *w, gpointer data)
{
  GtkWidget * pb;
}


/**************************************
  Set up the GUI
  ******************************/

static GnomeUIInfo help_menu[] = {
  GNOMEUIINFO_HELP(APPNAME),
  {GNOME_APP_UI_ITEM, N_("About..."), 
   N_("Tell about this application"), 
   about_cb, NULL, NULL,
   GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_ABOUT, 0, 0, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo file_menu[] = {
  {GNOME_APP_UI_ITEM, 
   N_("Save..."), N_("Write information to disk"), 
   save_cb, NULL, NULL,
   GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_SAVE, 's', 
   GDK_CONTROL_MASK, NULL },
  GNOMEUIINFO_SEPARATOR,
  {GNOME_APP_UI_ITEM, N_("Exit"), 
   N_("Quit the application, confirm any unsaved changes"),
   delete_event_cb, NULL, NULL,
   GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 'q', 
   GDK_CONTROL_MASK, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo tests_menu[] = {
  {GNOME_APP_UI_ITEM, 
   N_("Dialogs"), N_("Test dialogs"), 
   gnome_dialog_test, NULL, NULL,
   GNOME_APP_PIXMAP_NONE, NULL, '\0', 
   0, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo prefs_menu[] = {
  {GNOME_APP_UI_ITEM, N_("Preferences..."), 
   N_("Change application preferences"),
   preferences_cb, NULL, NULL,
   GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_PREF, 'p', 
   GDK_CONTROL_MASK, NULL },
  GNOMEUIINFO_END
};

static GnomeUIInfo main_menu[] = {
  GNOMEUIINFO_SUBTREE (N_("File"), file_menu),
  GNOMEUIINFO_SUBTREE (N_("Preferences"), prefs_menu),
  GNOMEUIINFO_SUBTREE (N_("Tests"), tests_menu),
#define MENU_HELP_POS 2
  GNOMEUIINFO_SUBTREE (N_("Help"), help_menu),
  GNOMEUIINFO_END
};


/*******************************
  Main
  *******************************/

static void prepare_app()
{
  GtkWidget * app_box;

  app = gnome_app_new( APPNAME, _("GNOME Test Program") ); 

  gnome_app_create_menus(GNOME_APP(app), main_menu);

  gtk_menu_item_right_justify(GTK_MENU_ITEM(main_menu[MENU_HELP_POS].widget));

  app_box = gtk_vbox_new ( FALSE, GNOME_PAD );
  gtk_container_border_width(GTK_CONTAINER(app_box), GNOME_PAD);

  gnome_app_set_contents ( GNOME_APP(app), app_box );

  gtk_signal_connect ( GTK_OBJECT(app), "destroy",
                       GTK_SIGNAL_FUNC(gtk_main_quit),
                       0 );

  gtk_signal_connect ( GTK_OBJECT (app), "delete_event",
                       GTK_SIGNAL_FUNC (delete_event_cb),
                       NULL );
 

  gtk_widget_show_all(app);
}

int main ( int argc, char ** argv )
{
  argp_program_version = VERSION;

  /* Initialize the i18n stuff */
  /*  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE); */ /* These args aren't defined until it's incorporated into the auto* build system. */

  gnome_init (APPNAME, 0, argc, argv, 0, 0);

  prepare_app();

  gtk_main();

  exit(EXIT_SUCCESS);
}


