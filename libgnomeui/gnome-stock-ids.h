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

#ifndef __GNOME_STOCK_IDS_H__
#define __GNOME_STOCK_IDS_H__

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

/* The names of `well known' icons. I define these strings mainly to
   prevent errors due to typos. */

#define GNOME_STOCK_PIXMAP_NEW         "New"
#define GNOME_STOCK_PIXMAP_OPEN        "Open"
#define GNOME_STOCK_PIXMAP_CLOSE       "Close"
#define GNOME_STOCK_PIXMAP_REVERT      "Revert"
#define GNOME_STOCK_PIXMAP_SAVE        "Save"
#define GNOME_STOCK_PIXMAP_SAVE_AS     "Save As"
#define GNOME_STOCK_PIXMAP_CUT         "Cut"
#define GNOME_STOCK_PIXMAP_COPY        "Copy"
#define GNOME_STOCK_PIXMAP_PASTE       "Paste"
#define GNOME_STOCK_PIXMAP_CLEAR       "Clear"
#define GNOME_STOCK_PIXMAP_PROPERTIES  "Properties"
#define GNOME_STOCK_PIXMAP_PREFERENCES "Preferences"
#define GNOME_STOCK_PIXMAP_HELP        "Help"
#define GNOME_STOCK_PIXMAP_SCORES      "Scores"
#define GNOME_STOCK_PIXMAP_PRINT       "Print"
#define GNOME_STOCK_PIXMAP_SEARCH      "Search"
#define GNOME_STOCK_PIXMAP_SRCHRPL     "Search/Replace"
#define GNOME_STOCK_PIXMAP_BACK        "Back"
#define GNOME_STOCK_PIXMAP_FORWARD     "Forward"
#define GNOME_STOCK_PIXMAP_FIRST       "First"
#define GNOME_STOCK_PIXMAP_LAST        "Last"
#define GNOME_STOCK_PIXMAP_HOME        "Home"
#define GNOME_STOCK_PIXMAP_STOP        "Stop"
#define GNOME_STOCK_PIXMAP_REFRESH     "Refresh"
#define GNOME_STOCK_PIXMAP_UNDO        "Undo"
#define GNOME_STOCK_PIXMAP_REDO        "Redo"
#define GNOME_STOCK_PIXMAP_TIMER       "Timer"
#define GNOME_STOCK_PIXMAP_TIMER_STOP  "Timer Stopped"
#define GNOME_STOCK_PIXMAP_MAIL	       "Mail"
#define GNOME_STOCK_PIXMAP_MAIL_RCV    "Receive Mail"
#define GNOME_STOCK_PIXMAP_MAIL_SND    "Send Mail"
#define GNOME_STOCK_PIXMAP_MAIL_RPL    "Reply to Mail"
#define GNOME_STOCK_PIXMAP_MAIL_FWD    "Forward Mail"
#define GNOME_STOCK_PIXMAP_MAIL_NEW    "New Mail"
#define GNOME_STOCK_PIXMAP_TRASH       "Trash"
#define GNOME_STOCK_PIXMAP_TRASH_FULL  "Trash Full"
#define GNOME_STOCK_PIXMAP_UNDELETE    "Undelete"
#define GNOME_STOCK_PIXMAP_SPELLCHECK  "Spellchecker"
#define GNOME_STOCK_PIXMAP_MIC         "Microphone"
#define GNOME_STOCK_PIXMAP_LINE_IN     "Line In"
#define GNOME_STOCK_PIXMAP_CDROM       "Cdrom"
#define GNOME_STOCK_PIXMAP_VOLUME      "Volume"
#define GNOME_STOCK_PIXMAP_MIDI        "Midi"
#define GNOME_STOCK_PIXMAP_BOOK_RED    "Book Red"
#define GNOME_STOCK_PIXMAP_BOOK_GREEN  "Book Green"
#define GNOME_STOCK_PIXMAP_BOOK_BLUE   "Book Blue"
#define GNOME_STOCK_PIXMAP_BOOK_YELLOW "Book Yellow"
#define GNOME_STOCK_PIXMAP_BOOK_OPEN   "Book Open"
#define GNOME_STOCK_PIXMAP_ABOUT       "About"
#define GNOME_STOCK_PIXMAP_QUIT        "Quit"
#define GNOME_STOCK_PIXMAP_MULTIPLE    "Multiple"
#define GNOME_STOCK_PIXMAP_NOT         "Not"
#define GNOME_STOCK_PIXMAP_CONVERT     "Convert"
#define GNOME_STOCK_PIXMAP_JUMP_TO     "Jump To"
#define GNOME_STOCK_PIXMAP_UP          "Up"
#define GNOME_STOCK_PIXMAP_DOWN        "Down"
#define GNOME_STOCK_PIXMAP_TOP         "Top"
#define GNOME_STOCK_PIXMAP_BOTTOM      "Bottom"
#define GNOME_STOCK_PIXMAP_ATTACH      "Attach"
#define GNOME_STOCK_PIXMAP_INDEX       "Index"
#define GNOME_STOCK_PIXMAP_FONT        "Font"
#define GNOME_STOCK_PIXMAP_EXEC        "Exec"

