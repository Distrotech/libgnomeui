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

int
main (int argc, char **argv)
{
    GtkWidget *app;
    GtkWidget *vbox;
    GtkWidget *frame1, *frame2;
    GtkWidget *fselector1, *fselector2;
    GtkWidget *selector;

    gnome_program_init ("selector_demo", "1.0", argc, argv,
			GNOMEUI_INIT, NULL);

    app = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title (GTK_WINDOW (app), "Selector Demo!");
    gtk_window_set_default_size (GTK_WINDOW (app), 400, 400);

    vbox = gtk_vbox_new (FALSE, GNOME_PAD);

    frame1 = gtk_frame_new ("Test 1");

    gtk_box_pack_start (GTK_BOX (vbox), frame1, TRUE, TRUE,
			GNOME_PAD);

    selector = gtk_label_new ("<selector widget goes here>");

    fselector1 = gnome_file_selector_new_custom ("test", NULL,
						 selector, FALSE);

    gtk_container_add (GTK_CONTAINER (frame1), fselector1);

    frame2 = gtk_frame_new ("Test 2");

    gtk_box_pack_start (GTK_BOX (vbox), frame2, FALSE, TRUE,
			GNOME_PAD);

    fselector2 = gnome_file_selector_new ("test2", NULL);

    gtk_container_add (GTK_CONTAINER (frame2), fselector2);

    gtk_container_add (GTK_CONTAINER (app), vbox);

    gtk_widget_show_all (app);
    gtk_main ();

    return 0;
}

