/* GnomeApp widget (C) 1998 Red Hat Software, Miguel de Icaza, Federico Mena, 
 * Chris Toshok.
 *
 * Originally by Elliot Lee, with hacking by Chris Toshok for *_data, 
 * Marc Ewing added menu support, toggle and radio support, and I don't know 
 * what you other people did :) menu insertion/removal functions by Jaka Mocnik.
 * Some subtree hackage (and possibly various justification hacks) by Justin 
 * Maurer.
 *
 * Major cleanups and rearrangements by Federico Mena and Justin Maurer.
 */

#ifndef GNOME_APP_HELPER_H
#define GNOME_APP_HELPER_H

#include <gtk/gtkstatusbar.h>
#include <libgnome/gnome-defs.h>
#include "gnome-appbar.h"

BEGIN_GNOME_DECLS

/* This module lets you easily create menus and toolbars for your 
 * applications. You basically define a hierarchy of arrays of GnomeUIInfo 
 * structures, and you later call the provided functions to create menu bars 
 * or tool bars.
 */

/* These values identify the item type that a particular GnomeUIInfo structure 
 * specifies */
typedef enum {
	GNOME_APP_UI_ENDOFINFO,		/* No more items, use it at the end of 
					   an array */
	GNOME_APP_UI_ITEM,		/* Normal item, or radio item if it is 
					   inside a radioitems group */
	GNOME_APP_UI_TOGGLEITEM,	/* Toggle (check box) item */
	GNOME_APP_UI_RADIOITEMS,	/* Radio item group */
	GNOME_APP_UI_SUBTREE,		/* Item that defines a 
					   subtree/submenu */
	GNOME_APP_UI_SEPARATOR,		/* Separator line (menus) or blank 
					   space (toolbars) */
	GNOME_APP_UI_HELP,		/* Create a list of help topics, 
					   used in the Help menu */
	GNOME_APP_UI_BUILDER_DATA	/* Specifies the builder data for the 
					   following entries, see code for 
					   further info */
	/* one should be careful when using 
	 * gnome_app_create_*_[custom|interp|with_data]() functions with 
	 * GnomeUIInfo arrays containing GNOME_APP_UI_BUILDER_DATA items since 
	 * their GnomeUIBuilderData structures completely override the ones 
	 * generated or supplied by the above functions. */
} GnomeUIInfoType;

/* These values identify the type of pixmap used in an item */
typedef enum {
	GNOME_APP_PIXMAP_NONE,		/* No pixmap specified */
	GNOME_APP_PIXMAP_STOCK,		/* Use a stock pixmap (GnomeStock) */
	GNOME_APP_PIXMAP_DATA,		/* Use a pixmap from inline xpm data */
	GNOME_APP_PIXMAP_FILENAME	/* Use a pixmap from the specified 
					   filename */
} GnomeUIPixmapType;

/* This is the structure that defines an item in a menu bar or toolbar.  The 
 * idea is to create an array of such structures with the information needed 
 * to create menus or toolbars.  The most convenient way to create such a 
 * structure is to use the GNOMEUIINFO_* macros provided below. */
typedef struct {
	GnomeUIInfoType type;		/* Type of item */
	gchar *label;			/* String to use in the label */
	gchar *hint;			/* For toolbar items, the tooltip. For 
					   menu items, the status bar message */
	gpointer moreinfo;		/* For an item, toggleitem, or 
					   radioitem, this is a pointer to the 
					   function to call when the item is 
					   activated. For a subtree, a pointer 
					   to another array of GnomeUIInfo 
					   structures. For a radioitem lead 
					   entry, a pointer to an array of 
					   GnomeUIInfo structures for the radio 
					   item group. For a help item, 
					   specifies the help node to load 
					   (i.e. the application's identifier) 
					   or NULL for the main program's name.
					   For builder data, points to the 
					   GnomeUIBuilderData structure for 
					   the following items */
	gpointer user_data;		/* Data pointer to pass to callbacks */
	gpointer unused_data;		/* Reserved for future expansion, 
					   should be NULL */
	GnomeUIPixmapType pixmap_type;	/* Type of pixmap for the item */
	gpointer pixmap_info;		/* Pointer to the pixmap information:
					 *
					 * For GNOME_APP_PIXMAP_STOCK, a 
					 * pointer to the stock icon name.
					 *
					 * For GNOME_APP_PIXMAP_DATA, a 
					 * pointer to the inline xpm data.
					 *
					 * For GNOME_APP_PIXMAP_FILENAME, a 
					 * pointer to the filename string.
					 */
	guint accelerator_key;		/* Accelerator key, or 0 for none */
	GdkModifierType ac_mods;	/* Mask of modifier keys for the 
					   accelerator */

	GtkWidget *widget;		/* Filled in by gnome_app_create*, you 
					   can use this to tweak the widgets 
					   once they have been created */
} GnomeUIInfo;

