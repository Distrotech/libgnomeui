#ifndef __GNOME_PIXMAP_H__
#define __GNOME_PIXMAP_H__


#include <libgnome/gnome-defs.h>
#include <gtk/gtkwidget.h>
#include <gtk/gtkpixmap.h>
#include <gtk/gtkvbox.h>


/* A short description:

   These functions provide an applications programmer with default
   icons for toolbars, menu pixmaps, etc. One such `icon' should have
   at least three pixmaps to reflect it's state. There is a `regular'
   pixmap, a `disabled' pixmap and a `focused' pixmap. You can get
   either each of these pixmaps by calling gnome_stock_pixmap or you
   can get a widget by calling gnome_stock_pixmap_widget. This widget
   is a container which gtk_widget_shows the pixmap, that is
   reflecting the current state of the widget. If for example you
   gtk_container_add this widget to a button, which is currently not
   sensitive, the widget will just show the `disabled' pixmap. If the
   state of the button changes to sensitive, the widget will change to
   the `regular' pixmap. The `focused' pixmap will be shown, when the
   mouse pointer enters the widget.

   To support themability, we use (char *) to call those functions. A
   new theme might register new icons by calling
   gnome_stock_pixmap_register, or may change existing icons by
   calling gnome_stock_pixmap_change. An application should check (by
   calling gnome_stock_pixmap_checkfor), if the current theme supports
   an uncommon icon, before using it. The only icons an app can rely
   on, are those defined in this haeder file. */


BEGIN_GNOME_DECLS

/* The names of `well known' icons. I define these strings basically
   to prevent errors due to typos. */

#define GNOME_STOCK_PIXMAP_NEW         "New"
#define GNOME_STOCK_PIXMAP_OPEN        "Open"
#define GNOME_STOCK_PIXMAP_SAVE        "Save"
#define GNOME_STOCK_PIXMAP_CUT         "Cut"
#define GNOME_STOCK_PIXMAP_COPY        "Copy"
#define GNOME_STOCK_PIXMAP_PASTE       "Paste"
#define GNOME_STOCK_PIXMAP_PROPERTIES  "Properties"


/* The basic pixmap version of an icon. */

#define GNOME_STOCK_PIXMAP_REGULAR     "regular"
#define GNOME_STOCK_PIXMAP_DISABLED    "disabled"
#define GNOME_STOCK_PIXMAP_FOCUSED     "focused"



/* some internal definitions */

typedef struct _GnomeStockPixmapEntryData    GnomeStockPixmapEntryData;
typedef struct _GnomeStockPixmapEntryFile    GnomeStockPixmapEntryFile;
typedef struct _GnomeStockPixmapEntryPath    GnomeStockPixmapEntryPath;
typedef struct _GnomeStockPixmapEntryWidget  GnomeStockPixmapEntryWidget;
typedef union  _GnomeStockPixmapEntry        GnomeStockPixmapEntry;

typedef enum {
        GNOME_STOCK_PIXMAP_TYPE_NONE,
        GNOME_STOCK_PIXMAP_TYPE_DATA,
        GNOME_STOCK_PIXMAP_TYPE_FILE,
        GNOME_STOCK_PIXMAP_TYPE_PATH,
        GNOME_STOCK_PIXMAP_TYPE_WIDGET
} GnomeStockPixmapType;


/* a data entry holds a hardcoded pixmap */
struct _GnomeStockPixmapEntryData {
        GnomeStockPixmapType type;
        gchar **xpm_data;
};

/* a file entry holds a filename (no path) to the pixamp. this pixmap
   will be seached for using gnome_pixmap_file */
struct _GnomeStockPixmapEntryFile {
        GnomeStockPixmapType type;
        gchar *filename;
};

/* a path entry holds the complete (absolut) path to the pixmap file */
struct _GnomeStockPixmapEntryPath {
        GnomeStockPixmapType type;
        gchar *pathname;
};

/* a widget entry holds a GnomeStockPixmapWidget. This kind of icon
   can be used by a theme to completely change the handling of a stock
   icon. */
struct _GnomeStockPixmapEntryWidget {
        GnomeStockPixmapType type;
        GtkWidget *widget;
};

union _GnomeStockPixmapEntry {
        GnomeStockPixmapType type;
        GnomeStockPixmapEntryData data;
        GnomeStockPixmapEntryFile file;
        GnomeStockPixmapEntryPath path;
        GnomeStockPixmapEntryWidget widget;
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
        GtkWidget *window;    /* needed for style and gdk_pixmap_create... */
        GtkPixmap *pixmap;    /* the pixmap currently shown */
};

struct _GnomeStockPixmapWidgetClass {
	GtkVBoxClass parent_class;
};

guint gnome_stock_pixmap_widget_get_type(void);
GtkWidget *gnome_stock_pixmap_widget_new(GtkWidget *window, char *icon);



/* the utility functions */

/* just fetch a pixmap */
GtkPixmap             *gnome_stock_pixmap          (GtkWidget *window,
                                                    char *icon,
                                                    char *subtype);

/* just fetch a GnomeStockPixmapWidget */
GtkWidget             *gnome_stock_pixmap_widget   (GtkWidget *window,
                                                    char *icon);

/* register a pixmap. returns non-zero, if successfull */
gint                   gnome_stock_pixmap_register (char *icon, char *subtype,
                                                    GnomeStockPixmapEntry *entry);

/* change an existing entry. returns non-zero on success */
gint                   gnome_stock_pixmap_change   (char *icon, char *subtype,
                                                    GnomeStockPixmapEntry *entry);

/* check for the existance of an entry. returns the entry if it
   exists, or NULL otherwise */
GnomeStockPixmapEntry *gnome_stock_pixmap_checkfor (char *icon, char *subtype);

END_GNOME_DECLS

#endif
