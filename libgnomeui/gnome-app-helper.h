#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

/*
 * Helper routines,
 */
struct _GnomeUIInfo {
  enum {
    GNOME_APP_UI_ENDOFINFO = 0,
    GNOME_APP_UI_ITEM = 1,
    GNOME_APP_UI_SUBTREE = 2,
    GNOME_APP_UI_SEPARATOR = 3,
    GNOME_APP_UI_HELP = 4,
    GNOME_APP_UI_LAST = 4
  } type;

  gchar *label;

  gchar *hint; /* For toolbar items, the tooltip.
		  For menu items, the status bar message */

  /* For an item, procedure to call when activated.
     
     For a subtree, point to the GnomeUIInfo array for
     that subtree. */
  gpointer moreinfo; 

  enum {
    GNOME_APP_PIXMAP_NONE,
    GNOME_APP_PIXMAP_STOCK,
    GNOME_APP_PIXMAP_DATA, /* Can't currently use these last two in menus */
    GNOME_APP_PIXMAP_FILENAME,
  } pixmap_type;

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

END_GNOME_DECLS