#define GNOME_STOCK_PIXMAP_ALIGN_LEFT    "Left"
#define GNOME_STOCK_PIXMAP_ALIGN_RIGHT   "Right"
#define GNOME_STOCK_PIXMAP_ALIGN_CENTER  "Center"
#define GNOME_STOCK_PIXMAP_ALIGN_JUSTIFY "Justify"

#define GNOME_STOCK_PIXMAP_TEXT_BOLD      "Bold"
#define GNOME_STOCK_PIXMAP_TEXT_ITALIC    "Italic"
#define GNOME_STOCK_PIXMAP_TEXT_UNDERLINE "Underline"
#define GNOME_STOCK_PIXMAP_TEXT_STRIKEOUT "Strikeout"

#define GNOME_STOCK_PIXMAP_TEXT_INDENT "Text Indent"
#define GNOME_STOCK_PIXMAP_TEXT_UNINDENT "Text Unindent"

#define GNOME_STOCK_PIXMAP_EXIT        GNOME_STOCK_PIXMAP_QUIT

#define GNOME_STOCK_PIXMAP_COLORSELECTOR "Color Select"

#define GNOME_STOCK_PIXMAP_ADD    "Add"
#define GNOME_STOCK_PIXMAP_REMOVE "Remove"

#define GNOME_STOCK_PIXMAP_TABLE_BORDERS "Table Borders"
#define GNOME_STOCK_PIXMAP_TABLE_FILL "Table Fill"

#define GNOME_STOCK_PIXMAP_TEXT_BULLETED_LIST "Text Bulleted List"
#define GNOME_STOCK_PIXMAP_TEXT_NUMBERED_LIST "Text Numbered List"

/*
 * Buttons
 */

#define GNOME_STOCK_BUTTON_OK     "Button_Ok"
#define GNOME_STOCK_BUTTON_CANCEL "Button_Cancel"
#define GNOME_STOCK_BUTTON_YES    "Button_Yes"
#define GNOME_STOCK_BUTTON_NO     "Button_No"
#define GNOME_STOCK_BUTTON_CLOSE  "Button_Close"
#define GNOME_STOCK_BUTTON_APPLY  "Button_Apply"
#define GNOME_STOCK_BUTTON_HELP   "Button_Help"
#define GNOME_STOCK_BUTTON_NEXT   "Button_Next"
#define GNOME_STOCK_BUTTON_PREV   "Button_Prev"
#define GNOME_STOCK_BUTTON_UP     "Button_Up"
#define GNOME_STOCK_BUTTON_DOWN   "Button_Down"
#define GNOME_STOCK_BUTTON_FONT   "Button_Font"


/*
 * Menus
 */

