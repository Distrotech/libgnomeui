/* GnomeEntry widget - combo box with auto-saved history
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#include <stdio.h>
#include <string.h>
#include <gtk/gtkentry.h>
#include <gtk/gtklist.h>
#include <gtk/gtklistitem.h>
#include <gtk/gtksignal.h>
#include "libgnome/libgnome.h"
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
entry_changed (GtkWidget *widget, gpointer data)
{
	GnomeEntry *gentry;

	gentry = data;
	gentry->changed = TRUE;
}

static void
entry_activated (GtkWidget *widget, gpointer data)
{
	GnomeEntry *gentry;
	char *text;

	gentry = data;

	text = gtk_entry_get_text (GTK_ENTRY (widget));

	if (!gentry->changed || (strcmp (text, "") == 0)) {
		gentry->changed = FALSE;
		return;
	}

	gnome_entry_prepend_history (gentry, TRUE, gtk_entry_get_text (GTK_ENTRY (widget)));
}

static void
gnome_entry_init (GnomeEntry *gentry)
{
	gentry->changed    = FALSE;
	gentry->history_id = NULL;
	gentry->items      = NULL;

	gtk_signal_connect (GTK_OBJECT (gnome_entry_gtk_entry (gentry)), "changed",
			    (GtkSignalFunc) entry_changed,
			    gentry);
	gtk_signal_connect (GTK_OBJECT (gnome_entry_gtk_entry (gentry)), "activate",
			    (GtkSignalFunc) entry_activated,
			    gentry);
}

GtkWidget *
gnome_entry_new (char *history_id)
{
	GnomeEntry *gentry;

	gentry = gtk_type_new (gnome_entry_get_type ());

	gnome_entry_set_history_id (gentry, history_id);
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
	GtkWidget *entry;
	char *text;

	g_return_if_fail (object != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (object));

	gentry = GNOME_ENTRY (object);

	entry = gnome_entry_gtk_entry (gentry);
	text = gtk_entry_get_text (GTK_ENTRY (entry));

	if (strcmp (text, "") != 0)
		gnome_entry_prepend_history (gentry, TRUE, text);

	gnome_entry_save_history (gentry);

	if (gentry->history_id)
		g_free (gentry->history_id);

	free_items (gentry);

	if (GTK_OBJECT_CLASS (parent_class)->destroy)
		(* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

GtkWidget *
gnome_entry_gtk_entry (GnomeEntry *gentry)
{
	g_return_val_if_fail (gentry != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_ENTRY (gentry), NULL);

	return GTK_COMBO (gentry)->entry;
}

void
gnome_entry_set_history_id (GnomeEntry *gentry, char *history_id)
{
	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	if (gentry->history_id)
		g_free (gentry->history_id);

	gentry->history_id = g_strdup (history_id); /* this handles NULL correctly */
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

void
gnome_entry_prepend_history (GnomeEntry *gentry, int save, char *text)
{
	struct item *item;
	GList *gitem;
	GtkWidget *li;
	GtkWidget *entry;
	char *tmp;

	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));
	g_return_if_fail (text != NULL); /* FIXME: should we just return without warning? */

	entry = gnome_entry_gtk_entry (gentry);
	tmp = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	
	item = g_new (struct item, 1);
	item->save = save;
	item->text = g_strdup (text);

	gentry->items = g_list_prepend (gentry->items, item);

	li = gtk_list_item_new_with_label (text);
	gtk_widget_show (li);

	gitem = g_list_append (NULL, li);
	gtk_list_prepend_items (GTK_LIST (GTK_COMBO (gentry)->list), gitem);

	gtk_entry_set_text (GTK_ENTRY (entry), tmp);
	g_free (tmp);
}

void
gnome_entry_append_history (GnomeEntry *gentry, int save, char *text)
{
	struct item *item;
	GList *gitem;
	GtkWidget *li;
	GtkWidget *entry;
	char *tmp;

	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));
	g_return_if_fail (text != NULL); /* FIXME: shoudl we just return without warning? */

	entry = gnome_entry_gtk_entry (gentry);
	tmp = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	
	item = g_new (struct item, 1);
	item->save = save;
	item->text = g_strdup (text);

	gentry->items = g_list_append (gentry->items, item);

	li = gtk_list_item_new_with_label (text);
	gtk_widget_show (li);

	gitem = g_list_append (NULL, li);
	gtk_list_append_items (GTK_LIST (GTK_COMBO (gentry)->list), gitem);

	gtk_entry_set_text (GTK_ENTRY (entry), tmp);
	g_free (tmp);
}

static void
set_combo_items (GnomeEntry *gentry)
{
	GtkList *gtklist;
	GList *items;
	GList *gitems;
	struct item *item;
	GtkWidget *li;
	GtkWidget *entry;
	char *tmp;

	gtklist = GTK_LIST (GTK_COMBO (gentry)->list);

	gtk_container_block_resize (GTK_CONTAINER (gtklist));

	/* We save the contents of the entry because when we insert
	 * items on the combo list, the contents of the entry will get
	 * changed.
	 */

	entry = gnome_entry_gtk_entry (gentry);
	tmp = g_strdup (gtk_entry_get_text (GTK_ENTRY (entry)));
	
	gtk_list_clear_items (gtklist, 0, -1); /* erase everything */

	gitems = NULL;

	for (items = gentry->items; items; items = items->next) {
		item = items->data;

		li = gtk_list_item_new_with_label (item->text);
		gtk_widget_show (li);
		
		gitems = g_list_append (gitems, li);
	}

	gtk_list_append_items (gtklist, gitems); /* this handles NULL correctly */

	gtk_entry_set_text (GTK_ENTRY (entry), tmp);
	g_free (tmp);

	gtk_container_unblock_resize (GTK_CONTAINER (gtklist));
}

void
gnome_entry_load_history (GnomeEntry *gentry)
{
	char *prefix;
	struct item *item;
	int n;
	char key[32];
	char *value;
	GSList *prefix_list;

	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	if (!(gnome_app_id && gentry->history_id))
		return;

	free_items (gentry);

	/*so that we don't disturb the app's prefix list*/
	prefix_list = gnome_config_remove_prefix_list();

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
	gnome_config_set_prefix_list(prefix_list);
}

void
gnome_entry_save_history (GnomeEntry *gentry)
{
	char *prefix;
	GList *items;
	struct item *item;
	int n;
	char key[32];
	GSList *prefix_list;

	g_return_if_fail (gentry != NULL);
	g_return_if_fail (GNOME_IS_ENTRY (gentry));

	if (!(gnome_app_id && gentry->history_id))
		return;

	/*so that we don't disturb the app's prefix list*/
	prefix_list = gnome_config_remove_prefix_list();

	prefix = build_prefix (gentry, FALSE);
	if (gnome_config_has_section (prefix))
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
	gnome_config_set_prefix_list(prefix_list);
	gnome_config_sync ();
}
