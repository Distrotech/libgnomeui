/* GnomeEntry widget - combo box with auto-saved history
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_ENTRY_H
#define GNOME_ENTRY_H


#include <glib.h>
#include <gtk/gtkcombo.h>
#include <libgnome/gnome-defs.h>


BEGIN_GNOME_DECLS


#define GNOME_ENTRY(obj)         GTK_CHECK_CAST (obj, gnome_entry_get_type (), GnomeEntry)
#define GNOME_ENTRY_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gnome_entry_get_type (), GnomeEntryClass)
#define GNOME_IS_ENTRY(obj)      GTK_CHECK_TYPE (obj, gnome_entry_get_type ())


typedef struct _GnomeEntry      GnomeEntry;
typedef struct _GnomeEntryClass GnomeEntryClass;

struct _GnomeEntry {
	GtkCombo combo;
	gchar   *history_id;
	GList   *items;

	guint16    max_saved;
	gboolean changed : 1;
};

struct _GnomeEntryClass {
	GtkComboClass parent_class;
};


guint      gnome_entry_get_type        (void);
GtkWidget *gnome_entry_new             (const gchar *history_id);

/* for language bindings and subclassing, use gnome_entry_new */
void	   gnome_entry_construct       (GnomeEntry *gentry, 
					const gchar *history_id);

GtkWidget *gnome_entry_gtk_entry       (GnomeEntry *gentry);
void       gnome_entry_set_history_id  (GnomeEntry *gentry, const gchar *history_id);
void	   gnome_entry_set_max_saved   (GnomeEntry *gentry, guint max_saved);

void       gnome_entry_prepend_history (GnomeEntry *gentry, gboolean save, const gchar *text);
void       gnome_entry_append_history  (GnomeEntry *gentry, gboolean save, const gchar *text);
void       gnome_entry_load_history    (GnomeEntry *gentry);
void       gnome_entry_save_history    (GnomeEntry *gentry);

END_GNOME_DECLS

#endif