/* Callback data */

#define GNOMEUIINFO_KEY_UIDATA		"uidata"
#define GNOMEUIINFO_KEY_UIBDATA		"uibdata"

/* Handy GnomeUIInfo macros */

/* Used to terminate an array of GnomeUIInfo structures */
#define GNOMEUIINFO_END			{ GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL, NULL, NULL,		\
					  (GnomeUIPixmapType) 0, NULL, 0, (GdkModifierType) 0, NULL }

/* Insert a separator line (on a menu) or a blank space (on a toolbar) */
#define GNOMEUIINFO_SEPARATOR		{ GNOME_APP_UI_SEPARATOR, NULL, NULL, NULL, NULL, NULL,		\
					  (GnomeUIPixmapType) 0, NULL, 0, (GdkModifierType) 0, NULL }

/* Insert an item with an inline xpm icon */
#define GNOMEUIINFO_ITEM(label, tooltip, callback, xpm_data) \
	{ GNOME_APP_UI_ITEM, label, tooltip, callback, NULL, NULL, \
		GNOME_APP_PIXMAP_DATA, xpm_data, 0, (GdkModifierType) 0, NULL}

/* Insert an item with a stock icon */
#define GNOMEUIINFO_ITEM_STOCK(label, tooltip, callback, stock_id) \
	{ GNOME_APP_UI_ITEM, label, tooltip, callback, NULL, NULL, \
		GNOME_APP_PIXMAP_STOCK, stock_id, 0, (GdkModifierType) 0, NULL }

/* Insert an item with no icon */
#define GNOMEUIINFO_ITEM_NONE(label, tooltip, callback) \
	{ GNOME_APP_UI_ITEM, label, tooltip, callback, NULL, NULL, \
		GNOME_APP_PIXMAP_NONE, NULL, 0, (GdkModifierType) 0, NULL }

/* Insert an item with an inline xpm icon and a user data pointer */
#define GNOMEUIINFO_ITEM_DATA(label, tooltip, callback, user_data, xpm_data) \
	{ GNOME_APP_UI_ITEM, label, tooltip, callback, user_data, NULL, \
		GNOME_APP_PIXMAP_DATA, xpm_data, 0, (GdkModifierType) 0, NULL }

/* Insert a toggle item (check box) with an inline xpm icon */
#define GNOMEUIINFO_TOGGLEITEM(label, tooltip, callback, xpm_data) \
	{ GNOME_APP_UI_TOGGLEITEM, label, tooltip, callback, NULL, NULL, \
		GNOME_APP_PIXMAP_DATA, xpm_data, 0, (GdkModifierType) 0, NULL }

/* Insert a toggle item (check box) with an inline xpm icon and a user data pointer */
#define GNOMEUIINFO_TOGGLEITEM_DATA(label, tooltip, callback, user_data, xpm_data)		\
					{ GNOME_APP_UI_TOGGLEITEM, label, tooltip, callback,	\
					  user_data, NULL, GNOME_APP_PIXMAP_DATA, xpm_data,	\
					  0, (GdkModifierType) 0, NULL }

