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

#ifndef __GNOME_STOCK_IDS_H__
#define __GNOME_STOCK_IDS_H__

#include <gmacros.h>

G_BEGIN_DECLS

/* The names of `well known' icons. I define these strings mainly to
   prevent errors due to typos. */

extern const char gnome_stock_pixmap_new[];
extern const char gnome_stock_pixmap_open[];
extern const char gnome_stock_pixmap_close[];
extern const char gnome_stock_pixmap_revert[];
extern const char gnome_stock_pixmap_save[];
extern const char gnome_stock_pixmap_save_as[];
extern const char gnome_stock_pixmap_cut[];
extern const char gnome_stock_pixmap_copy[];
extern const char gnome_stock_pixmap_paste[];
extern const char gnome_stock_pixmap_clear[];
extern const char gnome_stock_pixmap_properties[];
extern const char gnome_stock_pixmap_preferences[];
extern const char gnome_stock_pixmap_help[];
extern const char gnome_stock_pixmap_scores[];
extern const char gnome_stock_pixmap_print[];
extern const char gnome_stock_pixmap_search[];
extern const char gnome_stock_pixmap_srchrpl[];
extern const char gnome_stock_pixmap_back[];
extern const char gnome_stock_pixmap_forward[];
extern const char gnome_stock_pixmap_first[];
extern const char gnome_stock_pixmap_last[];
extern const char gnome_stock_pixmap_home[];
extern const char gnome_stock_pixmap_stop[];
extern const char gnome_stock_pixmap_refresh[];
extern const char gnome_stock_pixmap_undo[];
extern const char gnome_stock_pixmap_redo[];
extern const char gnome_stock_pixmap_timer[];
extern const char gnome_stock_pixmap_timer_stop[];
extern const char gnome_stock_pixmap_mail[];
extern const char gnome_stock_pixmap_mail_rcv[];
extern const char gnome_stock_pixmap_mail_snd[];
extern const char gnome_stock_pixmap_mail_rpl[];
extern const char gnome_stock_pixmap_mail_fwd[];
extern const char gnome_stock_pixmap_mail_new[];
extern const char gnome_stock_pixmap_trash[];
extern const char gnome_stock_pixmap_trash_full[];
extern const char gnome_stock_pixmap_undelete[];
extern const char gnome_stock_pixmap_spellcheck[];
extern const char gnome_stock_pixmap_mic[];
extern const char gnome_stock_pixmap_line_in[];
extern const char gnome_stock_pixmap_cdrom[];
extern const char gnome_stock_pixmap_volume[];
extern const char gnome_stock_pixmap_midi[];
extern const char gnome_stock_pixmap_book_red[];
extern const char gnome_stock_pixmap_book_green[];
extern const char gnome_stock_pixmap_book_blue[];
extern const char gnome_stock_pixmap_book_yellow[];
extern const char gnome_stock_pixmap_book_open[];
extern const char gnome_stock_pixmap_about[];
extern const char gnome_stock_pixmap_quit[];
extern const char gnome_stock_pixmap_multiple[];
extern const char gnome_stock_pixmap_not[];
extern const char gnome_stock_pixmap_convert[];
extern const char gnome_stock_pixmap_jump_to[];
extern const char gnome_stock_pixmap_up[];
extern const char gnome_stock_pixmap_down[];
extern const char gnome_stock_pixmap_top[];
extern const char gnome_stock_pixmap_bottom[];
extern const char gnome_stock_pixmap_attach[];
extern const char gnome_stock_pixmap_index[];
extern const char gnome_stock_pixmap_font[];
extern const char gnome_stock_pixmap_exec[];
extern const char gnome_stock_pixmap_align_left[];
extern const char gnome_stock_pixmap_align_right[];
extern const char gnome_stock_pixmap_align_center[];
extern const char gnome_stock_pixmap_align_justify[];
extern const char gnome_stock_pixmap_text_bold[];
extern const char gnome_stock_pixmap_text_italic[];
extern const char gnome_stock_pixmap_text_underline[];
extern const char gnome_stock_pixmap_text_strikeout[];
extern const char gnome_stock_pixmap_text_indent[];
extern const char gnome_stock_pixmap_text_unindent[];
extern const char gnome_stock_pixmap_colorselector[];
extern const char gnome_stock_pixmap_add[];
extern const char gnome_stock_pixmap_remove[];
extern const char gnome_stock_pixmap_table_borders[];
extern const char gnome_stock_pixmap_table_fill[];
extern const char gnome_stock_pixmap_text_bulleted_list[];
extern const char gnome_stock_pixmap_text_numbered_list[];

