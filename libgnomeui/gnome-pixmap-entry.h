/* GnomePixmapEntry widget - Combo box with "Browse" button for files and
 *			     preview space for pixmaps and a file picker in
 *			     electric eyes style (well kind of)
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: George Lebl <jirka@5z.com>
 */

#ifndef GNOME_PIXMAP_ENTRY_H
#define GNOME_PIXMAP_ENTRY_H


#include <gtk/gtkvbox.h>
#include <libgnome/gnome-defs.h>
#include <libgnomeui/gnome-file-entry.h>


BEGIN_GNOME_DECLS


#define GNOME_PIXMAP_ENTRY(obj)         GTK_CHECK_CAST (obj, gnome_pixmap_entry_get_type (), GnomePixmapEntry)
#define GNOME_PIXMAP_ENTRY_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gnome_pixmap_entry_get_type (), GnomePixmapEntryClass)
#define GNOME_IS_PIXMAP_ENTRY(obj)      GTK_CHECK_TYPE (obj, gnome_pixmap_entry_get_type ())


typedef struct _GnomePixmapEntry      GnomePixmapEntry;
typedef struct _GnomePixmapEntryClass GnomePixmapEntryClass;

struct _GnomePixmapEntry {
	GtkVBox vbox;
	
	GtkWidget *fentry;

	GtkWidget *preview;
	GtkWidget *preview_sw;
	
	/*very private*/
	gchar *last_preview;

	guint32 do_preview : 1; /*put a preview frame with the pixmap next to
				  the entry*/
};

struct _GnomePixmapEntryClass {
	GtkVBoxClass parent_class;
};


guint      gnome_pixmap_entry_get_type    (void);
GtkWidget *gnome_pixmap_entry_new         (const gchar *history_id,
					   const gchar *browse_dialog_title,
					   gboolean do_preview);
void       gnome_pixmap_entry_construct   (GnomePixmapEntry *gentry,
					   const gchar *history_id,
					   const gchar *browse_dialog_title,
					   gboolean do_preview);

/*by default gnome_pixmap entry sets the default directory to the
  gnome pixmap directory, this will set it to a subdirectory of that,
  or one would use the file_entry functions for any other path*/
void       gnome_pixmap_entry_set_pixmap_subdir(GnomePixmapEntry *pentry,
						const gchar *subdir);

GtkWidget *gnome_pixmap_entry_gnome_file_entry(GnomePixmapEntry *pentry);
GtkWidget *gnome_pixmap_entry_gnome_entry (GnomePixmapEntry *pentry);
GtkWidget *gnome_pixmap_entry_gtk_entry   (GnomePixmapEntry *pentry);

/*set the preview parameters, if preview is off then the preview frame
  will be hidden*/
void       gnome_pixmap_entry_set_preview (GnomePixmapEntry *pentry,
					   gboolean do_preview);
void	   gnome_pixmap_entry_set_preview_size(GnomePixmapEntry *pentry,
					       gint preview_w,
					       gint preview_h);

/*only return a file if it was possible to load it with gdk-pixbuf*/
gchar      *gnome_pixmap_entry_get_filename(GnomePixmapEntry *pentry);

END_GNOME_DECLS

#endif
