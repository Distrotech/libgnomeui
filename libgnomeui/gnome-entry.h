/* GnomeEntry widget - combo box with auto-saved history
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_ENTRY_H
#define GNOME_ENTRY_H


#include <gtk/gtkcombo.h>
#include <libgnome/gnome-defs.h>


BEGIN_GNOME_DECLS


#define GNOME_ENTRY(obj)         GTK_CHECK_CAST (obj, gnome_entry_get_type (), GnomeEntry)
#define GNOME_ENTRY_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gtk_entry_get_type (), GtkEntryClass)
#define GNOME_IS_ENTRY(obj)      GTK_CHECK_TYPE (obj, gtk_entry_get_type ())


typedef struct _GnomeEntry      GnomeEntry;
typedef struct _GnomeEntryClass GnomeEntryClass;

struct _GnomeEntry {
	GtkCombo combo;

	char  *app_id;
	char  *history_id;
	GList *items;
};

struct GnomeEntryClass {
	GtkComboClass parent_class;
};


guint      gnome_entry_get_type (void);
GtkWidget *gnome_entry_new (char *app_id,
			    char *history_id);


END_GNOME_DECLS

#endif
