BEGIN_GNOME_DECLS

/*
 * Helper routines,
 */
struct _GnomeMenuInfo {
	enum {
		GNOME_APP_MENU_ENDOFINFO,
		GNOME_APP_MENU_ITEM,
		GNOME_APP_MENU_SUBMENU,
		GNOME_APP_MENU_SEPARATOR,
		GNOME_APP_MENU_HELP
	} type;
	gchar *label;
	gpointer moreinfo; /* For a menuitem, this should point to the
			      procedure to be called when this menu item is
			      activated.
			      
			      For a submenu, it should point to the
			      GnomeMenuInfo array for that menu.

			      For a help entry, is points to a string
			      which is a component of the help path
			      used to find the help for this app.
			   */
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

void gnome_app_create_menus             (GnomeApp *app,
			                 GnomeMenuInfo *menuinfo);
void gnome_app_create_menus_with_data   (GnomeApp *app,
			                 GnomeMenuInfo *menuinfo,
				         gpointer data);
void gnome_app_create_toolbar           (GnomeApp *app,
			                 GnomeToolbarInfo *tbinfo);
void gnome_app_create_toolbar_with_data (GnomeApp *app,
			                 GnomeToolbarInfo *tbinfo,
				         gpointer data);

END_GNOME_DECLS
