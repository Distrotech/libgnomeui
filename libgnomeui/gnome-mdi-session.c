/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */

#include <gnome-mdi-session.h>

static GPtrArray *	config_get_list		(const gchar *);
static void		config_set_list		(const gchar *, GList *,
						 gpointer (*)(gpointer));
static void		restore_window_child	(GnomeMDI *, GHashTable *,
						 GHashTable *, glong, glong,
						 gboolean *, gint, gint,
						 gint, gint);
static void		restore_window		(GnomeMDI *, const gchar *,
						 GPtrArray *, GHashTable *,
						 GHashTable *, glong);

static gpointer		window_list_func	(gpointer);
static gpointer		widget_parent_func	(gpointer);

static GPtrArray *
config_get_list (const gchar *key)
{
	GPtrArray *array;
	gchar *string, *pos;

	string = gnome_config_get_string (key);
	if (!string) return NULL;

	array = g_ptr_array_new ();

	pos = string;
	while (*pos) {
		glong value;
		gchar *tmp;

		tmp = strchr (pos, ':');
		if (tmp) *tmp = '\0';

		if (sscanf (pos, "%lx", &value) == 1)
			g_ptr_array_add (array, (gpointer) value);

		if (tmp)
			pos = tmp+1;
		else
			break;
	}

	g_free (string);
	return array;
}

static void
config_set_list (const gchar *key, GList *list, gpointer (*func)(gpointer))
{
	gchar value [BUFSIZ];

	value [0] = '\0';
	while (list) {
		char buffer [BUFSIZ];
		gpointer data;

		data = func ? func (list->data) : list->data;
		sprintf (buffer, "%lx", (glong) data);
		if (*value) strcat (value, ":");
		strcat (value, buffer);

		list = g_list_next (list);
	}

	gnome_config_set_string (key, value);
}

static void
restore_window_child (GnomeMDI *mdi, GHashTable *child_hash,
		      GHashTable *child_windows, glong window, glong child,
		      gboolean *init, gint x, gint y, gint width, gint height)
{
	GPtrArray *windows;
	GnomeMDIChild *mdi_child;
	guint k;
		
	windows = g_hash_table_lookup (child_windows, (gpointer) child);
	if (!windows) return;
		
	mdi_child = g_hash_table_lookup (child_hash, (gpointer) child);
	if (!mdi_child) return;
		
	for (k = 0; k < windows->len; k++) {
		if (windows->pdata [k] != (gpointer) window)
			continue;
		
		if (*init)
			gnome_mdi_add_view (mdi, mdi_child);
		else {
			gnome_mdi_add_toplevel_view (mdi, mdi_child);
			
			gtk_widget_set_usize
				(GTK_WIDGET (mdi->active_window),
				 width, height);

			gtk_widget_set_uposition
				(GTK_WIDGET (mdi->active_window), x, y);
			
			*init = TRUE;
		}
	}
}

static void
restore_window (GnomeMDI *mdi, const gchar *section, GPtrArray *child_list,
		GHashTable *child_hash, GHashTable *child_windows,
		glong window)
{
	gboolean init = FALSE;
	gchar key [BUFSIZ], *string;
	gint ret, x, y, w, h;
	guint j;

	sprintf (key, "%s/mdi_window_%lx", section, window);
	string = gnome_config_get_string (key);
	if (!string) return;

	ret = sscanf (string, "%d/%d/%d/%d", &x, &y, &w, &h);
	g_free (string);
	if (ret != 4) return;

	printf ("Window: %lx - %d,%d,%d,%d\n", window, x, y, w, h);

	for (j = 0; j < child_list->len; j++)
		restore_window_child (mdi, child_hash, child_windows,
				      window, (glong) child_list->pdata [j],
				      &init, x, y, w, h);
}

