/*
 * Copyright (C) 2000 SuSE GmbH
 * Author: Martin Baulig <baulig@suse.de>
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

#include <gnome.h>

#include "gnome-file-selector.h"
#include "gnome-icon-selector.h"

static GtkWidget *iselector;

static void
quit_cb (void)
{
        gtk_main_quit ();
}

static void
file_list_cb (void)
{
	GSList *list, *c;

	list = gnome_selector_get_file_list (GNOME_SELECTOR (iselector), TRUE);

	for (c = list; c; c = c->next) {
		g_print ("FILE: `%s'\n", c->data);
	}
}

static void
selection_cb (void)
{
	GSList *list, *c;

	list = gnome_selector_get_selection (GNOME_SELECTOR (iselector));

	for (c = list; c; c = c->next) {
		g_print ("SELECTION: `%s'\n", c->data);
	}
}

static GnomeUIInfo file_menu[] = {
	{ GNOME_APP_UI_ITEM, "Exit", NULL, quit_cb, NULL, NULL,
	  GNOME_APP_PIXMAP_STOCK, GNOME_STOCK_MENU_EXIT, 'X',
	  GDK_CONTROL_MASK, NULL },
        { GNOME_APP_UI_ENDOFINFO }
};

static GnomeUIInfo test_menu[] = {
	{ GNOME_APP_UI_ITEM, "Display file list", NULL, file_list_cb, NULL,
	  NULL, GNOME_APP_PIXMAP_NONE, NULL, 'f', GDK_CONTROL_MASK, NULL },
	{ GNOME_APP_UI_ITEM, "Display selection", NULL, selection_cb, NULL,
	  NULL, GNOME_APP_PIXMAP_NONE, NULL, 's', GDK_CONTROL_MASK, NULL },
        { GNOME_APP_UI_ENDOFINFO }
};

static GnomeUIInfo main_menu[] = {
        { GNOME_APP_UI_SUBTREE, ("File"), NULL, file_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
        { GNOME_APP_UI_SUBTREE, ("Test"), NULL, test_menu, NULL, NULL,
	  GNOME_APP_PIXMAP_NONE, NULL, 0, 0, NULL },
        { GNOME_APP_UI_ENDOFINFO }
};


#if 0

static void
test_cb (GtkWidget *button, gpointer data)
{
	GnomeCanvas *canvas = GNOME_CANVAS (data);

	gnome_canvas_scroll_to (canvas, 100, 100);
	gnome_canvas_update_now (canvas);
}

static void
test_canvas (void)
{
    GtkWidget *app, *vbox, *sw, *button, *canvas;
    GnomeCanvasGroup *root;
    GnomeCanvasItem *item;

    app = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (app), "Canvas Demo!");
    gtk_window_set_default_size (GTK_WINDOW (app), 400, 400);

    vbox = gtk_vbox_new (FALSE, GNOME_PAD);

    sw = gtk_scrolled_window_new (NULL, NULL);

    canvas = gnome_canvas_new ();

    gnome_canvas_set_scroll_region (GNOME_CANVAS (canvas),
				    0.0, 0.0, 1000.0, 1000.0);

    gtk_container_add (GTK_CONTAINER (sw), canvas);

    root = gnome_canvas_root (GNOME_CANVAS (canvas));

    item = gnome_canvas_item_new (root, GNOME_TYPE_CANVAS_RECT,
				  "x1", 50.0, "y1", 50.0, "x2", 900.0,
				  "y2", 900.0, "fill_color", "red",
				  "outline_color", "blue",
				  "width_pixels", 5, NULL);

    gnome_canvas_set_pixels_per_unit (GNOME_CANVAS (canvas), 0.5);

    gtk_box_pack_start (GTK_BOX (vbox), sw, TRUE, TRUE, GNOME_PAD);

    button = gtk_button_new_with_label ("Test!");

    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			test_cb, canvas);

    gtk_box_pack_start (GTK_BOX (vbox), button, FALSE, TRUE, GNOME_PAD);

    gtk_container_add (GTK_CONTAINER (app), vbox);

    gtk_widget_show_all (app);
}

#endif

int
main (int argc, char **argv)
{
    GtkWidget *app;
    GtkWidget *vbox;
    GtkWidget *frame1, *frame2;
    GtkWidget *fselector;
    GtkWidget *selector;
    gchar *pixmap_dir;

    gnome_program_init ("selector_demo", "1.0", argc, argv,
			GNOMEUI_INIT, NULL);

    app = gnome_app_new ("selector-demo", "Selector Demo");
    gtk_window_set_default_size (GTK_WINDOW (app), 400, 400);
    gnome_app_create_menus (GNOME_APP (app), main_menu);

    vbox = gtk_vbox_new (FALSE, GNOME_PAD);

    frame1 = gtk_frame_new ("Test 1");

    gtk_box_pack_start (GTK_BOX (vbox), frame1, FALSE, FALSE,
			GNOME_PAD);

    selector = gtk_label_new ("<selector widget goes here>");

    fselector = gnome_file_selector_new_custom ("test",
						"Albert Einstein",
						selector, NULL, 0);

    gtk_container_add (GTK_CONTAINER (frame1), fselector);

    frame2 = gtk_frame_new ("Test 2");

    gtk_box_pack_start (GTK_BOX (vbox), frame2, TRUE, TRUE,
			GNOME_PAD);

    iselector = gnome_icon_selector_new ("test2", NULL);

    pixmap_dir = gnome_unconditional_datadir_file ("pixmaps");
  
    gnome_selector_append_directory (GNOME_SELECTOR (iselector), pixmap_dir);

    gnome_selector_set_selection_mode (GNOME_SELECTOR (iselector),
				       GTK_SELECTION_MULTIPLE);

    g_free(pixmap_dir);

    gtk_container_add (GTK_CONTAINER (frame2), iselector);

    gnome_app_set_contents (GNOME_APP (app), vbox);

    /* test_canvas (); */

    gtk_widget_show_all (app);
    gtk_main ();

    gtk_widget_destroy (app);

    g_mem_profile ();

    return 0;
}