#define GNOME_STOCK_PIXMAP_NEW gnome_stock_pixmap_new
#define GNOME_STOCK_PIXMAP_OPEN gnome_stock_pixmap_open
#define GNOME_STOCK_PIXMAP_CLOSE gnome_stock_pixmap_close
#define GNOME_STOCK_PIXMAP_REVERT gnome_stock_pixmap_revert
#define GNOME_STOCK_PIXMAP_SAVE gnome_stock_pixmap_save
#define GNOME_STOCK_PIXMAP_SAVE_AS gnome_stock_pixmap_save_as
#define GNOME_STOCK_PIXMAP_CUT gnome_stock_pixmap_cut
#define GNOME_STOCK_PIXMAP_COPY gnome_stock_pixmap_copy
#define GNOME_STOCK_PIXMAP_PASTE gnome_stock_pixmap_paste
#define GNOME_STOCK_PIXMAP_CLEAR gnome_stock_pixmap_clear
#define GNOME_STOCK_PIXMAP_PROPERTIES gnome_stock_pixmap_properties
#define GNOME_STOCK_PIXMAP_PREFERENCES gnome_stock_pixmap_preferences
#define GNOME_STOCK_PIXMAP_HELP gnome_stock_pixmap_help
#define GNOME_STOCK_PIXMAP_SCORES gnome_stock_pixmap_scores
#define GNOME_STOCK_PIXMAP_PRINT gnome_stock_pixmap_print
#define GNOME_STOCK_PIXMAP_SEARCH gnome_stock_pixmap_search
#define GNOME_STOCK_PIXMAP_SRCHRPL gnome_stock_pixmap_srchrpl
#define GNOME_STOCK_PIXMAP_BACK gnome_stock_pixmap_back
#define GNOME_STOCK_PIXMAP_FORWARD gnome_stock_pixmap_forward
#define GNOME_STOCK_PIXMAP_FIRST gnome_stock_pixmap_first
#define GNOME_STOCK_PIXMAP_LAST gnome_stock_pixmap_last
#define GNOME_STOCK_PIXMAP_HOME gnome_stock_pixmap_home
#define GNOME_STOCK_PIXMAP_STOP gnome_stock_pixmap_stop
#define GNOME_STOCK_PIXMAP_REFRESH gnome_stock_pixmap_refresh
#define GNOME_STOCK_PIXMAP_UNDO gnome_stock_pixmap_undo
#define GNOME_STOCK_PIXMAP_REDO gnome_stock_pixmap_redo
#define GNOME_STOCK_PIXMAP_TIMER gnome_stock_pixmap_timer
#define GNOME_STOCK_PIXMAP_TIMER_STOP gnome_stock_pixmap_timer_stop
#define GNOME_STOCK_PIXMAP_MAIL gnome_stock_pixmap_mail
#define GNOME_STOCK_PIXMAP_MAIL_RCV gnome_stock_pixmap_mail_rcv
#define GNOME_STOCK_PIXMAP_MAIL_SND gnome_stock_pixmap_mail_snd
#define GNOME_STOCK_PIXMAP_MAIL_RPL gnome_stock_pixmap_mail_rpl
#define GNOME_STOCK_PIXMAP_MAIL_FWD gnome_stock_pixmap_mail_fwd
#define GNOME_STOCK_PIXMAP_MAIL_NEW gnome_stock_pixmap_mail_new
#define GNOME_STOCK_PIXMAP_TRASH gnome_stock_pixmap_trash
#define GNOME_STOCK_PIXMAP_TRASH_FULL gnome_stock_pixmap_trash_full
#define GNOME_STOCK_PIXMAP_UNDELETE gnome_stock_pixmap_undelete
#define GNOME_STOCK_PIXMAP_SPELLCHECK gnome_stock_pixmap_spellcheck
#define GNOME_STOCK_PIXMAP_MIC gnome_stock_pixmap_mic
#define GNOME_STOCK_PIXMAP_LINE_IN gnome_stock_pixmap_line_in
#define GNOME_STOCK_PIXMAP_CDROM gnome_stock_pixmap_cdrom
#define GNOME_STOCK_PIXMAP_VOLUME gnome_stock_pixmap_volume
#define GNOME_STOCK_PIXMAP_MIDI gnome_stock_pixmap_midi
#define GNOME_STOCK_PIXMAP_BOOK_RED gnome_stock_pixmap_book_red
#define GNOME_STOCK_PIXMAP_BOOK_GREEN gnome_stock_pixmap_book_green
#define GNOME_STOCK_PIXMAP_BOOK_BLUE gnome_stock_pixmap_book_blue
#define GNOME_STOCK_PIXMAP_BOOK_YELLOW gnome_stock_pixmap_book_yellow
#define GNOME_STOCK_PIXMAP_BOOK_OPEN gnome_stock_pixmap_book_open
#define GNOME_STOCK_PIXMAP_ABOUT gnome_stock_pixmap_about
#define GNOME_STOCK_PIXMAP_QUIT gnome_stock_pixmap_quit
#define GNOME_STOCK_PIXMAP_MULTIPLE gnome_stock_pixmap_multiple
#define GNOME_STOCK_PIXMAP_NOT gnome_stock_pixmap_not
#define GNOME_STOCK_PIXMAP_CONVERT gnome_stock_pixmap_convert
#define GNOME_STOCK_PIXMAP_JUMP_TO gnome_stock_pixmap_jump_to
#define GNOME_STOCK_PIXMAP_UP gnome_stock_pixmap_up
#define GNOME_STOCK_PIXMAP_DOWN gnome_stock_pixmap_down
#define GNOME_STOCK_PIXMAP_TOP gnome_stock_pixmap_top
#define GNOME_STOCK_PIXMAP_BOTTOM gnome_stock_pixmap_bottom
#define GNOME_STOCK_PIXMAP_ATTACH gnome_stock_pixmap_attach
#define GNOME_STOCK_PIXMAP_INDEX gnome_stock_pixmap_index
#define GNOME_STOCK_PIXMAP_FONT gnome_stock_pixmap_font
#define GNOME_STOCK_PIXMAP_EXEC gnome_stock_pixmap_exec

