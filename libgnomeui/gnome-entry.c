/* GnomeEntry widget - combo box with auto-saved history
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include "gnome-entry.h"


static void gnome_entry_class_init (GnomeEntryClass *class);
static void gnome_entry_init       (GnomeEntry      *gentry);
static void gnome_entry_destroy    (GtkObject       *object);


static GtkComboClass *parent_class;


guint
gnome_entry_get_type (void)
{
	static guint entry_type = 0;

	if (!entry_type) {
		GtkTypeInfo entry_info = {
			"GnomeEntry",
			sizeof (GnomeEntry),
			sizeof (GnomeEntryClass),
			(GtkClassInitFunc) gnome_entry_class_init,
			(GtkObjectInitFunc) gnome_entry_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL
		};

		entry_type = gtk_type_unique (gtk_combo_get_type (), &entry_info);
	}

	return entry_type;
}

static void
gnome_entry_class_init (GnomeEntryClass *class)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass *) class;

	parent_class = gtk_type_class (gtk_combo_get_type ());

	object_class->destroy = gnome_entry_destroy;
}

static void
gnome_entry_init (GnomeEntry *gentry)
{
	gentry->app_id     = NULL;
	gentry->history_id = NULL;
	gentry->items      = NULL;
}

GtkWidget *
gnome_entry_new (char *app_id,
		 char *history_id)
{
	GnomeEntry *gentry;

	gentry = gtk_type_new (gnome_entry_get_type ());

	gentry->app_id     = g_strdup (app_id); /* these handle NULL correctly */
	gentry->history_id = g_strdup (history_id);

	/* FIXME: load items with gnome_config */

	return GTK_WIDGET (gentry);
}

static void
gnome_entry_destroy (GtkObject *object)
{
	GnomeEntry *gentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (object));

	gentry = GNOME_ENTRY (object);

	/* FIXME: save items with gnome_config */

	if (gentry->app_id)
		g_free (gentry->app_id);

	if (gentry->history_id)
		g_free (gentry->history_id);

	/* FIXME: free items */

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}
