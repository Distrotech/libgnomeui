/* GnomeApp widget */

#ifndef GNOME_APP_H
#define GNOME_APP_H

#include <gtk/gtk.h>

BEGIN_GNOME_DECLS

#define GNOME_APP(obj) GTK_CHECK_CAST(obj, gnome_app_get_type(), GnomeApp)
#define GNOME_APP_CLASS(class) GTK_CHECK_CAST_CLASS(class, gnome_app_get_type(), GnomeAppClass)
#define GNOME_IS_APP(obj) GTK_CHECK_TYPE(obj, gnome_app_get_type())

typedef struct _GnomeApp GnomeApp;
typedef struct _GnomeAppClass GnomeAppClass;        

typedef enum 
{
  POS_NOCHANGE = -1,
  POS_TOP = 0,
  POS_BOTTOM = 1,
  POS_LEFT = 2,
  POS_RIGHT = 3,
} GnomeAppWidgetPositionType;

typedef enum
{
  GAPP_MENUBAR = 1,
  GAPP_TOOLBAR = 2,
} GnomeAppWidgetType;

struct _GnomeMenuInfo {
  enum { MI_ENDOFINFO, MI_ITEM, MI_SUBMENU } type;
  gchar *label;
  gpointer moreinfo; /* For a menuitem, this should point to the
		        procedure to be called when this menu item is
		        activated.

		        For a submenu, it should point to the
		        GnomeMenuInfo array for that menu. */
  GtkWidget *widget; /* This is filled in by gnome_app_create_menu() */
};

typedef struct _GnomeMenuInfo GnomeMenuInfo;

struct _GnomeToolbarInfo {
  enum { TBI_ENDOFINFO, TBI_ITEM, TBI_SPACE } type;
  /* You can leave the rest of these to NULL or whatever if this is an
     TBI_SPACE */
  gchar *text;
  gchar *tooltip_text;
  enum { PMAP_NONE, PMAP_DATA, PMAP_FILENAME } pixmap_type;
  gpointer pixmap_info; /* Either a pointer to the char ** for the pixmap
			   (for PMAP_DATA)
			   or a char * for the filename (PMAP_FILENAME) */
  GtkSignalFunc clicked_callback; /* Useful for TB_ITEMs only */ 
};
typedef struct _GnomeToolbarInfo GnomeToolbarInfo;

/* Everything gets put into a table that looks like:

   XXX
   ABC
   YYY

   There's one table element on top, three in the middle, and one on
   the bottom.

   Obviously you can change the positions of things as needed
   using the supplied function.
 */
struct _GnomeApp {
  GtkWindow parent_object;
  GtkWidget *menubar /* GtkMenuBar */,
    *toolbar /* GtkToolbar */,
    *contents /* GtkContainer */;
  GtkWidget *table; /* The table widgets that hold & pack it all */
  GnomeAppWidgetPositionType pos_menubar, pos_toolbar;
};

struct _GnomeAppClass {
  GtkWindow parent_class;
};

guint gnome_app_get_type    (void);
GtkWidget *gnome_app_new    (gchar *appname);
/* You can OR the various enums here to have more than one piece
   created at once */
void gnome_app_create_menu   (GnomeApp *app,
			      GnomeMenuInfo *menuinfo);
void gnome_app_create_toolbar(GnomeApp *app,
			      GnomeToolbarInfo *tbinfo);
void gnome_app_set_contents  (GnomeApp *app, GtkWidget *contents);
void gnome_app_set_positions (GnomeApp *app,
			      GnomeAppWidgetPositionType pos_menubar,
			      GnomeAppWidgetPositionType pos_toolbar);

END_GNOME_DECLS

#endif /* GNOME_APP_H */