#define GNOME_STOCK_PIXMAP_ALIGN_LEFT gnome_stock_pixmap_align_left
#define GNOME_STOCK_PIXMAP_ALIGN_RIGHT gnome_stock_pixmap_align_right
#define GNOME_STOCK_PIXMAP_ALIGN_CENTER gnome_stock_pixmap_align_center
#define GNOME_STOCK_PIXMAP_ALIGN_JUSTIFY gnome_stock_pixmap_align_justify

#define GNOME_STOCK_PIXMAP_TEXT_BOLD gnome_stock_pixmap_text_bold
#define GNOME_STOCK_PIXMAP_TEXT_ITALIC gnome_stock_pixmap_text_italic
#define GNOME_STOCK_PIXMAP_TEXT_UNDERLINE gnome_stock_pixmap_text_underline
#define GNOME_STOCK_PIXMAP_TEXT_STRIKEOUT gnome_stock_pixmap_text_strikeout
#define GNOME_STOCK_PIXMAP_TEXT_INDENT gnome_stock_pixmap_text_indent
#define GNOME_STOCK_PIXMAP_TEXT_UNINDENT gnome_stock_pixmap_text_unindent
#define GNOME_STOCK_PIXMAP_COLORSELECTOR gnome_stock_pixmap_colorselector

