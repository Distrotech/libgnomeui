/* Stock icons, buttons, and menu items.
   Copyright (C) 1997, 1998 Free Software Foundation
   All rights reserved.

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

   Author: Eckehard Berns
*/
/*
  @NOTATION@
*/

#ifndef __GNOME_STOCK_ICONS_H__
#define __GNOME_STOCK_ICONS_H__

#include <glib/gmacros.h>
#include <gtk/gtkiconfactory.h>

G_BEGIN_DECLS

#define GNOME_ICON_SIZE_TOOLBAR gnome_icon_size_toolbar
GtkIconSize gnome_icon_size_toolbar;

extern void init_gnome_stock_icons (void);

#define GNOME_STOCK_TIMER "gnome-stock-timer"
#define GNOME_STOCK_TIMER_STOP "gnome-stock-timer-stop"
#define GNOME_STOCK_TRASH_FULL "gnome-stock-trash-full"

#define GNOME_STOCK_SCORES "gnome-stock-scores"
#define GNOME_STOCK_ABOUT "gnome-stock-about"
#define GNOME_STOCK_BLANK "gnome-stock-blank"

#define GNOME_STOCK_VOLUME "gnome-stock-volume"
#define GNOME_STOCK_MIDI "gnome-stock-midi"
#define GNOME_STOCK_MIC "gnome-stock-mic"
#define GNOME_STOCK_LINE_IN "gnome-stock-line-in"

#define GNOME_STOCK_MAIL "gnome-stock-mail"
#define GNOME_STOCK_MAIL_RCV "gnome-stock-mail-rcv"
#define GNOME_STOCK_MAIL_SND "gnome-stock-mail-snd"
#define GNOME_STOCK_MAIL_RPL "gnome-stock-mail-rpl"
#define GNOME_STOCK_MAIL_FWD "gnome-stock-mail-fwd"
#define GNOME_STOCK_MAIL_NEW "gnome-stock-mail-new"
#define GNOME_STOCK_ATTACH "gnome-stock-attach"

#define GNOME_STOCK_BOOK_RED "gnome-stock-book-red"
#define GNOME_STOCK_BOOK_GREEN "gnome-stock-book-green"
#define GNOME_STOCK_BOOK_BLUE "gnome-stock-book-blue"
#define GNOME_STOCK_BOOK_YELLOW "gnome-stock-book-yellow"
#define GNOME_STOCK_BOOK_OPEN "gnome-stock-book-open"

#define GNOME_STOCK_MULTIPLE_FILE "gnome-stock-multiple-file"
#define GNOME_STOCK_NOT "gnome-stock-not"

#define GNOME_STOCK_TABLE_BORDERS "gnome-stock-table-borders"
#define GNOME_STOCK_TABLE_FILL "gnome-stock-table-fill"

#define GNOME_STOCK_TEXT_INDENT "gnome-stock-text-indent"
#define GNOME_STOCK_TEXT_UNINDENT "gnome-stock-text-unindent"
#define GNOME_STOCK_TEXT_BULLETED_LIST "gnome-stock-text-bulleted-list"
#define GNOME_STOCK_TEXT_NUMBERED_LIST "gnome-stock-text-numbered-list"

G_END_DECLS

#endif /* GNOME_STOCK_ICONS_H */
