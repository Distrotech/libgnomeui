#ifndef GNOME_APP_HELPER_H
#define GNOME_APP_HELPER_H

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

/*
 * Helper routines,
 */

typedef enum {
  GNOME_APP_UI_ENDOFINFO,
  GNOME_APP_UI_ITEM,
  GNOME_APP_UI_TOGGLEITEM, /* check item for menu - no toolbar support */
  GNOME_APP_UI_RADIOITEMS, /* no toolbar support */
  GNOME_APP_UI_SUBTREE,
  GNOME_APP_UI_SEPARATOR,
  GNOME_APP_UI_HELP,
  GNOME_APP_UI_JUSTIFY_RIGHT, /* this should right justify all the following entries */
  GNOME_APP_UI_BUILDER_DATA   /* specifies the builder data for the following entries, see code for further info */
  /* one should be careful when using gnome_app_create_*_[custom|interp|with_data]() functions
     with GnomeUIInfo arrays containing GNOME_APP_UI_BUILDER_DATA items since their GnomeUIBuilderData
     structures completely override the ones generated or supplied by the above functions. */
} GnomeUIInfoType;

typedef enum {
  GNOME_APP_PIXMAP_NONE,
  GNOME_APP_PIXMAP_STOCK,
  GNOME_APP_PIXMAP_DATA, /* Can't currently use these last two in menus */
  GNOME_APP_PIXMAP_FILENAME
} GnomeUIPixmapType;

struct _GnomeUIInfo {
  GnomeUIInfoType type;

  gchar *label;

  gchar *hint; /* For toolbar items, the tooltip.
		  For menu items, the status bar message */

  /* For an item, toggleitem, or radioitem, procedure to call when activated.
     
     For a subtree, point to the GnomeUIInfo array for
     that subtree.

     For a radioitem lead entry, point to the GnomeUIInfo array for
     the radio item group.  For the radioitem array, procedure to
     call when activated.
     
     For a help item, specifies the help node to load
     (or NULL for main prog's name)

     For builder data, point to the GnomeUIBuilderData structure for the following items */
  gpointer moreinfo;

  /* Value for pass to gtk_signal_connect() */
  gpointer user_data;

  /* Unsed - for future expansion.  Should always be NULL. */
  gpointer unused_data;

  GnomeUIPixmapType pixmap_type;

  /* Either 
   * a pointer to the char for the pixmap (GNOME_APP_PIXMAP_DATA),
   * a char* for the filename (GNOME_APP_PIXMAP_FILENAME),
   * or a char* for the stock pixmap name (GNOME_APP_PIXMAP_STOCK).
   */
  gpointer pixmap_info;

  guint accelerator_key; /* Accelerator key... Set to 0 to ignore */
  GdkModifierType ac_mods; /* An OR of the masks for the accelerator */

  GtkWidget *widget; /* Filled in by gnome_app_create* */
};
typedef struct _GnomeUIInfo GnomeUIInfo;


/* Handy GnomeUIInfo macros */

#define GNOMEUIINFO_END       {GNOME_APP_UI_ENDOFINFO, NULL, NULL, NULL, \
                               NULL, NULL, (GnomeUIPixmapType)0, NULL, 0, \
                               (GdkModifierType)0, NULL}
#define GNOMEUIINFO_SEPARATOR {GNOME_APP_UI_SEPARATOR, NULL, NULL, NULL, \
                               NULL, NULL, (GnomeUIPixmapType)0, NULL, 0, \
                               (GdkModifierType)0, NULL}
#define GNOMEUIINFO_JUSTIFY_RIGHT   {GNOME_APP_UI_JUSTIFY_RIGHT, \
                                     NULL, NULL, NULL, NULL, NULL, 0,\
                                     (GnomeUIPixmapType)0, '\0',       \
                                     (GdkModifierType)0, NULL}

#define GNOMEUIINFO_ITEM(label, tip, cb, xpm) \
                              {GNOME_APP_UI_ITEM, label, tip, cb, \
			       NULL, NULL, GNOME_APP_PIXMAP_DATA, xpm, \
			       0, (GdkModifierType)0, NULL}
#define GNOMEUIINFO_ITEM_STOCK(label, tip, cb, xpm) \
                              {GNOME_APP_UI_ITEM, label, tip, cb, \
			       NULL, NULL, GNOME_APP_PIXMAP_STOCK, xpm, \
			       0, (GdkModifierType)0, NULL}
#define GNOMEUIINFO_ITEM_NONE(label, tip, cb) \
                              {GNOME_APP_UI_ITEM, label, tip, cb, \
			       NULL, NULL, GNOME_APP_PIXMAP_NONE, NULL, \
			       0, (GdkModifierType)0, NULL}
#define GNOMEUIINFO_ITEM_DATA(label, tip, cb, data, xpm) \
                              {GNOME_APP_UI_ITEM, label, tip, cb, \
			       data, NULL, GNOME_APP_PIXMAP_DATA, xpm, \
			       0, (GdkModifierType)0, NULL}