#define GNOME_STOCK_MENU_BLANK        "Menu_"
#define GNOME_STOCK_MENU_NEW          "Menu_New"
#define GNOME_STOCK_MENU_SAVE         "Menu_Save"
#define GNOME_STOCK_MENU_SAVE_AS      "Menu_Save As"
#define GNOME_STOCK_MENU_REVERT       "Menu_Revert"
#define GNOME_STOCK_MENU_OPEN         "Menu_Open"
#define GNOME_STOCK_MENU_CLOSE        "Menu_Close"
#define GNOME_STOCK_MENU_QUIT         "Menu_Quit"
#define GNOME_STOCK_MENU_CUT          "Menu_Cut"
#define GNOME_STOCK_MENU_COPY         "Menu_Copy"
#define GNOME_STOCK_MENU_PASTE        "Menu_Paste"
#define GNOME_STOCK_MENU_PROP         "Menu_Properties"
#define GNOME_STOCK_MENU_PREF         "Menu_Preferences"
#define GNOME_STOCK_MENU_ABOUT        "Menu_About"
#define GNOME_STOCK_MENU_SCORES       "Menu_Scores"
#define GNOME_STOCK_MENU_UNDO         "Menu_Undo"
#define GNOME_STOCK_MENU_REDO         "Menu_Redo"
#define GNOME_STOCK_MENU_PRINT        "Menu_Print"
#define GNOME_STOCK_MENU_SEARCH       "Menu_Search"
#define GNOME_STOCK_MENU_SRCHRPL      "Menu_Search/Replace"
#define GNOME_STOCK_MENU_BACK         "Menu_Back"
#define GNOME_STOCK_MENU_FORWARD      "Menu_Forward"
#define GNOME_STOCK_MENU_FIRST        "Menu_First"
#define GNOME_STOCK_MENU_LAST         "Menu_Last"
#define GNOME_STOCK_MENU_HOME         "Menu_Home"
#define GNOME_STOCK_MENU_STOP         "Menu_Stop"
#define GNOME_STOCK_MENU_REFRESH      "Menu_Refresh"
#define GNOME_STOCK_MENU_MAIL         "Menu_Mail"
#define GNOME_STOCK_MENU_MAIL_RCV     "Menu_Receive Mail"
#define GNOME_STOCK_MENU_MAIL_SND     "Menu_Send Mail"
#define GNOME_STOCK_MENU_MAIL_RPL     "Menu_Reply to Mail"
#define GNOME_STOCK_MENU_MAIL_FWD     "Menu_Forward Mail"
#define GNOME_STOCK_MENU_MAIL_NEW     "Menu_New Mail"
#define GNOME_STOCK_MENU_TRASH        "Menu_Trash"
#define GNOME_STOCK_MENU_TRASH_FULL   "Menu_Trash Full"
#define GNOME_STOCK_MENU_UNDELETE     "Menu_Undelete"
#define GNOME_STOCK_MENU_TIMER        "Menu_Timer"
#define GNOME_STOCK_MENU_TIMER_STOP   "Menu_Timer Stopped"
#define GNOME_STOCK_MENU_SPELLCHECK   "Menu_Spellchecker"
#define GNOME_STOCK_MENU_MIC          "Menu_Microphone"
#define GNOME_STOCK_MENU_LINE_IN      "Menu_Line In"
#define GNOME_STOCK_MENU_CDROM	     "Menu_Cdrom"
#define GNOME_STOCK_MENU_VOLUME       "Menu_Volume"
#define GNOME_STOCK_MENU_MIDI         "Menu_Midi"
#define GNOME_STOCK_MENU_BOOK_RED     "Menu_Book Red"
#define GNOME_STOCK_MENU_BOOK_GREEN   "Menu_Book Green"
#define GNOME_STOCK_MENU_BOOK_BLUE    "Menu_Book Blue"
#define GNOME_STOCK_MENU_BOOK_YELLOW  "Menu_Book Yellow"
#define GNOME_STOCK_MENU_BOOK_OPEN    "Menu_Book Open"
#define GNOME_STOCK_MENU_CONVERT      "Menu_Convert"
#define GNOME_STOCK_MENU_JUMP_TO      "Menu_Jump To"
#define GNOME_STOCK_MENU_UP           "Menu_Up"
#define GNOME_STOCK_MENU_DOWN         "Menu_Down"
#define GNOME_STOCK_MENU_TOP          "Menu_Top"
#define GNOME_STOCK_MENU_BOTTOM       "Menu_Bottom"
#define GNOME_STOCK_MENU_ATTACH       "Menu_Attach"
#define GNOME_STOCK_MENU_INDEX        "Menu_Index"
#define GNOME_STOCK_MENU_FONT         "Menu_Font"
#define GNOME_STOCK_MENU_EXEC         "Menu_Exec"

#define GNOME_STOCK_MENU_ALIGN_LEFT     "Menu_Left"
#define GNOME_STOCK_MENU_ALIGN_RIGHT    "Menu_Right"
#define GNOME_STOCK_MENU_ALIGN_CENTER   "Menu_Center"
#define GNOME_STOCK_MENU_ALIGN_JUSTIFY  "Menu_Justify"

#define GNOME_STOCK_MENU_TEXT_BOLD      "Menu_Bold"
#define GNOME_STOCK_MENU_TEXT_ITALIC    "Menu_Italic"
#define GNOME_STOCK_MENU_TEXT_UNDERLINE "Menu_Underline"
#define GNOME_STOCK_MENU_TEXT_STRIKEOUT "Menu_Strikeout"

#define GNOME_STOCK_MENU_EXIT     GNOME_STOCK_MENU_QUIT


END_GNOME_DECLS

#endif /* GNOME_STOCK_H */
