/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* GNOME GUI Library - test-recently-used.c
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
#include "gnome-recently-used.h"
#include <gnome.h>

static GnomeRecentlyUsed *ru = NULL;
static GHashTable* doc_to_menuitem = NULL;

static void
add_clicked(GtkWidget* button, GtkWidget *entry)
{
        gchar* text;

        text = gtk_editable_get_chars(GTK_EDITABLE (entry), 0, -1);

        gnome_recently_used_add_simple(ru,
                                       "echo 'hello'",
                                       text,
                                       NULL, NULL, NULL, NULL);
        g_free(text);
}

static void
add_to_menu(GtkWidget* menu, GnomeRecentDocument *doc)
{
        GtkWidget *mi;
        const gchar* name;
        GtkWidget *label;
        
        name = gnome_recent_document_peek(doc, "menu-text");

        label = gtk_label_new(name);
        mi = gtk_menu_item_new();

        gtk_container_add(GTK_CONTAINER(mi), label);
        
        /* ref for the menu item */
        gnome_recent_document_ref(doc);
        gtk_object_set_data_full(GTK_OBJECT(mi), "doc", 
                                 doc,
                                 (GtkDestroyNotify)gnome_recent_document_unref);

        gtk_object_set_data(GTK_OBJECT(mi), "label", label);
        
        gtk_widget_show_all(mi);

        g_hash_table_insert(doc_to_menuitem, doc, mi);
        
        /* FIXME actually we'd want to sort by time... */
        gtk_menu_shell_append(GTK_MENU_SHELL (menu), mi);
}

static void
item_removed(GnomeRecentlyUsed* recently_used, GnomeRecentDocument* doc, gpointer user_data)
{
        /*UNUSED GtkWidget *menu = user_data; */
        GtkWidget *mi;

        mi = g_hash_table_lookup(doc_to_menuitem, doc);

        /* removed before we even heard about it */
        if (mi == NULL)
                return;
        else {
                g_hash_table_remove(doc_to_menuitem, doc);
                gtk_widget_destroy(mi);
        }
}

static void
item_added(GnomeRecentlyUsed* recently_used, GnomeRecentDocument* doc, gpointer user_data)
{
        GtkWidget *menu = user_data;

        add_to_menu(menu, doc);
}

static void
item_changed(GnomeRecentlyUsed* recently_used, GnomeRecentDocument* doc, gpointer user_data)
{
        /*UNUSED GtkWidget *menu = user_data; */
        const gchar* name;
        GtkWidget *mi;
        
        name = gnome_recent_document_peek(doc, "menu-text");

        mi = g_hash_table_lookup(doc_to_menuitem, doc);

        if (mi == NULL)
                return; /* changed before we heard about it */
        
        gtk_label_set_text(gtk_object_get_data(GTK_OBJECT(mi), "label"),
                           name);
}

int
main(int argc, char** argv)
{
        GtkWidget *window;
        GtkWidget *menu;
        GtkWidget *vbox;
        GtkWidget *entry;
        GtkWidget *button;
        GtkWidget *option;
        GSList *list;
        GSList *iter;
        
        gnome_program_init("test-recently-used", "0.1",
                           argc, argv, GNOMEUI_INIT, GNOME_GCONF_INIT,
                           NULL);

        doc_to_menuitem = g_hash_table_new(NULL, NULL);
        
        window = gtk_window_new(GTK_WINDOW_TOPLEVEL);

        vbox = gtk_vbox_new(FALSE, 6);

        gtk_container_add(GTK_CONTAINER(window), vbox);
        
        entry = gtk_entry_new();

        gtk_box_pack_start(GTK_BOX(vbox), entry,
                           FALSE, FALSE, 0);

        button = gtk_button_new_with_label("Add");

        gtk_signal_connect(GTK_OBJECT(button), "clicked",
                           GTK_SIGNAL_FUNC(add_clicked),
                           entry);
        
        gtk_box_pack_start(GTK_BOX(vbox), button,
                           FALSE, FALSE, 0);

        option = gtk_option_menu_new();

        gtk_box_pack_start(GTK_BOX(vbox), option,
                           FALSE, FALSE, 0);
        
        menu = gtk_menu_new();

        ru = gnome_recently_used_new();

        gtk_signal_connect(GTK_OBJECT(ru),
                           "document_added",
                           GTK_SIGNAL_FUNC(item_added),
                           menu);

        gtk_signal_connect(GTK_OBJECT(ru),
                           "document_removed",
                           GTK_SIGNAL_FUNC(item_removed),
                           menu);

        gtk_signal_connect(GTK_OBJECT(ru),
                           "document_changed",
                           GTK_SIGNAL_FUNC(item_changed),
                           menu);

        
        list = gnome_recently_used_get_all(ru);

        iter = list;

        while (iter != NULL) {
                GnomeRecentDocument *doc = iter->data;

                add_to_menu(menu, doc);

                iter = g_slist_next(iter);
        }
        
        g_slist_free(list);
        
        gtk_option_menu_set_menu(GTK_OPTION_MENU(option), menu);

        gtk_widget_show_all(window);
        
        gtk_main();

        return 0;
}
