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

#ifndef __GNOME_STOCK_H__
#define __GNOME_STOCK_H__



#include <libgnome/gnome-defs.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkvbox.h>
#include <gtk/gtkbutton.h>
#include "gnome-pixmap.h"
/* #include <gdk_imlib.h> */


/* A short description:
 *
 *  These functions provide an applications programmer with default
 *  icons for toolbars, menu pixmaps, etc. One such `icon' should have
 *  at least three pixmaps to reflect it's state. There is a `regular'
 *  pixmap, a `disabled' pixmap and a `focused' pixmap. You can get
 *  either each of these pixmaps by calling gnome_stock_pixmap or you
 *  can get a widget by calling gnome_stock_pixmap_widget. This widget
 *  is a container which gtk_widget_shows the pixmap, that is
 *  reflecting the current state of the widget. If for example you
 *  gtk_container_add this widget to a button, which is currently not
 *  sensitive, the widget will just show the `disabled' pixmap. If the
 *  state of the button changes to sensitive, the widget will change to
 *  the `regular' pixmap. The `focused' pixmap will be shown, when the
 *  mouse pointer enters the widget.
 *
 *  To support themability, we use (char *) to call those functions. A
 *  new theme might register new icons by calling
 *  gnome_stock_pixmap_register, or may change existing icons by
 *  calling gnome_stock_pixmap_change. An application should check (by
 *  calling gnome_stock_pixmap_checkfor), if the current theme supports
 *  an uncommon icon, before using it. The only icons an app can rely
 *  on, are those defined in this haeder file.
 *
 *  We now have stock buttons too. To use them, just replace any
 *  gtk_button_new{_with_label} with
 *  gnome_stock_button(GNOME_STOCK_BUTTON_...).  This function returns
 *  a GtkButton with a gettexted default text and an icon.
 *
 *  There's an additional feature, which might be interesting. If an
 *  application calls gnome_stock_pixmap_register and uses it by
 *  calling gnome_stock_pixmap_widget, it doesn't have to care about
 *  the state_changed signal to display the appropriate pixmap
 *  itself. Additionally gnome-stock generates a disabled version of a
 *  pixmap automatically, when no pixmap for a disabled state is
 *  provided.
 */

/* State:
 *
 *  currently implemented:
 *    - gnome_stock_pixmap
 *    - gnome_stock_pixmap_widget
 *    - gnome_stock_pixmap_checkfor
 *    - GnomeStockPixmapWidget
 *    - gnome_stock_button
 *    - gnome_stock_pixmap_register
 *
 *  not implemented:
 *    - gnome_stock_pixmap_change
 */

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
#define GNOME_STOCK_PIXMAP_SPELLCHECK  "Spellchecker"
#define GNOME_STOCK_PIXMAP_MIC         "Microphone"
#define GNOME_STOCK_PIXMAP_LINE_IN     "Line In"
#define GNOME_STOCK_PIXMAP_VOLUME      "Volume"
#define GNOME_STOCK_PIXMAP_BOOK_RED    "Book Red"
#define GNOME_STOCK_PIXMAP_BOOK_GREEN  "Book Green"
#define GNOME_STOCK_PIXMAP_BOOK_BLUE   "Book Blue"
#define GNOME_STOCK_PIXMAP_BOOK_YELLOW "Book Yellow"
#define GNOME_STOCK_PIXMAP_BOOK_OPEN   "Book Open"
#define GNOME_STOCK_PIXMAP_ABOUT       "About"
#define GNOME_STOCK_PIXMAP_QUIT        "Quit"
#define GNOME_STOCK_PIXMAP_MULTIPLE    "Multiple"
#define GNOME_STOCK_PIXMAP_NOT         "Not"

#define GNOME_STOCK_PIXMAP_EXIT        GNOME_STOCK_PIXMAP_QUIT


/* The basic pixmap version of an icon. */

#define GNOME_STOCK_PIXMAP_REGULAR     "regular"
#define GNOME_STOCK_PIXMAP_DISABLED    "disabled"
#define GNOME_STOCK_PIXMAP_FOCUSED     "focused"



/* some internal definitions */

typedef struct _GnomeStockPixmapEntryAny     GnomeStockPixmapEntryAny;
typedef struct _GnomeStockPixmapEntryData    GnomeStockPixmapEntryData;
typedef struct _GnomeStockPixmapEntryFile    GnomeStockPixmapEntryFile;
typedef struct _GnomeStockPixmapEntryPath    GnomeStockPixmapEntryPath;
typedef struct _GnomeStockPixmapEntryWidget  GnomeStockPixmapEntryWidget;
typedef struct _GnomeStockPixmapEntryGPixmap GnomeStockPixmapEntryGPixmap;
typedef union  _GnomeStockPixmapEntry        GnomeStockPixmapEntry;