#define GNOMEUIINFO_TOGGLEITEM(label, tip, cb, xpm) \
                              {GNOME_APP_UI_TOGGLEITEM, label, tip, cb, \
			       NULL, NULL, GNOME_APP_PIXMAP_DATA, xpm, \
			       0, (GdkModifierType)0, NULL}
#define GNOMEUIINFO_TOGGLEITEM_DATA(label, tip, cb, data, xpm) \
                              {GNOME_APP_UI_TOGGLEITEM, label, tip, cb, \
			       data, NULL, GNOME_APP_PIXMAP_DATA, xpm, \
			       0, (GdkModifierType)0, NULL}
#define GNOMEUIINFO_HELP(name) \
                              {GNOME_APP_UI_HELP, NULL, NULL, name, \
                               NULL, NULL, (GnomeUIPixmapType)0, NULL, 0, \
                               (GdkModifierType)0, NULL}
#define GNOMEUIINFO_SUBTREE(label, tree) \
                              {GNOME_APP_UI_SUBTREE, label, NULL, tree, \
                               NULL, NULL, (GnomeUIPixmapType)0, NULL, 0, \
                               (GdkModifierType)0, NULL}
#define GNOMEUIINFO_RADIOLIST(list) \
                              {GNOME_APP_UI_RADIOITEMS, NULL, NULL, list, \
                               NULL, NULL, (GnomeUIPixmapType)0, NULL, 0, \
                               (GdkModifierType)0, NULL}
#define GNOMEUIINFO_RADIOITEM(label, tip, cb, xpm) \
                              {GNOME_APP_UI_RADIOITEMS, label, tip, cb, \
			       NULL, NULL, GNOME_APP_PIXMAP_DATA, xpm, \
			       0, (GdkModifierType)0, NULL}
#define GNOMEUIINFO_RADIOITEM_DATA(label, tip, cb, data, xpm) \
                              {GNOME_APP_UI_RADIOITEMS, label, tip, cb, \
			       data, NULL, GNOME_APP_PIXMAP_DATA, xpm, \
			       0, (GdkModifierType)0, NULL}


/* Functions */
    
typedef struct _GnomeUIBuilderData GnomeUIBuilderData;
typedef void (*GnomeUISignalConnectFunc)(GnomeApp *app,
					 GnomeUIInfo *info_item,
					 gchar *signal_name,
					 GnomeUIBuilderData *uidata);
#define GNOME_UISIGFUNC(x) ((void *)x)
struct _GnomeUIBuilderData {
  GnomeUISignalConnectFunc connect_func;
  gpointer data;
  gboolean is_interp;
  GtkCallbackMarshal relay_func;
  GtkDestroyNotify destroy_func;
};

void gnome_app_create_menus             (GnomeApp *app,
			                 GnomeUIInfo *menuinfo);
void gnome_app_create_menus_interp      (GnomeApp *app,
					 GnomeUIInfo *menuinfo,
					 GtkCallbackMarshal relay_func,
					 gpointer data,
					 GtkDestroyNotify destroy_func);
void gnome_app_create_menus_with_data   (GnomeApp *app,
			                 GnomeUIInfo *menuinfo,
				         gpointer data);
void gnome_app_create_menus_custom      (GnomeApp *app,
					 GnomeUIInfo *menuinfo,
					 GnomeUIBuilderData *uibdata);
void gnome_app_create_toolbar           (GnomeApp *app,
			                 GnomeUIInfo *toolbarinfo);
void gnome_app_create_toolbar_interp    (GnomeApp *app,
					 GnomeUIInfo *tbinfo,
					 GtkCallbackMarshal relay_func,
					 gpointer data,
					 GtkDestroyNotify destroy_func);
void gnome_app_create_toolbar_with_data (GnomeApp *app,
			                 GnomeUIInfo *toolbarinfo,
				         gpointer data);
void gnome_app_create_toolbar_custom    (GnomeApp *app,
					 GnomeUIInfo *tbinfo,
					 GnomeUIBuilderData *uibdata);
GtkWidget *gnome_app_find_menu_pos      (GtkWidget *parent,
					 gchar *path,
					 gint *pos);
void gnome_app_remove_menus             (GnomeApp *app,
					 gchar *path,
					 gint items);
void gnome_app_insert_menus_custom      (GnomeApp *app,
					 gchar *path,
					 GnomeUIInfo *menuinfo,
					 GnomeUIBuilderData *uibdata);
void gnome_app_insert_menus             (GnomeApp *app,
					 gchar *path,
					 GnomeUIInfo *menuinfo);
void gnome_app_insert_menus_with_data   (GnomeApp *app,
					 gchar *path,
					 GnomeUIInfo *menuinfo,
					 gpointer data);
void gnome_app_insert_menus_interp      (GnomeApp *app,
					 gchar *path,
					 GnomeUIInfo *menuinfo,
					 GtkCallbackMarshal relay_func,
					 gpointer data,
					 GtkDestroyNotify destroy_func);

END_GNOME_DECLS

#endif
