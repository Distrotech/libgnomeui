/*  -*- Mode: C; c-set-style: linux; indent-tabs-mode: nil; c-basic-offset: 8 -*- */
/* Stock icons, buttons, and menu items.
   Copyright (C) 1997, 1998 Free Software Foundation

   The Gnome Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The Gnome Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the Gnome Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.

   Author: Eckehard Berns  */

#include <config.h>

#include <stdlib.h>
#include <string.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include <libgnome/gnome-i18n.h>

#include <libgnomeui/gnome-stock-icons.h>
#include "pixmaps/gnome-stock-pixbufs.h"

#ifndef GETTEXT_PACKAGE
#define GETTEXT_PACKAGE PACKAGE
#endif

static void G_GNUC_UNUSED
add_sized (GtkIconFactory *factory,
           const guchar   *inline_data,
           GtkIconSize     size,
           const gchar    *stock_id)
{
    GtkIconSet *set;
    GtkIconSource *source;
    GdkPixbuf *pixbuf;

    pixbuf = gdk_pixbuf_new_from_stream (-1, inline_data, FALSE, NULL);

    source = gtk_icon_source_new ();
    gtk_icon_source_set_pixbuf (source, pixbuf);
    gtk_icon_source_set_size (source, size);

    set = gtk_icon_set_new ();                
    gtk_icon_set_add_source (set, source);

    gtk_icon_factory_add (factory, stock_id, set);

    gdk_pixbuf_unref (pixbuf);
    gtk_icon_source_free (source);
    gtk_icon_set_unref (set);
}

static void G_GNUC_UNUSED
add_unsized (GtkIconFactory *factory,
             const guchar   *inline_data,
             const gchar    *stock_id)
{
    GtkIconSet *set;
    GtkIconSource *source;
    GdkPixbuf *pixbuf;

    pixbuf = gdk_pixbuf_new_from_stream (-1, inline_data, FALSE, NULL);

    source = gtk_icon_source_new ();
    gtk_icon_source_set_pixbuf (source, pixbuf);

    set = gtk_icon_set_new ();                
    gtk_icon_set_add_source (set, source);

    gtk_icon_factory_add (factory, stock_id, set);

    gdk_pixbuf_unref (pixbuf);
    gtk_icon_source_free (source);
    gtk_icon_set_unref (set);
}

static void G_GNUC_UNUSED
get_default_icons (GtkIconFactory *factory)
{
    /* KEEP IN SYNC with gtkstock.c */

    gnome_icon_size_toolbar = gtk_icon_size_register ("gnome-toolbar", 20, 20);

    add_sized (factory, stock_timer, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_TIMER);
    
    add_sized (factory, stock_mail, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_MAIL);
    add_sized (factory, stock_mail_receive, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_MAIL_RCV);
    add_sized (factory, stock_mail_send, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_MAIL_SND);
    add_sized (factory, stock_mail_reply, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_MAIL_RPL);
    add_sized (factory, stock_mail_forward, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_MAIL_FWD);
    
    add_sized (factory, stock_mic, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_MIC);
    add_sized (factory, stock_volume, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_VOLUME);
    add_sized (factory, stock_midi, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_MIDI);
    add_sized (factory, stock_line_in, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_LINE_IN);
    
    add_sized (factory, stock_book_red, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_BOOK_RED);
    add_sized (factory, stock_book_green, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_BOOK_GREEN);
    add_sized (factory, stock_book_blue, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_BOOK_BLUE);
    add_sized (factory, stock_book_yellow, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_BOOK_YELLOW);
    
    add_sized (factory, stock_attach, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_ATTACH);
    
    add_sized (factory, stock_not, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_NOT);
    add_sized (factory, stock_scores, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_SCORES);
    
    add_sized (factory, stock_menu_about, GTK_ICON_SIZE_MENU, GNOME_STOCK_PIXMAP_ABOUT);
    
    add_sized (factory, stock_timer, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_TIMER);
    
    add_sized (factory, stock_mail, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_MAIL);
    add_sized (factory, stock_mail_receive, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_MAIL_RCV);
    add_sized (factory, stock_mail_send, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_MAIL_SND);
    add_sized (factory, stock_mail_reply, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_MAIL_RPL);
    add_sized (factory, stock_mail_forward, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_MAIL_FWD);
    
    add_sized (factory, stock_line_in, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_LINE_IN);
    add_sized (factory, stock_volume, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_VOLUME);
    add_sized (factory, stock_midi, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_MIDI);
    
    add_sized (factory, stock_book_red, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_BOOK_RED);
    add_sized (factory, stock_book_green, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_BOOK_GREEN);
    add_sized (factory, stock_book_blue, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_BOOK_BLUE);
    add_sized (factory, stock_book_yellow, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_BOOK_YELLOW);
    
    add_sized (factory, stock_menu_about, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_ABOUT);
    /* TODO: I shouldn't waste a pixmap for that */
    add_sized (factory, stock_menu_blank, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_BLANK); 
    
    add_sized (factory, stock_attach, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_ATTACH);
    
    add_sized (factory, stock_table_borders, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_TABLE_BORDERS);
    add_sized (factory, stock_table_fill, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_TABLE_FILL);
    add_sized (factory, stock_text_bulleted_list, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_TEXT_BULLETED_LIST);
    add_sized (factory, stock_text_numbered_list, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_TEXT_NUMBERED_LIST);
    add_sized (factory, stock_text_indent, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_TEXT_INDENT);
    add_sized (factory, stock_text_unindent, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_TEXT_UNINDENT);
    
    add_sized (factory, stock_menu_scores, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_MENU_SCORES);
}

static GtkStockItem builtin_items [] =
{
    { GNOME_STOCK_PIXMAP_TABLE_BORDERS, N_("Table Borders"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_PIXMAP_TABLE_FILL, N_("Table Fill"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_PIXMAP_TEXT_BULLETED_LIST, N_("Bulleted List"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_PIXMAP_TEXT_NUMBERED_LIST, N_("Numbered List"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_PIXMAP_TEXT_INDENT, N_("Indent"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_PIXMAP_TEXT_UNINDENT, N_("Un-Indent"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_BUTTON_OK, N_("OK"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_BUTTON_APPLY, N_("Apply"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_BUTTON_CANCEL, N_("Cancel"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_BUTTON_CLOSE, N_("Close"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_BUTTON_YES, N_("Yes"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_BUTTON_NO, N_("No"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_BUTTON_HELP, N_("Help"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_BUTTON_NEXT, N_("Next"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_BUTTON_PREV, N_("Prev"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_BUTTON_UP, N_("Up"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_BUTTON_DOWN, N_("Down"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_BUTTON_FONT, N_("Font"), 0, 0, GETTEXT_PACKAGE },
};

void
init_gnome_stock_icons (void)
{
    static gboolean initialized = FALSE;
    GtkIconFactory *factory;

    if (initialized)
	return;
    else
	initialized = TRUE;

    factory = gtk_icon_factory_new ();

    get_default_icons (factory);

    gtk_icon_factory_add_default (factory);

    gtk_stock_add_static (builtin_items, G_N_ELEMENTS (builtin_items));
}