/* Insert all the help topics based on the application's id */
#define GNOMEUIINFO_HELP(app_name) \
	{ GNOME_APP_UI_HELP, NULL, NULL, app_name, NULL, NULL, \
		(GnomeUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }

/* Insert a subtree (submenu) */
#define GNOMEUIINFO_SUBTREE(label, tree) \
	{ GNOME_APP_UI_SUBTREE, label, NULL, tree, NULL, NULL, \
		(GnomeUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }

/* Insert a subtree (submenu) with a stock icon */
#define GNOMEUIINFO_SUBTREE_STOCK(label, tree, stock_id) \
	{ GNOME_APP_UI_SUBTREE, label, NULL, tree, NULL, NULL, \
		GNOME_APP_PIXMAP_STOCK, stock_id, 0, (GdkModifierType) 0, NULL }

/* Insert a list of radio items */
#define GNOMEUIINFO_RADIOLIST(list) \
	{ GNOME_APP_UI_RADIOITEMS, NULL, NULL, list, NULL, NULL, \
		(GnomeUIPixmapType) 0, NULL, 0,	(GdkModifierType) 0, NULL }

/* Insert a radio item with an inline xpm icon */
#define GNOMEUIINFO_RADIOITEM(label, tooltip, callback, xpm_data) \
	{ GNOME_APP_UI_RADIOITEMS, label, tooltip, callback, NULL, NULL, \
		GNOME_APP_PIXMAP_DATA, xpm, 0, (GdkModifierType) 0, NULL }

/* Insert a radio item with an inline xpm icon and a user data pointer */
#define GNOMEUIINFO_RADIOITEM_DATA(label, tooltip, callback, user_data, xpm_data)		\
					{ GNOME_APP_UI_RADIOITEMS, label, tooltip, callback,	\
					  user_data, NULL, GNOME_APP_PIXMAP_DATA, xpm_data,	\
					  0, (GdkModifierType) 0, NULL }


/* Types useful to language bindings */
    
typedef struct _GnomeUIBuilderData GnomeUIBuilderData;

typedef void (* GnomeUISignalConnectFunc) (GnomeUIInfo        *uiinfo,
					   gchar              *signal_name,
					   GnomeUIBuilderData *uibdata);

struct _GnomeUIBuilderData {
	GnomeUISignalConnectFunc connect_func;	/* Function that connects to the item's signals */
	gpointer data;				/* User data pointer */
	gboolean is_interp;			/* Should use gtk_signal_connect_interp or normal gtk_signal_connect? */
	GtkCallbackMarshal relay_func;		/* Marshaller function for language bindings */
	GtkDestroyNotify destroy_func;		/* Destroy notification function for language bindings */
};


/* Fills the specified menu shell with items created from the specified
 * info, inserting them from the item no. pos on.  If the specified
 * accelgroup is not NULL, then the menu's hotkeys are put into that
 * accelgroup.  If accel_group is non-NULL and insert_shortcuts is
 * TRUE, then the shortcut keys (MOD1 + underlined letters) in the
 * items' labels will be put into the accel group as well.
 */
void gnome_app_fill_menu (GtkMenuShell *menu_shell, GnomeUIInfo *uiinfo,
			  GtkAccelGroup *accel_group,
			  gboolean insert_shortcuts, gint pos);

/* Fills the specified menu shell with items created from the specified
 * info, inserting them from item no. pos on and using the specified
 * builder data -- this is intended for language bindings.  If the
 * specified accelgroup is not NULL, then the menu's hotkeys are put
 * into that accelgroup.  If accel_group is non-NULL and
 * insert_shortcuts is TRUE, then the shortcut keys (MOD1 + underlined
 * letters) in the items' labels will be put into the accel group as
 * well (this is useful for toplevel menu bars in which you want MOD1-F
 * to activate the "_File" menu, for example).
 */
void gnome_app_fill_menu_custom (GtkMenuShell *menu_shell, GnomeUIInfo *uiinfo,
				 GnomeUIBuilderData *uibdata,
				 GtkAccelGroup *accel_group,
				 gboolean insert_shortcuts, gint pos);

/* Constructs a menu bar and attaches it to the specified application window */
void gnome_app_create_menus (GnomeApp *app, GnomeUIInfo *uiinfo);

/* Constructs a menu bar and attaches it to the specified application window -- this version is
 * intended for language bindings.
 */
void gnome_app_create_menus_interp (GnomeApp *app, GnomeUIInfo *uiinfo,
				    GtkCallbackMarshal relay_func, gpointer data,
				    GtkDestroyNotify destroy_func);

/* Constructs a menu bar, sets all the user data pointers to the specified value, and attaches it to
 * the specified application window.
 */
void gnome_app_create_menus_with_data (GnomeApp *app, GnomeUIInfo *uiinfo, gpointer user_data);

/* Constructs a menu bar and attaches it to the specified application window, using the
 * specified builder data -- intended for language bindings.
 */
void gnome_app_create_menus_custom (GnomeApp *app, GnomeUIInfo *uiinfo, GnomeUIBuilderData *uibdata);

/* Fills the specified toolbar with buttons created from the specified info.  If accel_group is not
 * NULL, then the items' accelerator keys are put into it.
 */
void gnome_app_fill_toolbar (GtkToolbar *toolbar, GnomeUIInfo *uiinfo, GtkAccelGroup *accel_group);

/* Fills the specified toolbar with buttons created from the specified info, using the specified
 * builder data -- this is intended for language bindings.  If accel_group is not NULL, then the
 * items' accelerator keys are put into it.
 */
void gnome_app_fill_toolbar_custom (GtkToolbar *toolbar, GnomeUIInfo *uiinfo, GnomeUIBuilderData *uibdata,
				    GtkAccelGroup *accel_group);

/* Configures the toolbar, changing any separators to the proper type if the orientation
 * has changed, for example.
 */
void gnome_app_configure_toolbar (GtkToolbar *toolbar);

/* Constructs a toolbar and attaches it to the specified application window */
void gnome_app_create_toolbar (GnomeApp *app, GnomeUIInfo *uiinfo);

/* Constructs a toolbar and attaches it to the specified application window -- this version is
 * intended for language bindings.
 */
void gnome_app_create_toolbar_interp (GnomeApp *app, GnomeUIInfo *uiinfo,
				      GtkCallbackMarshal relay_func, gpointer data,
				      GtkDestroyNotify destroy_func);

/* Constructs a toolbar, sets all the user data pointers to the specified value, and attaches it to
 * the specified application window.
 */
void gnome_app_create_toolbar_with_data (GnomeApp *app, GnomeUIInfo *uiinfo, gpointer user_data);

/* Constructs a toolbar and attaches it to the specified application window, using the specified
 * builder data -- intended for language bindings.
 */
void gnome_app_create_toolbar_custom (GnomeApp *app, GnomeUIInfo *uiinfo, GnomeUIBuilderData *uibdata);

/* finds menu item described by path (see below for details) starting in the GtkMenuShell top
 * and returns its parent GtkMenuShell and the position after this item in pos:
 * gtk_menu_shell_insert(p, w, pos) would then insert widget w in GtkMenuShell p right after
 * the menu item described by path.
 * the path argument should be in the form "File/.../.../Something".
 * "" will insert the item as the first one in the menubar
 * "File/" will insert it as the first one in the File menu
 * "File/Settings" will insert it after the Setting item in the File menu
 * use of  "File/<Separator>" should be obvious. however this stops after the first separator.
 */
GtkWidget *gnome_app_find_menu_pos (GtkWidget *parent, gchar *path, gint *pos);

/* removes num items from the existing app's menu structure begining with item described
 * by path
 */
void gnome_app_remove_menus (GnomeApp *app, gchar *path, gint items);

/* Same as the above, except it removes the specified number of items 
 * from the existing app's menu structure begining with item described by path,
 * plus the number specified by start - very useful for adding and removing Recent
 * document items in the File menu.
 */
void gnome_app_remove_menu_range (GnomeApp *app, gchar *path, gint start, gint items);

/* inserts menus described by uiinfo in existing app's menu structure right after the item described by path.
 */
void gnome_app_insert_menus_custom (GnomeApp *app, gchar *path, GnomeUIInfo *menuinfo, GnomeUIBuilderData *uibdata);

/* FIXME: what does it do? */
void gnome_app_insert_menus (GnomeApp *app, gchar *path, GnomeUIInfo *menuinfo);

/* FIXME: what does it do? */
void gnome_app_insert_menus_with_data (GnomeApp *app, gchar *path, GnomeUIInfo *menuinfo, gpointer data);

/* FIXME: what does it do? */
void gnome_app_insert_menus_interp (GnomeApp *app, gchar *path, GnomeUIInfo *menuinfo,
				    GtkCallbackMarshal relay_func, gpointer data,
				    GtkDestroyNotify destroy_func);


/* Activate the menu item hints, displaying in the given appbar.
   This can't be automatic since we can't reliably find the 
   appbar. */
void gnome_app_install_appbar_menu_hints    (GnomeAppBar* appbar,
                                             GnomeUIInfo* uiinfo);

void gnome_app_install_statusbar_menu_hints (GtkStatusbar* bar,
                                             GnomeUIInfo* uiinfo);

END_GNOME_DECLS

#endif
