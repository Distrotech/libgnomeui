/* GnomeEntry widget - combo box with auto-saved history
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include "libgnome/gnome-defs.h"
#include "libgnome/gnome-config.h"
#include "libgnome/gnome-util.h"
#include "gnome-entry.h"


#define DEFAULT_MAX_HISTORY_SAVED 60  /* Why 60?  Because MC defaults to that :-) */


struct item {
	int   save;
	char *text;
};


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
	gentry->history_id = NULL;
	gentry->items      = NULL;
}

GtkWidget *
gnome_entry_new (char *history_id)
{
	GnomeEntry *gentry;

	gentry = gtk_type_new (gnome_entry_get_type ());

	gentry->history_id = g_strdup (history_id); /* this handles NULL correctly */

	gnome_entry_load_history (gentry);

	return GTK_WIDGET (gentry);
}

static void
free_item (gpointer data, gpointer user_data)
{
	struct item *item;

	item = data;
	if (item->text)
		g_free (item->text);

	g_free (item);
}

static void
free_items (GnomeEntry *gentry)
{
	g_list_foreach (gentry->items, free_item, NULL);
	g_list_free (gentry->items);
	gentry->items = NULL;
}

static void
gnome_entry_destroy (GtkObject *object)
{
	GnomeEntry *gentry;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (object));

	gentry = GNOME_ENTRY (object);

	gnome_entry_save_history (gentry);

	if (gentry->history_id)
		g_free (gentry->history_id);

	free_items (gentry);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

static char *
build_prefix (GnomeEntry *gentry, int trailing_slash)
{
	return g_copy_strings ("/",
			       gnome_app_id,
			       "/History: ",
			       gentry->history_id,
			       trailing_slash ? "/" : "",
			       NULL);
}

static void
set_combo_items (GnomeEntry *gentry)
{
	GtkList *gtklist;

	gtklist = GTK_LIST (GTK_COMBO (gentry)->list);

	gtk_list_clear_items (gtklist, 0, -1); /* erase everything */


}

void
gnome_entry_load_history (GnomeEntry *gentry)
{
	char *prefix;
	struct item *item;
	int n;
	char key[32];
	char *value;

	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	if (!(gnome_get_app_id () && gentry->history_id))
		return;

	free_items (gentry);

	prefix = build_prefix (gentry, TRUE);
	gnome_config_push_prefix (prefix);
	g_free (prefix);

	for (n = 0; ; n++) {
		sprintf (key, "%d", n);
		value = gnome_config_get_string (key);
		if (!value)
			break;

		item = g_new (struct item, 1);
		item->save = TRUE;
		item->text = value;

		gentry->items = g_list_prepend (gentry->items, item);
	}

	set_combo_items (gentry);

	gnome_config_pop_prefix ();
}

void
gnome_entry_save_history (GnomeEntry *gentry)
{
	char *prefix;
	GList *items;
	struct item *item;
	int n;
	char key[32];

	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	if (!(gnome_app_get_id () && gentry->history_id))
		return;

	prefix = build_prefix (gentry, FALSE);
	gnome_config_clean_section (prefix);
	g_free (prefix);
	
	prefix = build_prefix (gentry, TRUE);
	gnome_config_push_prefix (prefix);
	g_free (prefix);

	/* FIXME: add condition n < max_num_history_saved */

	for (n = 0, items = gentry->items; items; items = items->next) {
		item = items->data;

		if (item->save) {
			sprintf (key, "%d", n++);
			gnome_config_set_string (key, item->text);
		}
	}

	gnome_config_pop_prefix ();
	gnome_config_sync ();
}
