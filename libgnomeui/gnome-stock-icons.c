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

#include "gnome-stock-ids.h"
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

    pixbuf = gdk_pixbuf_new_from_inline (inline_data, FALSE, -1, NULL);

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

    pixbuf = gdk_pixbuf_new_from_inline (inline_data, FALSE, -1, NULL);

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

    add_sized (factory, stock_new, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_NEW);
    add_sized (factory, stock_save, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_SAVE);
    add_sized (factory, stock_save_as, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_SAVE_AS);
    add_sized (factory, stock_revert, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_REVERT);
    add_sized (factory, stock_cut, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_CUT);
    add_sized (factory, stock_help, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_HELP);
    add_sized (factory, stock_print, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_PRINT);
    add_sized (factory, stock_search, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_SEARCH);
    add_sized (factory, stock_search_replace, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_SRCHRPL);
    add_sized (factory, stock_left_arrow, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_BACK);
    add_sized (factory, stock_right_arrow, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_FORWARD);
    add_sized (factory, stock_first, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_FIRST);
    add_sized (factory, stock_last, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_LAST);
    add_sized (factory, stock_home, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_HOME);
    add_sized (factory, stock_stop, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_STOP);
    add_sized (factory, stock_refresh, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_REFRESH);
    add_sized (factory, stock_undelete, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_UNDELETE);
    add_sized (factory, stock_open, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_OPEN);
    add_sized (factory, stock_close, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_CLOSE);
    add_sized (factory, stock_copy, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_COPY);
    add_sized (factory, stock_paste, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_PASTE);
    add_sized (factory, stock_properties, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_PROPERTIES);
    add_sized (factory, stock_preferences, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_PREFERENCES);
    add_sized (factory, stock_undo, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_UNDO);
    add_sized (factory, stock_redo, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_REDO);
    add_sized (factory, stock_timer, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_TIMER);
    add_sized (factory, stock_timer_stopped, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_TIMER_STOP);
    add_sized (factory, stock_mail, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_MAIL);
    add_sized (factory, stock_mail_receive, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_MAIL_RCV);
    add_sized (factory, stock_mail_send, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_MAIL_SND);
    add_sized (factory, stock_mail_reply, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_MAIL_RPL);
    add_sized (factory, stock_mail_forward, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_MAIL_FWD);
    add_sized (factory, stock_mail_compose, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_MAIL_NEW);
    add_sized (factory, stock_trash, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_TRASH);
    add_sized (factory, stock_trash_full, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_TRASH_FULL);
    add_sized (factory, stock_spellcheck, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_SPELLCHECK);
    add_sized (factory, stock_mic, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_MIC);
    add_sized (factory, stock_volume, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_VOLUME);
    add_sized (factory, stock_midi, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_MIDI);
    add_sized (factory, stock_line_in, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_LINE_IN);
    add_sized (factory, stock_cdrom, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_CDROM);
    add_sized (factory, stock_book_red, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_BOOK_RED);
    add_sized (factory, stock_book_green, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_BOOK_GREEN);
    add_sized (factory, stock_book_blue, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_BOOK_BLUE);
    add_sized (factory, stock_book_yellow, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_BOOK_YELLOW);
    add_sized (factory, stock_book_open, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_BOOK_OPEN);
    add_sized (factory, stock_convert, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_CONVERT);
    add_sized (factory, stock_jump_to, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_JUMP_TO);
    add_sized (factory, stock_up_arrow, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_UP);
    add_sized (factory, stock_down_arrow, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_DOWN);
    add_sized (factory, stock_top, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_TOP);
    add_sized (factory, stock_bottom, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_BOTTOM);
    add_sized (factory, stock_attach, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_ATTACH);
    add_sized (factory, stock_font, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_FONT);
    add_sized (factory, stock_index, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_INDEX);
    add_sized (factory, stock_exec, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_EXEC);
    add_sized (factory, stock_align_left, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_ALIGN_LEFT);
    add_sized (factory, stock_align_right, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_ALIGN_RIGHT);
    add_sized (factory, stock_align_center, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_ALIGN_CENTER);
    add_sized (factory, stock_align_justify, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_ALIGN_JUSTIFY);
    add_sized (factory, stock_text_bold, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_TEXT_BOLD);
    add_sized (factory, stock_text_italic, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_TEXT_ITALIC);
    add_sized (factory, stock_text_underline, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_TEXT_UNDERLINE);
    add_sized (factory, stock_text_strikeout, GTK_ICON_SIZE_BUTTON, GNOME_STOCK_PIXMAP_TEXT_STRIKEOUT);
    add_sized (factory, stock_not, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_NOT);
    add_sized (factory, stock_scores, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_SCORES);
    add_sized (factory, stock_exit, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_EXIT);
    add_sized (factory, stock_menu_about, GTK_ICON_SIZE_MENU, GNOME_STOCK_PIXMAP_ABOUT);
    add_sized (factory, stock_save, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_SAVE);
    add_sized (factory, stock_save_as, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_SAVE_AS);
    add_sized (factory, stock_revert, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_REVERT);
    add_sized (factory, stock_open, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_OPEN);
    add_sized (factory, stock_close, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_CLOSE);
    add_sized (factory, stock_exit, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_QUIT);
    add_sized (factory, stock_cut, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_CUT);
    add_sized (factory, stock_copy, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_COPY);
    add_sized (factory, stock_paste, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_PASTE);
    add_sized (factory, stock_properties, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_PROP);
    add_sized (factory, stock_preferences, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_PREF);
    add_sized (factory, stock_undo, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_UNDO);
    add_sized (factory, stock_redo, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_REDO);
    add_sized (factory, stock_print, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_PRINT);
    add_sized (factory, stock_search, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_SEARCH);
    add_sized (factory, stock_search_replace, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_SRCHRPL);
    add_sized (factory, stock_left_arrow, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_BACK);
    add_sized (factory, stock_right_arrow, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_FORWARD);
    add_sized (factory, stock_first, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_FIRST);
    add_sized (factory, stock_last, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_LAST);
    add_sized (factory, stock_home, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_HOME);
    add_sized (factory, stock_stop, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_STOP);
    add_sized (factory, stock_refresh, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_REFRESH);
    add_sized (factory, stock_undelete, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_UNDELETE);
    add_sized (factory, stock_timer, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_TIMER);
    add_sized (factory, stock_timer_stopped, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_TIMER_STOP);
    add_sized (factory, stock_mail, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_MAIL);
    add_sized (factory, stock_mail_receive, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_MAIL_RCV);
    add_sized (factory, stock_mail_send, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_MAIL_SND);
    add_sized (factory, stock_mail_reply, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_MAIL_RPL);
    add_sized (factory, stock_mail_forward, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_MAIL_FWD);
    add_sized (factory, stock_mail_compose, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_MAIL_NEW);
    add_sized (factory, stock_trash, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_TRASH);
    add_sized (factory, stock_trash_full, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_TRASH_FULL);
    add_sized (factory, stock_spellcheck, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_SPELLCHECK);
    add_sized (factory, stock_mic, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_MIC);
    add_sized (factory, stock_line_in, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_LINE_IN);
    add_sized (factory, stock_volume, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_VOLUME);
    add_sized (factory, stock_midi, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_MIDI);
    add_sized (factory, stock_cdrom, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_CDROM);
    add_sized (factory, stock_book_red, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_BOOK_RED);
    add_sized (factory, stock_book_green, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_BOOK_GREEN);
    add_sized (factory, stock_book_blue, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_BOOK_BLUE);
    add_sized (factory, stock_book_yellow, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_BOOK_YELLOW);
    add_sized (factory, stock_book_open, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_BOOK_OPEN);
    add_sized (factory, stock_convert, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_CONVERT);
    add_sized (factory, stock_jump_to, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_JUMP_TO);
    add_sized (factory, stock_menu_about, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_ABOUT);
    /* TODO: I shouldn't waste a pixmap for that */
    add_sized (factory, stock_menu_blank, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_BLANK); 
    add_sized (factory, stock_up_arrow, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_UP);
    add_sized (factory, stock_down_arrow, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_DOWN);
    add_sized (factory, stock_top, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_TOP);
    add_sized (factory, stock_bottom, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_BOTTOM);
    add_sized (factory, stock_attach, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_ATTACH);
    add_sized (factory, stock_index, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_INDEX);
    add_sized (factory, stock_font, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_FONT);
    add_sized (factory, stock_exec, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_EXEC);
    add_sized (factory, stock_align_left, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_ALIGN_LEFT);
    add_sized (factory, stock_align_right, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_ALIGN_RIGHT);
    add_sized (factory, stock_align_center, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_ALIGN_CENTER);
    add_sized (factory, stock_align_justify, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_ALIGN_JUSTIFY);
    add_sized (factory, stock_text_bold, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_TEXT_BOLD);
    add_sized (factory, stock_text_italic, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_TEXT_ITALIC);
    add_sized (factory, stock_text_underline, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_TEXT_UNDERLINE);
    add_sized (factory, stock_text_strikeout, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_TEXT_STRIKEOUT);
    add_sized (factory, stock_add, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_ADD);
    add_sized (factory, stock_clear, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_CLEAR);
    add_sized (factory, stock_colorselector, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_COLORSELECTOR);
    add_sized (factory, stock_remove, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_REMOVE);
    add_sized (factory, stock_table_borders, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_TABLE_BORDERS);
    add_sized (factory, stock_table_fill, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_TABLE_FILL);
    add_sized (factory, stock_text_bulleted_list, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_TEXT_BULLETED_LIST);
    add_sized (factory, stock_text_numbered_list, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_TEXT_NUMBERED_LIST);
    add_sized (factory, stock_text_indent, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_TEXT_INDENT);
    add_sized (factory, stock_text_unindent, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_PIXMAP_TEXT_UNINDENT);
    add_sized (factory, stock_button_ok, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_BUTTON_OK);
    add_sized (factory, stock_button_apply, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_BUTTON_APPLY);
    add_sized (factory, stock_button_cancel, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_BUTTON_CANCEL);
    add_sized (factory, stock_button_close, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_BUTTON_CLOSE);
    add_sized (factory, stock_button_yes, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_BUTTON_YES);
    add_sized (factory, stock_button_no, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_BUTTON_NO);
    add_sized (factory, stock_help, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_BUTTON_HELP);
    add_sized (factory, stock_right_arrow, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_BUTTON_NEXT);
    add_sized (factory, stock_left_arrow, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_BUTTON_PREV);
    add_sized (factory, stock_up_arrow, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_BUTTON_UP);
    add_sized (factory, stock_down_arrow, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_BUTTON_DOWN);
    add_sized (factory, stock_font, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_BUTTON_FONT);
    add_sized (factory, stock_new, GTK_ICON_SIZE_MENU, GNOME_STOCK_MENU_NEW);
    add_sized (factory, stock_menu_scores, GNOME_ICON_SIZE_TOOLBAR, GNOME_STOCK_MENU_SCORES);
}

static GtkStockItem builtin_items [] =
{
    { GTK_STOCK_DIALOG_INFO, N_("Information"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_PIXMAP_ADD, N_("Add"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_PIXMAP_CLEAR, N_("Clear"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_PIXMAP_COLORSELECTOR, N_("Select Color"), 0, 0, GETTEXT_PACKAGE },
    { GNOME_STOCK_PIXMAP_REMOVE, N_("Remove"), 0, 0, GETTEXT_PACKAGE },
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
