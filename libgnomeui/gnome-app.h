/* GnomeApp widget, by Elliot Lee <sopwith@cuc.edu> */

#ifndef GNOME_APP_H
#define GNOME_APP_H

#include <gtk/gtk.h>

BEGIN_GNOME_DECLS

#define GNOME_APP(obj)         GTK_CHECK_CAST(obj, gnome_app_get_type(), GnomeApp)
#define GNOME_APP_CLASS(class) GTK_CHECK_CAST_CLASS(class, gnome_app_get_type(), GnomeAppClass)
#define GNOME_IS_APP(obj)      GTK_CHECK_TYPE(obj, gnome_app_get_type())

typedef struct _GnomeApp       GnomeApp;
typedef struct _GnomeAppClass  GnomeAppClass;

typedef enum 
{
	GNOME_APP_POS_TOP,
	GNOME_APP_POS_BOTTOM,
	GNOME_APP_POS_LEFT,
	GNOME_APP_POS_RIGHT,
	GNOME_APP_POS_FLOATING,
} GnomeAppWidgetPositionType;

typedef enum
{
	GNOME_APP_MENUBAR = 1,
	GNOME_APP_TOOLBAR = 2,
} GnomeAppWidgetType;

struct _GnomeMenuInfo {
	enum {
		GNOME_APP_MENU_ENDOFINFO,
		GNOME_APP_MENU_ITEM,
		GNOME_APP_MENU_SUBMENU
	} type;
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
	enum {
		GNOME_APP_TOOLBAR_ENDOFINFO,
		GNOME_APP_TOOLBAR_ITEM,
		GNOME_APP_TOOLBAR_SPACE
	} type;
	
	/* You can leave the rest of these to NULL or whatever if this is an
	 * GNOME_APP_TOOLBAR_SPACE
	 */
	
	gchar *text;
	gchar *tooltip_text;
	enum {
		GNOME_APP_PIXMAP_NONE,
		GNOME_APP_PIXMAP_DATA,
		GNOME_APP_PIXMAP_FILENAME
	} pixmap_type;
	
	/* Either a pointer to the char 
	 *  for the pixmap
	 * (for PMAP_DATA) or a char * for the filename
	 * (PMAP_FILENAME)
	 */
	gpointer pixmap_info;

	/* Useful for TB_ITEMs only,
	 *  it's the GtkSignalFunc
	 */
	gpointer clicked_callback; 
};
typedef struct _GnomeToolbarInfo GnomeToolbarInfo;

/* Everything gets put into a table that looks like:
 *
 * XXX
 * ABC
 * YYY
 *
 * There's one table element on top, three in the middle, and one on
 * the bottom.
 *
 * Obviously you can change the positions of things as needed
 * using the supplied function.
 */
struct _GnomeApp {
	GtkWindow parent_object;
	char     *name;		/* Application name */
	char     *prefix;	/* gnome config prefix */
	GtkWidget *menubar;	/* The MenuBar */
	GtkWidget *toolbar;	/* The Toolbar */
	GtkWidget *contents;	/* The contents */
	GtkWidget *table;	/* The table widget that ties them all */
	
	/* Positions for the menubar and the toolbar */
	GnomeAppWidgetPositionType pos_menubar, pos_toolbar;
};

struct _GnomeAppClass {
	GtkWindowClass parent_class;
};

guint gnome_app_get_type      (void);
GtkWidget *gnome_app_new      (gchar *appname, char *title);

void gnome_app_create_menus         (GnomeApp *app,
			             GnomeMenuInfo *menuinfo);
void gnome_app_create_toolbar       (GnomeApp *app,
			             GnomeToolbarInfo *tbinfo);
void gnome_app_set_menus            (GnomeApp *app,
			             GtkMenuBar *menubar);
void gnome_app_set_toolbar          (GnomeApp *app,
			             GtkToolbar *toolbar);
void gnome_app_set_contents         (GnomeApp *app,
			             GtkWidget *contents);
void gnome_app_toolbar_set_position (GnomeApp *app,
				     GnomeAppWidgetPositionType pos_toolbar);

void gnome_app_menu_set_position    (GnomeApp *app,
				     GnomeAppWidgetPositionType pos_menu);

END_GNOME_DECLS

#endif /* GNOME_APP_H */