#define GNOME_STOCK_PIXMAP_ADD gnome_stock_pixmap_add
#define GNOME_STOCK_PIXMAP_REMOVE gnome_stock_pixmap_remove

#define GNOME_STOCK_PIXMAP_TABLE_BORDERS gnome_stock_pixmap_table_borders
#define GNOME_STOCK_PIXMAP_TABLE_FILL gnome_stock_pixmap_table_fill

#define GNOME_STOCK_PIXMAP_TEXT_BULLETED_LIST gnome_stock_pixmap_text_bulleted_list
#define GNOME_STOCK_PIXMAP_TEXT_NUMBERED_LIST gnome_stock_pixmap_text_numbered_list

#define GNOME_STOCK_PIXMAP_EXIT        GNOME_STOCK_PIXMAP_QUIT

/*
 * Buttons
 */

extern const char gnome_stock_button_ok[];
extern const char gnome_stock_button_cancel[];
extern const char gnome_stock_button_yes[];
extern const char gnome_stock_button_no[];
extern const char gnome_stock_button_close[];
extern const char gnome_stock_button_apply[];
extern const char gnome_stock_button_help[];
extern const char gnome_stock_button_next[];
extern const char gnome_stock_button_prev[];
extern const char gnome_stock_button_up[];
extern const char gnome_stock_button_down[];
extern const char gnome_stock_button_font[];
#define GNOME_STOCK_BUTTON_OK gnome_stock_button_ok
#define GNOME_STOCK_BUTTON_CANCEL gnome_stock_button_cancel
#define GNOME_STOCK_BUTTON_YES gnome_stock_button_yes
#define GNOME_STOCK_BUTTON_NO gnome_stock_button_no
#define GNOME_STOCK_BUTTON_CLOSE gnome_stock_button_close
#define GNOME_STOCK_BUTTON_APPLY gnome_stock_button_apply
#define GNOME_STOCK_BUTTON_HELP gnome_stock_button_help
#define GNOME_STOCK_BUTTON_NEXT gnome_stock_button_next
#define GNOME_STOCK_BUTTON_PREV gnome_stock_button_prev
#define GNOME_STOCK_BUTTON_UP gnome_stock_button_up
#define GNOME_STOCK_BUTTON_DOWN gnome_stock_button_down
#define GNOME_STOCK_BUTTON_FONT gnome_stock_button_font

/*
 * Menus
 */

