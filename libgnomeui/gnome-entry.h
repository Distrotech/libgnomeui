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


#define GNOME_TYPE_ENTRY            (gnome_entry_get_type ())
#define GNOME_ENTRY(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_ENTRY, GnomeEntry))
#define GNOME_ENTRY_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_ENTRY, GnomeEntryClass))
#define GNOME_IS_ENTRY(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_ENTRY))
#define GNOME_IS_ENTRY_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_ENTRY))


typedef struct _GnomeEntry      GnomeEntry;
typedef struct _GnomeEntryClass GnomeEntryClass;

struct _GnomeEntry {
	GtkCombo combo;

	gboolean changed;
	gchar   *history_id;
	GList   *items;
	guint    max_saved;
};

struct _GnomeEntryClass {
	GtkComboClass parent_class;
};


guint      gnome_entry_get_type        (void);
GtkWidget *gnome_entry_new             (const gchar *history_id);

GtkWidget *gnome_entry_gtk_entry       (GnomeEntry *gentry);
void       gnome_entry_set_history_id  (GnomeEntry *gentry, const gchar *history_id);
void	   gnome_entry_set_max_saved   (GnomeEntry *gentry, guint max_saved);

void       gnome_entry_prepend_history (GnomeEntry *gentry, gint save, const gchar *text);
void       gnome_entry_append_history  (GnomeEntry *gentry, gint save, const gchar *text);
void       gnome_entry_load_history    (GnomeEntry *gentry);
void       gnome_entry_save_history    (GnomeEntry *gentry);

END_GNOME_DECLS

#endif