typedef enum {
        GNOME_STOCK_PIXMAP_TYPE_NONE,
        GNOME_STOCK_PIXMAP_TYPE_DATA,
        GNOME_STOCK_PIXMAP_TYPE_FILE,
        GNOME_STOCK_PIXMAP_TYPE_PATH,
        GNOME_STOCK_PIXMAP_TYPE_WIDGET,
	GNOME_STOCK_PIXMAP_TYPE_IMLIB,
	GNOME_STOCK_PIXMAP_TYPE_IMLIB_SCALED,
	GNOME_STOCK_PIXMAP_TYPE_GPIXMAP
} GnomeStockPixmapType;


/* a data entry holds a hardcoded pixmap */
struct _GnomeStockPixmapEntryData {
        GnomeStockPixmapType type;
	int width, height;
	char *label;
        gchar **xpm_data;
};

/* a data entry holds a hardcoded pixmap */
typedef struct _GnomeStockPixmapEntryImlib   GnomeStockPixmapEntryImlib;
struct _GnomeStockPixmapEntryImlib {
        GnomeStockPixmapType type;
	int width, height;
	char *label;
        gchar *rgb_data;
	GdkImlibColor shape;
};

/* a scalable version */
typedef struct _GnomeStockPixmapEntryImlibScaled GnomeStockPixmapEntryImlibScaled;
struct _GnomeStockPixmapEntryImlibScaled {
        GnomeStockPixmapType type;
	int width, height;
	char *label;
        gchar *rgb_data;
	GdkImlibColor shape;
	int scaled_width, scaled_height;
};

/* a file entry holds a filename (no path) to the pixamp. this pixmap
   will be seached for using gnome_pixmap_file */
struct _GnomeStockPixmapEntryFile {
        GnomeStockPixmapType type;
	int width, height;
	char *label;
        gchar *filename;
};

/* a path entry holds the complete (absolut) path to the pixmap file */
struct _GnomeStockPixmapEntryPath {
        GnomeStockPixmapType type;
	int width, height;
	char *label;
        gchar *pathname;
};

/* a widget entry holds a GnomeStockPixmapWidget. This kind of icon can be
 * used by a theme to completely change the handling of a stock icon. */
struct _GnomeStockPixmapEntryWidget {
        GnomeStockPixmapType type;
	int width, height;
	char *label;
        GtkWidget *widget;
};

/* a GnomePixmap */
struct _GnomeStockPixmapEntryGPixmap {
        GnomeStockPixmapType type;
	int width, height;
	char *label;
        GnomePixmap *pixmap;
};

struct _GnomeStockPixmapEntryAny {
        GnomeStockPixmapType type;
	int width, height;
	char *label;
};

union _GnomeStockPixmapEntry {
        GnomeStockPixmapType type;
        GnomeStockPixmapEntryAny any;
        GnomeStockPixmapEntryData data;
        GnomeStockPixmapEntryFile file;
        GnomeStockPixmapEntryPath path;
        GnomeStockPixmapEntryWidget widget;
	GnomeStockPixmapEntryImlib imlib;
	GnomeStockPixmapEntryImlibScaled imlib_s;
        GnomeStockPixmapEntryGPixmap gpixmap;
};



/* the GnomeStockPixmapWidget */

#define GNOME_STOCK_PIXMAP_WIDGET(obj)         GTK_CHECK_CAST(obj, gnome_stock_pixmap_widget_get_type(), GnomeStockPixmapWidget)
#define GNOME_STOCK_PIXMAP_WIDGET_CLASS(klass) GTK_CHECK_CAST_CLASS(obj, gnome_stock_pixmap_widget_get_type(), GnomeStockPixmapWidget)
#define GNOME_IS_STOCK_PIXMAP_WIDGET(obj)      GTK_CHECK_TYPE(obj, gnome_stock_pixmap_widget_get_type())

typedef struct _GnomeStockPixmapWidget         GnomeStockPixmapWidget;
typedef struct _GnomeStockPixmapWidgetClass    GnomeStockPixmapWidgetClass;

struct _GnomeStockPixmapWidget {
	GtkVBox parent_object;

        char *icon;
	int width, height;      /* needed to answer size_requests even before
			         * a pixmap is loaded/created */
        GtkWidget   *window;    /* needed for style and gdk_pixmap_create... */
        GnomePixmap *pixmap;    /* the pixmap currently shown */
        GnomePixmap *regular, *disabled, *focused;  /* pixmap cache */
};

struct _GnomeStockPixmapWidgetClass {
	GtkVBoxClass parent_class;
};

guint gnome_stock_pixmap_widget_get_type(void);
GtkWidget *gnome_stock_pixmap_widget_new(GtkWidget *window, const char *icon);