gboolean
gnome_mdi_restore_state (GnomeMDI *mdi, const gchar *section,
			 GnomeMDIChildCreate create_child_func)
{
	gchar key [BUFSIZ], *string;
	GPtrArray *window_list, *child_list;
	GHashTable *child_hash, *child_windows;
	guint i;

	sprintf (key, "%s/mdi_session=0", section);
	if (gnome_config_get_int (key) == 0)
		return FALSE;

	child_hash = g_hash_table_new (NULL, NULL);
	child_windows = g_hash_table_new (NULL, NULL);

	/* Walk the list of windows. */

	sprintf (key, "%s/mdi_windows", section);
	window_list = config_get_list (key);

	/* Walk the list of children. */

	sprintf (key, "%s/mdi_children", section);
	child_list = config_get_list (key);

	/* Read child descriptions. */

	for (i = 0; i < child_list->len; i++) {
		glong child = (glong) child_list->pdata [i];
		GnomeMDIChild *mdi_child;
		GPtrArray *windows;

		sprintf (key, "%s/mdi_child_config_%lx", section, child);
		string = gnome_config_get_string (key);
		if (!string) continue;

		mdi_child = create_child_func (string);
		g_free (string);

		gnome_mdi_add_child (mdi, mdi_child);

		g_hash_table_insert (child_hash, (gpointer) child, mdi_child);
		
		sprintf (key, "%s/mdi_child_views_%lx", section, child);
		windows = config_get_list (key);

		g_hash_table_insert (child_windows, (gpointer) child, windows);
	}

	/* Read window descriptions. */
	
	for (i = 0; i < window_list->len; i++)
		restore_window (mdi, section, child_list,
				child_hash, child_windows,
				(glong) window_list->pdata [i]);

	return TRUE;
}

static gpointer
window_list_func (gpointer data)
{
	return ((GnomeApp *) data)->contents;
}

static gpointer
widget_parent_func (gpointer data)
{
	return ((GtkWidget *) data)->parent;
}

void
gnome_mdi_save_state (GnomeMDI *mdi, const gchar *section)
{
	gchar key [BUFSIZ], value [BUFSIZ];
	GList *child, *window;
	gint x, y, w, h;

	gnome_config_clean_section (section);

	sprintf (key, "%s/mdi_session", section);
	gnome_config_set_int (key, 1);

	/* Write list of children. */

	sprintf (key, "%s/mdi_children", section);
	config_set_list (key, mdi->children, NULL);

	/* Write list of windows. */

	sprintf (key, "%s/mdi_windows", section);
	config_set_list (key, mdi->windows, window_list_func);

	/* Walk list of children. */

	child = mdi->children;
	while (child) {
		GnomeMDIChild *mdi_child;
		gchar *string;

		mdi_child = GNOME_MDI_CHILD (child->data);

		/* Save child configuration. */

		gtk_signal_emit_by_name (GTK_OBJECT (mdi_child),
					 "get_config_string", &string);

		if (string) {
			sprintf (key, "%s/mdi_child_config_%lx",
				 section, (long) mdi_child);
			gnome_config_set_string (key, string);
			g_free (string);
		}

		/* Save list of views this child has. */

		sprintf (key, "%s/mdi_child_views_%lx",
			 section, (long) mdi_child);
		config_set_list (key, mdi_child->views, widget_parent_func);

		child = g_list_next (child);
	}

	/* Save list of toplevel windows. */

	window = mdi->windows;
	while (window) {
		GnomeApp *app;

		app = GNOME_APP (window->data);

		gdk_window_get_geometry (GTK_WIDGET (app)->window,
					 &x, &y, &w, &h, NULL);

		gdk_window_get_position (GTK_WIDGET (app)->window, &x, &y);

		sprintf (key, "%s/mdi_window_%lx",
			 section, (long) app->contents);
		sprintf (value, "%d/%d/%d/%d", x, y, w, h);

		gnome_config_set_string (key, value);

		window = g_list_next (window);
	}
}
