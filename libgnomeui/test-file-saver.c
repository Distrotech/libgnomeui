/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME GUI Library - test-file-saver.c
 * Copyright (C) 2000  Red Hat Inc.,
 * All rights reserved.
 *
 * Author: Havoc Pennington <hp@redhat.com>
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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
 */
/*
  @NOTATION@
*/
#include "gnome-file-saver.h"
#include "libgnomeui.h"

int
main(int argc, char** argv)
{
        GtkWidget *fs;
  
        gnome_program_init("test-file-saver", "0.1",
                           argc, argv, GNOMEUI_INIT, GNOME_GCONF_INIT,
                           NULL);

        fs = gnome_file_saver_new("Save a File", "document-saver");

        gnome_file_saver_add_mime_type(GNOME_FILE_SAVER(fs),
                                       "text/plain");

        gnome_file_saver_add_mime_type(GNOME_FILE_SAVER(fs),
                                       "text/html");

        gnome_file_saver_add_mime_type(GNOME_FILE_SAVER(fs),
                                       "image/gif");
        
        gtk_widget_show(fs);
        
        gtk_main();

        return 0;
}