/* the utility functions */

/* just fetch a pixmap */
/* window isn't needed for pixmap creation but for the style
 * when a disabled icon is automatically created */
/* okay, since there are many problems with this function (realization issues),
 * don't use it. Use gnome_stock_pixmap_widget instead -- it's far more save and
 * the result is the same */
#if 0
GnomePixmap           *gnome_stock_pixmap          (GtkWidget *window,
                                                    const char *icon,
                                                    const char *subtype);
#endif

/* just fetch a GnomeStockPixmapWidget */
GtkWidget             *gnome_stock_pixmap_widget   (GtkWidget *window,
                                                    const char *icon);

/* change the icon/look of a GnomeStockPixmapWidget */
void gnome_stock_pixmap_widget_set_icon(GnomeStockPixmapWidget *widget,
					const char *icon);

/* register a pixmap. returns non-zero, if successful */
gint                   gnome_stock_pixmap_register (const char *icon,
						    const char *subtype,
                                                    GnomeStockPixmapEntry *entry);

/* change an existing entry. returns non-zero on success */
gint                   gnome_stock_pixmap_change   (const char *icon,
						    const char *subtype,
                                                    GnomeStockPixmapEntry *entry);

/* check for the existance of an entry. returns the entry if it
   exists, or NULL otherwise */
GnomeStockPixmapEntry *gnome_stock_pixmap_checkfor (const char *icon,
						    const char *subtype);



/*  buttons  */

/* this function returns a button with a pixmap (if ButtonUseIcons is enabled)
 * and the provided text */

GtkWidget            *gnome_pixmap_button         (GtkWidget *pixmap,
						   const char *text);
void		      gnome_button_can_default    (GtkButton *button,
						   gboolean can_default);

#define GNOME_STOCK_BUTTON_OK     "Button_Ok"
#define GNOME_STOCK_BUTTON_CANCEL "Button_Cancel"
#define GNOME_STOCK_BUTTON_YES    "Button_Yes"
#define GNOME_STOCK_BUTTON_NO     "Button_No"
#define GNOME_STOCK_BUTTON_CLOSE  "Button_Close"
#define GNOME_STOCK_BUTTON_APPLY  "Button_Apply"
#define GNOME_STOCK_BUTTON_HELP   "Button_Help"

/* returns a default button widget for dialogs */
GtkWidget             *gnome_stock_button          (const char *type);

/* Returns a button widget.  If the TYPE argument matches a
   GNOME_STOCK_BUTTON_* define, then a stock button is created.
   Otherwise, an ordinary button is created, and TYPE is given as the
   label.  */
GtkWidget             *gnome_stock_or_ordinary_button (const char *type);


/*  menus  */

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
#define GNOME_STOCK_MENU_SPELLCHECK   "Menu_Spellchecker"
#define GNOME_STOCK_MENU_MIC          "Menu_Microphone"
#define GNOME_STOCK_MENU_LINE_IN      "Menu_Line In"
#define GNOME_STOCK_MENU_VOLUME       "Menu_Volume"
#define GNOME_STOCK_MENU_BOOK_RED     "Menu_Book Red"
#define GNOME_STOCK_MENU_BOOK_GREEN   "Menu_Book Green"
#define GNOME_STOCK_MENU_BOOK_BLUE    "Menu_Book Blue"
#define GNOME_STOCK_MENU_BOOK_YELLOW  "Menu_Book Yellow"
#define GNOME_STOCK_MENU_BOOK_OPEN    "Menu_Book Open"

#define GNOME_STOCK_MENU_EXIT     GNOME_STOCK_MENU_QUIT


/* returns a GtkMenuItem with an stock icon and text */
GtkWidget             *gnome_stock_menu_item       (const char *type,
						    const char *text);


/*
 * Stock menu accelerators
 */

/* To customize the accelerators add a file ~/.gnome/GnomeStock, wich looks
 * like that:
 *
 * [Accelerators]
 * Menu_New=Shft+Ctl+N
 * Menu_About=Ctl+A
 * Menu_Save As=Ctl+Shft+S
 * Menu_Quit=Alt+X
 */

/* this function returns the stock menu accelerators for the menu type in key
 * and mod */
gboolean	       gnome_stock_menu_accel      (const char *type,
						    guchar *key,
						    guint8 *mod);

/* apps can call this function at startup to add per app accelerator
 * redefinitions. section should be something like "/filename/section/" with
 * both the leading and trailing `/' */
void                   gnome_stock_menu_accel_parse(const char *section);

/*
 * Creates a toplevel window with a shaped mask.  Useful for making the DnD
 * windows
 */
GtkWidget *gnome_stock_transparent_window (const char *icon, const char *subtype);

END_GNOME_DECLS

#endif /* GNOME_STOCK_H */