extern const char gnome_stock_menu_blank[];
extern const char gnome_stock_menu_new[];
extern const char gnome_stock_menu_save[];
extern const char gnome_stock_menu_save_as[];
extern const char gnome_stock_menu_revert[];
extern const char gnome_stock_menu_open[];
extern const char gnome_stock_menu_close[];
extern const char gnome_stock_menu_quit[];
extern const char gnome_stock_menu_cut[];
extern const char gnome_stock_menu_copy[];
extern const char gnome_stock_menu_paste[];
extern const char gnome_stock_menu_prop[];
extern const char gnome_stock_menu_pref[];
extern const char gnome_stock_menu_about[];
extern const char gnome_stock_menu_scores[];
extern const char gnome_stock_menu_undo[];
extern const char gnome_stock_menu_redo[];
extern const char gnome_stock_menu_print[];
extern const char gnome_stock_menu_search[];
extern const char gnome_stock_menu_srchrpl[];
extern const char gnome_stock_menu_back[];
extern const char gnome_stock_menu_forward[];
extern const char gnome_stock_menu_first[];
extern const char gnome_stock_menu_last[];
extern const char gnome_stock_menu_home[];
extern const char gnome_stock_menu_stop[];
extern const char gnome_stock_menu_refresh[];
extern const char gnome_stock_menu_mail[];
extern const char gnome_stock_menu_mail_rcv[];
extern const char gnome_stock_menu_mail_snd[];
extern const char gnome_stock_menu_mail_rpl[];
extern const char gnome_stock_menu_mail_fwd[];
extern const char gnome_stock_menu_mail_new[];
extern const char gnome_stock_menu_trash[];
extern const char gnome_stock_menu_trash_full[];
extern const char gnome_stock_menu_undelete[];
extern const char gnome_stock_menu_timer[];
extern const char gnome_stock_menu_timer_stop[];
extern const char gnome_stock_menu_spellcheck[];
extern const char gnome_stock_menu_mic[];
extern const char gnome_stock_menu_line_in[];
extern const char gnome_stock_menu_cdrom[];
extern const char gnome_stock_menu_volume[];
extern const char gnome_stock_menu_midi[];
extern const char gnome_stock_menu_book_red[];
extern const char gnome_stock_menu_book_green[];
extern const char gnome_stock_menu_book_blue[];
extern const char gnome_stock_menu_book_yellow[];
extern const char gnome_stock_menu_book_open[];
extern const char gnome_stock_menu_convert[];
extern const char gnome_stock_menu_jump_to[];
extern const char gnome_stock_menu_up[];
extern const char gnome_stock_menu_down[];
extern const char gnome_stock_menu_top[];
extern const char gnome_stock_menu_bottom[];
extern const char gnome_stock_menu_attach[];
extern const char gnome_stock_menu_index[];
extern const char gnome_stock_menu_font[];
extern const char gnome_stock_menu_exec[];
extern const char gnome_stock_menu_align_left[];
extern const char gnome_stock_menu_align_right[];
extern const char gnome_stock_menu_align_center[];
extern const char gnome_stock_menu_align_justify[];
extern const char gnome_stock_menu_text_bold[];
extern const char gnome_stock_menu_text_italic[];
extern const char gnome_stock_menu_text_underline[];
extern const char gnome_stock_menu_text_strikeout[];
#define GNOME_STOCK_MENU_BLANK gnome_stock_menu_blank
#define GNOME_STOCK_MENU_NEW gnome_stock_menu_new
#define GNOME_STOCK_MENU_SAVE gnome_stock_menu_save
#define GNOME_STOCK_MENU_SAVE_AS gnome_stock_menu_save_as
#define GNOME_STOCK_MENU_REVERT gnome_stock_menu_revert
#define GNOME_STOCK_MENU_OPEN gnome_stock_menu_open
#define GNOME_STOCK_MENU_CLOSE gnome_stock_menu_close
#define GNOME_STOCK_MENU_QUIT gnome_stock_menu_quit
#define GNOME_STOCK_MENU_CUT gnome_stock_menu_cut
#define GNOME_STOCK_MENU_COPY gnome_stock_menu_copy
#define GNOME_STOCK_MENU_PASTE gnome_stock_menu_paste
#define GNOME_STOCK_MENU_PROP gnome_stock_menu_prop
#define GNOME_STOCK_MENU_PREF gnome_stock_menu_pref
#define GNOME_STOCK_MENU_ABOUT gnome_stock_menu_about
#define GNOME_STOCK_MENU_SCORES gnome_stock_menu_scores
#define GNOME_STOCK_MENU_UNDO gnome_stock_menu_undo
#define GNOME_STOCK_MENU_REDO gnome_stock_menu_redo
#define GNOME_STOCK_MENU_PRINT gnome_stock_menu_print
#define GNOME_STOCK_MENU_SEARCH gnome_stock_menu_search
#define GNOME_STOCK_MENU_SRCHRPL gnome_stock_menu_srchrpl
#define GNOME_STOCK_MENU_BACK gnome_stock_menu_back
#define GNOME_STOCK_MENU_FORWARD gnome_stock_menu_forward
#define GNOME_STOCK_MENU_FIRST gnome_stock_menu_first
#define GNOME_STOCK_MENU_LAST gnome_stock_menu_last
#define GNOME_STOCK_MENU_HOME gnome_stock_menu_home
#define GNOME_STOCK_MENU_STOP gnome_stock_menu_stop
#define GNOME_STOCK_MENU_REFRESH gnome_stock_menu_refresh
#define GNOME_STOCK_MENU_MAIL gnome_stock_menu_mail
#define GNOME_STOCK_MENU_MAIL_RCV gnome_stock_menu_mail_rcv
#define GNOME_STOCK_MENU_MAIL_SND gnome_stock_menu_mail_snd
#define GNOME_STOCK_MENU_MAIL_RPL gnome_stock_menu_mail_rpl
#define GNOME_STOCK_MENU_MAIL_FWD gnome_stock_menu_mail_fwd
#define GNOME_STOCK_MENU_MAIL_NEW gnome_stock_menu_mail_new
#define GNOME_STOCK_MENU_TRASH gnome_stock_menu_trash
#define GNOME_STOCK_MENU_TRASH_FULL gnome_stock_menu_trash_full
#define GNOME_STOCK_MENU_UNDELETE gnome_stock_menu_undelete
#define GNOME_STOCK_MENU_TIMER gnome_stock_menu_timer
#define GNOME_STOCK_MENU_TIMER_STOP gnome_stock_menu_timer_stop
#define GNOME_STOCK_MENU_SPELLCHECK gnome_stock_menu_spellcheck
#define GNOME_STOCK_MENU_MIC gnome_stock_menu_mic
#define GNOME_STOCK_MENU_LINE_IN gnome_stock_menu_line_in
#define GNOME_STOCK_MENU_CDROM gnome_stock_menu_cdrom
#define GNOME_STOCK_MENU_VOLUME gnome_stock_menu_volume
#define GNOME_STOCK_MENU_MIDI gnome_stock_menu_midi
#define GNOME_STOCK_MENU_BOOK_RED gnome_stock_menu_book_red
#define GNOME_STOCK_MENU_BOOK_GREEN gnome_stock_menu_book_green
#define GNOME_STOCK_MENU_BOOK_BLUE gnome_stock_menu_book_blue
#define GNOME_STOCK_MENU_BOOK_YELLOW gnome_stock_menu_book_yellow
#define GNOME_STOCK_MENU_BOOK_OPEN gnome_stock_menu_book_open
#define GNOME_STOCK_MENU_CONVERT gnome_stock_menu_convert
#define GNOME_STOCK_MENU_JUMP_TO gnome_stock_menu_jump_to
#define GNOME_STOCK_MENU_UP gnome_stock_menu_up
#define GNOME_STOCK_MENU_DOWN gnome_stock_menu_down
#define GNOME_STOCK_MENU_TOP gnome_stock_menu_top
#define GNOME_STOCK_MENU_BOTTOM gnome_stock_menu_bottom
#define GNOME_STOCK_MENU_ATTACH gnome_stock_menu_attach
#define GNOME_STOCK_MENU_INDEX gnome_stock_menu_index
#define GNOME_STOCK_MENU_FONT gnome_stock_menu_font
#define GNOME_STOCK_MENU_EXEC gnome_stock_menu_exec

#define GNOME_STOCK_MENU_ALIGN_LEFT gnome_stock_menu_align_left
#define GNOME_STOCK_MENU_ALIGN_RIGHT gnome_stock_menu_align_right
#define GNOME_STOCK_MENU_ALIGN_CENTER gnome_stock_menu_align_center
#define GNOME_STOCK_MENU_ALIGN_JUSTIFY gnome_stock_menu_align_justify

#define GNOME_STOCK_MENU_TEXT_BOLD gnome_stock_menu_text_bold
#define GNOME_STOCK_MENU_TEXT_ITALIC gnome_stock_menu_text_italic
#define GNOME_STOCK_MENU_TEXT_UNDERLINE gnome_stock_menu_text_underline
#define GNOME_STOCK_MENU_TEXT_STRIKEOUT gnome_stock_menu_text_strikeout

#define GNOME_STOCK_MENU_EXIT     GNOME_STOCK_MENU_QUIT


G_END_DECLS

#endif /* GNOME_STOCK_H */
