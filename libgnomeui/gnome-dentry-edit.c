/* gnome-dentry-edit.c: Copyright (C) 1998 Free Software Foundation
 * Written by: Havoc Pennington, based on code by John Ellis.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <config.h>

#include <ctype.h>
#include <string.h>
#include "gnome-dentry-edit.h"

#ifdef NEED_GNOMESUPPORT_H
# include "gnomesupport.h"
#endif
#include "libgnome/libgnomeP.h"

#include "gnome-stock.h"
#include "gnome-dialog.h"
#include "gnome-uidefs.h"
#include "gnome-pixmap.h"
#include "gnome-icon-sel.h"

static void gnome_dentry_edit_class_init (GnomeDEntryEditClass *klass);
static void gnome_dentry_edit_init       (GnomeDEntryEdit      *messagebox);

static void gnome_dentry_edit_destroy (GtkObject *dee);

static void gnome_dentry_edit_sync_display(GnomeDEntryEdit * dee,
					   GnomeDesktopEntry * dentry);

static void gnome_dentry_edit_sync_dentry(GnomeDEntryEdit * dee,
					  GnomeDesktopEntry * dentry);

static void gnome_dentry_edit_changed(GnomeDEntryEdit * dee);
static void gnome_dentry_edit_icon_changed(GnomeDEntryEdit * dee);
static void gnome_dentry_edit_name_changed(GnomeDEntryEdit * dee);

static void gnome_dentry_edit_set_icon(GnomeDEntryEdit * dee,
				       const gchar * icon_name);

static void show_icon_selection(GtkButton * b, GnomeDEntryEdit * dee);

enum {
  CHANGED,
  ICON_CHANGED,
  NAME_CHANGED,
  LAST_SIGNAL
};

static gint dentry_edit_signals[LAST_SIGNAL] = { 0 };

static GtkObjectClass *parent_class;

guint
gnome_dentry_edit_get_type ()
{
  static guint dee_type = 0;

  if (!dee_type)
    {
      GtkTypeInfo dee_info =
      {
	"GnomeDEntryEdit",
	sizeof (GnomeDEntryEdit),
	sizeof (GnomeDEntryEditClass),
	(GtkClassInitFunc) gnome_dentry_edit_class_init,
	(GtkObjectInitFunc) gnome_dentry_edit_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
      };

      dee_type = gtk_type_unique (gtk_object_get_type (), &dee_info);
    }

  return dee_type;
}

static void
gnome_dentry_edit_class_init (GnomeDEntryEditClass *klass)
{
  GtkObjectClass *object_class;
  GnomeDEntryEditClass * dentry_edit_class;

  dentry_edit_class = (GnomeDEntryEditClass*) klass;

  object_class = (GtkObjectClass*) klass;

  parent_class = gtk_type_class (gtk_object_get_type ());
  
  dentry_edit_signals[CHANGED] =
    gtk_signal_new ("changed",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeDEntryEditClass, changed),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);

  dentry_edit_signals[ICON_CHANGED] =
    gtk_signal_new ("icon_changed",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeDEntryEditClass, 
				       icon_changed),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);

  dentry_edit_signals[NAME_CHANGED] =
    gtk_signal_new ("name_changed",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeDEntryEditClass, 
				       name_changed),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);


  gtk_object_class_add_signals (object_class, dentry_edit_signals, 
				LAST_SIGNAL);

  object_class->destroy = gnome_dentry_edit_destroy;
  dentry_edit_class->changed = NULL;
}

#define TABLE_ENTRY_OPTIONS (GTK_EXPAND | GTK_FILL)
#define TABLE_LABEL_OPTIONS 0

static void 
table_attach_entry(GtkTable * table, GtkWidget * w,
		   gint l, gint r, gint t, gint b)
{
  gtk_table_attach(table,w, l, r, t, b, 
		   TABLE_ENTRY_OPTIONS, TABLE_ENTRY_OPTIONS, 
		   GNOME_PAD_SMALL, GNOME_PAD_SMALL);
}

static void
table_attach_label(GtkTable * table, GtkWidget * w,
		   gint l, gint r, gint t, gint b)
{
  gtk_table_attach(table,w, l, r, t, b, 
		   TABLE_LABEL_OPTIONS, TABLE_LABEL_OPTIONS, 
		   GNOME_PAD_SMALL, GNOME_PAD_SMALL);
}


static void
fill_easy_page(GnomeDEntryEdit * dee, GtkWidget * table)
{
  GtkWidget * label;
  GList * types = NULL;

  label = gtk_label_new(_("Name:"));
  table_attach_label(GTK_TABLE(table), label, 0, 1, 0, 1);

  dee->name_entry = gtk_entry_new();
  table_attach_entry(GTK_TABLE(table),dee->name_entry, 1, 2, 0, 1);
  gtk_signal_connect_object(GTK_OBJECT(dee->name_entry), "changed",
			    GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
			    GTK_OBJECT(dee));
  gtk_signal_connect_object(GTK_OBJECT(dee->name_entry), "changed",
			    GTK_SIGNAL_FUNC(gnome_dentry_edit_name_changed),
			    GTK_OBJECT(dee));


  label = gtk_label_new(_("Comment:"));
  table_attach_label(GTK_TABLE(table),label, 0, 1, 1, 2);

  dee->comment_entry = gtk_entry_new();
  table_attach_entry(GTK_TABLE(table),dee->comment_entry, 1, 2, 1, 2);
  gtk_signal_connect_object(GTK_OBJECT(dee->comment_entry), "changed",
			    GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
			    GTK_OBJECT(dee));


  label = gtk_label_new(_("Command:"));
  table_attach_label(GTK_TABLE(table),label, 0, 1, 2, 3);

  dee->exec_entry = gtk_entry_new();
  table_attach_entry(GTK_TABLE(table),dee->exec_entry, 1, 2, 2, 3);
  gtk_signal_connect_object(GTK_OBJECT(dee->exec_entry), "changed",
			    GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
			    GTK_OBJECT(dee));


  label = gtk_label_new(_("Type:"));
  table_attach_label(GTK_TABLE(table),label, 0, 1, 3, 4);

  types = g_list_prepend(types, "Application");
  types = g_list_prepend(types, "Directory");
  dee->type_combo = gtk_combo_new();
  gtk_combo_set_popdown_strings(GTK_COMBO(dee->type_combo), types);
  g_list_free(types);
  gtk_combo_set_value_in_list(GTK_COMBO(dee->type_combo), 
			      FALSE, TRUE);
  table_attach_entry(GTK_TABLE(table),dee->type_combo, 1, 2, 3, 4);
  gtk_signal_connect_object(GTK_OBJECT(GTK_COMBO(dee->type_combo)->entry), 
			    "changed",
			    GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
			    GTK_OBJECT(dee));

  dee->icon_button = gtk_button_new();
  gtk_widget_set_usize(dee->icon_button, 60, 60);

  gtk_table_attach(GTK_TABLE(table),dee->icon_button, 
		   0, 1, 5, 6, 0, 0, GNOME_PAD_SMALL, 
		   GNOME_PAD_SMALL);

  gtk_signal_connect(GTK_OBJECT(dee->icon_button), "clicked",
		     GTK_SIGNAL_FUNC(show_icon_selection),
		     dee);

  dee->icon_label = gtk_label_new("");
  table_attach_label(GTK_TABLE(table),
		     dee->icon_label, 1, 2, 5, 6);

  gnome_dentry_edit_set_icon(dee, NULL);

  dee->terminal_button = 
    gtk_check_button_new_with_label (_("Run in Terminal"));
  gtk_signal_connect_object(GTK_OBJECT(dee->terminal_button), 
			    "clicked",
			    GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
			    GTK_OBJECT(dee));
  gtk_table_attach(GTK_TABLE(table),dee->terminal_button, 1, 2, 6, 7,
		   0, 0, GNOME_PAD_SMALL, GNOME_PAD_SMALL);
}

static void
fill_advanced_page(GnomeDEntryEdit * dee, GtkWidget * page)
{
  GtkWidget * label;

  label = gtk_label_new(_("Try this before using:"));
  table_attach_label(GTK_TABLE(page),label, 0, 1, 0, 1);
  
  dee->tryexec_entry = gtk_entry_new();
  table_attach_entry(GTK_TABLE(page),dee->tryexec_entry, 1, 2, 0, 1);
  gtk_signal_connect_object(GTK_OBJECT(dee->tryexec_entry), 
			    "changed",
			    GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
			    GTK_OBJECT(dee));
  
  label = gtk_label_new(_("Documentation:"));
  table_attach_label(GTK_TABLE(page),label, 0, 1, 1, 2);
  
  dee->doc_entry = gtk_entry_new_with_max_length(255);
  table_attach_entry(GTK_TABLE(page),dee->doc_entry, 1, 2, 1, 2);
  gtk_signal_connect_object(GTK_OBJECT(dee->doc_entry), 
			    "changed",
			    GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
			    GTK_OBJECT(dee));
}

static GtkWidget *
make_page(GtkWidget * notebook, const gchar * label)
{
  GtkWidget * frame, * page;

  frame = gtk_frame_new(NULL);
  gtk_container_border_width(GTK_CONTAINER(frame), GNOME_PAD_SMALL);
  
  page = gtk_table_new(7, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (page), GNOME_PAD_SMALL);

  gtk_container_add (GTK_CONTAINER(frame), page);

  gtk_notebook_append_page ( GTK_NOTEBOOK(notebook), frame, 
			     gtk_label_new(label) );

  gtk_widget_show(frame);

  return page;
} 

static void
gnome_dentry_edit_init (GnomeDEntryEdit *dee)
{
  dee->icon_dialog  = NULL;
  dee->desktop_icon = NULL;
  dee->icon         = NULL;
}


GtkObject * gnome_dentry_edit_new (GtkNotebook * notebook)
{
  GnomeDEntryEdit * dee;
  GtkWidget * page;

  g_return_val_if_fail(notebook != NULL, NULL);
  
  dee = gtk_type_new(gnome_dentry_edit_get_type());

  page = make_page(GTK_WIDGET(notebook), _("Easy"));
  fill_easy_page(dee, page);
  gtk_widget_show_all(page);

  page = make_page(GTK_WIDGET(notebook), _("Advanced"));
  fill_advanced_page(dee, page);
  gtk_widget_show_all(page);

  /* Destroy self with the notebook. */
  gtk_signal_connect_object(GTK_OBJECT(notebook), "destroy",
			    GTK_SIGNAL_FUNC(gtk_object_destroy),
			    GTK_OBJECT(dee));

#ifdef GNOME_ENABLE_DEBUG
  g_print("Dialog: %p\n", dee->icon_dialog);
#endif

  return GTK_OBJECT (dee);
}

static void gnome_dentry_edit_destroy (GtkObject *dee)
{
  g_return_if_fail(dee != NULL);
  g_return_if_fail(GNOME_IS_DENTRY_EDIT(dee));

  if (GNOME_DENTRY_EDIT(dee)->icon_dialog) {
    gtk_widget_destroy(GNOME_DENTRY_EDIT(dee)->icon_dialog);
  }
  
  g_free(GNOME_DENTRY_EDIT(dee)->icon);

  if (GTK_OBJECT_CLASS(parent_class)->destroy)
    (* (GTK_OBJECT_CLASS(parent_class)->destroy))(dee);
}

/* Conform display to dentry */
static void gnome_dentry_edit_sync_display(GnomeDEntryEdit * dee,
					   GnomeDesktopEntry * dentry)
{
  gchar * s = NULL;
  g_return_if_fail(dee != NULL);
  g_return_if_fail(GNOME_IS_DENTRY_EDIT(dee));

  gtk_entry_set_text(GTK_ENTRY(dee->name_entry), 
		     dentry->name ? dentry->name : "");
  gtk_entry_set_text(GTK_ENTRY(dee->comment_entry),
		     dentry->comment ? dentry->comment : "");
  
  if (dentry->exec) s = g_flatten_vector(" ", dentry->exec);
  gtk_entry_set_text(GTK_ENTRY(dee->exec_entry), s ? s : "");
  g_free(s);

  gtk_entry_set_text(GTK_ENTRY(dee->tryexec_entry), 
		     dentry->tryexec ? dentry->tryexec : "");

  gnome_dentry_edit_set_icon(dee, dentry->icon);
  if (dee->icon_dialog && dentry->icon)
    gnome_icon_selection_select_icon(GNOME_ICON_SELECTION(gtk_object_get_user_data(GTK_OBJECT(dee))),
				     g_filename_pointer(dee->icon));

  gtk_entry_set_text(GTK_ENTRY(dee->doc_entry), 
		     dentry->docpath ? dentry->docpath : "");

  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(dee->type_combo)->entry),
			       dentry->type ? dentry->type : "");

  gtk_toggle_button_set_state(GTK_TOGGLE_BUTTON(dee->terminal_button),
			      dentry->terminal);
#ifdef GNOME_ENABLE_DEBUG
  g_print("Dialog (sync display): %p\n", dee->icon_dialog);
#endif
}

/* This is a simple-minded string splitter.  It splits on whitespace.
   Something better would be to define a quoting syntax so that the
   user can use quotes and such.  FIXME.  */
static void
gnome_dentry_edit_split (char *text, int *argcp, char ***argvp)
{
  char *p;
  int count = 0;

  /* First pass: find out how large to make the return vector.  */
  for (p = text; *p; ++p) {
    while (*p && isspace (*p))
      ++p;
    if (! *p)
      break;
    while (*p && ! isspace (*p))
      ++p;
    ++count;
  }

  /* Increment count to account for NULL terminator.  Resulting ARGC
     doesn't include NULL terminator, though.  */
  *argcp = count;
  ++count;
  *argvp = (char **) g_malloc (count * sizeof (char *));

  count = 0;
  for (p = text; *p; ++p) {
    char *q;

    while (*p && isspace (*p))
      ++p;
    if (! *p)
      break;

    q = p;
    while (*p && ! isspace (*p))
      ++p;
    (*argvp)[count++] = (char *) strndup (q, p - q);
  }

  (*argvp)[count] = NULL;
}

/* Conform dentry to display */
static void gnome_dentry_edit_sync_dentry(GnomeDEntryEdit * dee,
					  GnomeDesktopEntry * dentry)
{
  gchar * text;

  g_return_if_fail(dee != NULL);
  g_return_if_fail(GNOME_IS_DENTRY_EDIT(dee));
  g_return_if_fail(dentry != NULL);

  text = gtk_entry_get_text(GTK_ENTRY(dee->name_entry));
  g_free(dentry->name);
  if (text[0] != '\0') dentry->name = g_strdup(text);

  text = gtk_entry_get_text(GTK_ENTRY(dee->comment_entry));
  g_free(dentry->comment);
  if (text[0] != '\0') dentry->comment = g_strdup(text);

  text = gtk_entry_get_text(GTK_ENTRY(dee->exec_entry));
  gnome_string_array_free(dentry->exec);
  if (text[0] != '\0') {
    gnome_dentry_edit_split (text, &dentry->exec_length, &dentry->exec);
  } else {
    dentry->exec_length = 0;
  }

  text = gtk_entry_get_text(GTK_ENTRY(dee->tryexec_entry));
  g_free(dentry->tryexec);
  if (text[0] != '\0') dentry->tryexec = g_strdup(text);
  
  text = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(dee->type_combo)->entry));
  g_free(dentry->type);
  if (text[0] != '\0') dentry->type = g_strdup(text);
  
  g_free(dentry->icon);
  if (dee->icon) dentry->icon = g_strdup(g_filename_pointer(dee->icon));

  text = gtk_entry_get_text(GTK_ENTRY(dee->doc_entry));
  g_free(dentry->docpath);
  if (text[0] != '\0') dentry->docpath = g_strdup(text);

  dentry->terminal = 
    GTK_TOGGLE_BUTTON(dee->terminal_button)->active;

  g_free(dentry->location);
  dentry->location = NULL;
  g_free(dentry->geometry);
  dentry->geometry = NULL;
}

void        gnome_dentry_edit_load_file  (GnomeDEntryEdit * dee,
					  const gchar * path)
{
  GnomeDesktopEntry * newentry;

  g_return_if_fail(dee != NULL);
  g_return_if_fail(GNOME_IS_DENTRY_EDIT(dee));
  g_return_if_fail(path != NULL);

  newentry = gnome_desktop_entry_load_unconditional(path);

  if ( newentry ) {
    gnome_dentry_edit_sync_display(dee, newentry);
    gnome_desktop_entry_destroy(newentry);
  }
  else {
    g_warning("Failed to load file into GnomeDEntryEdit");
    return;
  }
}

/* Destroy existing dentry and replace it with this one,
   updating the DEntryEdit to reflect it. */
void        gnome_dentry_edit_set_dentry(GnomeDEntryEdit * dee,
					 GnomeDesktopEntry * dentry)
{
  g_return_if_fail(dee != NULL);
  g_return_if_fail(GNOME_IS_DENTRY_EDIT(dee));
  g_return_if_fail(dentry != NULL);

  gnome_dentry_edit_sync_display(dee, dentry);
}

GnomeDesktopEntry * gnome_dentry_get_dentry(GnomeDEntryEdit * dee)
{
  GnomeDesktopEntry * newentry;

  g_return_val_if_fail(dee != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_DENTRY_EDIT(dee), NULL);

  newentry = g_new0(GnomeDesktopEntry, 1);

  gnome_dentry_edit_sync_dentry(dee, newentry);
  
  return newentry;
}

void        gnome_dentry_edit_clear     (GnomeDEntryEdit * dee)
{
  g_return_if_fail(dee != NULL);
  g_return_if_fail(GNOME_IS_DENTRY_EDIT(dee));
 
  gtk_entry_set_text(GTK_ENTRY(dee->name_entry), "");
  gtk_entry_set_text(GTK_ENTRY(dee->comment_entry),"");
  gtk_entry_set_text(GTK_ENTRY(dee->exec_entry), "");  
  gtk_entry_set_text(GTK_ENTRY(dee->tryexec_entry), "");
  gtk_entry_set_text(GTK_ENTRY(dee->doc_entry), "");

  gnome_dentry_edit_set_icon(dee, NULL);
}

static void gnome_dentry_edit_changed(GnomeDEntryEdit * dee)
{
  gtk_signal_emit(GTK_OBJECT(dee), 
		  dentry_edit_signals[CHANGED],
		  NULL);
}

static void gnome_dentry_edit_icon_changed(GnomeDEntryEdit * dee)
{
  gtk_signal_emit(GTK_OBJECT(dee), 
		  dentry_edit_signals[ICON_CHANGED],
		  NULL);
}

static void gnome_dentry_edit_name_changed(GnomeDEntryEdit * dee)
{
  gtk_signal_emit(GTK_OBJECT(dee), 
		  dentry_edit_signals[NAME_CHANGED],
		  NULL);
}

gchar *     gnome_dentry_edit_get_icon   (GnomeDEntryEdit * dee)
{
  g_return_val_if_fail(dee != NULL, NULL);
 
  return dee->icon;
}

gchar *     gnome_dentry_edit_get_name   (GnomeDEntryEdit * dee)
{
  gchar * name;
  name = gtk_entry_get_text(GTK_ENTRY(dee->name_entry));
  return name;
}

static void gnome_dentry_edit_set_icon(GnomeDEntryEdit * dee,
				       const gchar * icon_name)
{
  g_return_if_fail(dee != NULL);

  g_free(dee->icon);
  dee->icon = NULL;

  if (dee->desktop_icon) gtk_widget_destroy(dee->desktop_icon);
  dee->desktop_icon = NULL;

  if (icon_name == NULL) {
    dee->desktop_icon = gtk_label_new(_("No\nicon"));
    if (!icon_name) icon_name = "";
  }
  else {
    if (g_file_exists(icon_name)) {
      dee->desktop_icon = 
        gnome_pixmap_new_from_file(icon_name);
    }
    else {
      gchar *icon_full_name = gnome_pixmap_file(icon_name);
      dee->desktop_icon = 
        gnome_pixmap_new_from_file(icon_full_name);
      g_free(icon_full_name);
    }
    if (dee->desktop_icon == NULL) {
      dee->desktop_icon = gtk_label_new(_("Couldn't\nload\nicon"));
    }
  }
  
  if ( dee->desktop_icon ) {
    gtk_container_add(GTK_CONTAINER(dee->icon_button), 
		      dee->desktop_icon);
    gtk_widget_show( dee->desktop_icon );
  }

#ifdef GNOME_ENABLE_DEBUG
  g_print("Setting icon name %s\n", g_filename_pointer(icon_name));
#endif

  gtk_label_set(GTK_LABEL(dee->icon_label),
		g_filename_pointer(icon_name));

  if (icon_name[0] != '\0') dee->icon = g_strdup(icon_name);

  gnome_dentry_edit_changed(dee);
  gnome_dentry_edit_icon_changed(dee);
}

static void icon_selected_cb(GtkButton * button, 
			     GnomeDEntryEdit * dee)
{
  const gchar * icon;
  GnomeIconSelection * gis;

  gis =  gtk_object_get_user_data(GTK_OBJECT(dee));
  icon = gnome_icon_selection_get_icon(gis, TRUE);

  if (icon != NULL) 
    gnome_dentry_edit_set_icon(dee, icon);
}

static void show_icon_selection(GtkButton * b, 
				GnomeDEntryEdit * dee)
{
#ifdef GNOME_ENABLE_DEBUG
  g_print("Dialog (show selection): %p\n", dee->icon_dialog);
#endif
  if ( dee->icon_dialog == NULL ) {
    GtkWidget * iconsel;

    dee->icon_dialog = 
      gnome_dialog_new(_("Choose an icon"),
		       GNOME_STOCK_BUTTON_OK,
		       GNOME_STOCK_BUTTON_CANCEL,
		       NULL);
    gnome_dialog_close_hides(GNOME_DIALOG(dee->icon_dialog), TRUE);
    gnome_dialog_set_close  (GNOME_DIALOG(dee->icon_dialog), TRUE);

    gtk_window_set_policy(GTK_WINDOW(dee->icon_dialog), 
			  TRUE, TRUE, TRUE);

    iconsel = gnome_icon_selection_new();

    /* FIXME this is all really broken, need to choose a 
       better directory to look at */
    gnome_icon_selection_add_defaults(GNOME_ICON_SELECTION(iconsel));

    gtk_widget_set_usize(GNOME_ICON_SELECTION(iconsel)->clist , 250, 350);

    gtk_container_add(GTK_CONTAINER(GNOME_DIALOG(dee->icon_dialog)->vbox),
		      iconsel);

    gtk_widget_show_all(dee->icon_dialog);

    gnome_icon_selection_show_icons(GNOME_ICON_SELECTION(iconsel));

    if (dee->icon) gnome_icon_selection_select_icon(GNOME_ICON_SELECTION(iconsel), 
				     g_filename_pointer(dee->icon));

    gnome_dialog_button_connect(GNOME_DIALOG(dee->icon_dialog), 
				0, /* OK button */
				GTK_SIGNAL_FUNC(icon_selected_cb),
				dee);
    gtk_object_set_user_data(GTK_OBJECT(dee), iconsel);
  }
  else {
#ifdef GNOME_ENABLE_DEBUG
    g_print("Already created dialog, just showing it\n");
#endif
    if ( ! GTK_WIDGET_VISIBLE(dee->icon_dialog) )
      gtk_widget_show(dee->icon_dialog);
  }
}

#ifdef TEST_DENTRY_EDIT

#include "libgnomeui.h"

static void changed_callback(GnomeDEntryEdit * dee, gpointer data)
{
  g_print("Changed!\n");
  fflush(stdout);
}

static void icon_changed_callback(GnomeDEntryEdit * dee, 
				  gpointer data)
{
  g_print("Icon changed!\n");
  fflush(stdout);
}

static void name_changed_callback(GnomeDEntryEdit * dee, 
				  gpointer data)
{
  g_print("Name changed!\n");
  fflush(stdout);
}

int main (int argc, char * argv[])
{
  GtkWidget * app;
  GtkWidget * notebook;
  GtkObject * dee;

  argp_program_version = VERSION;

  gnome_init ("testing dentry edit", NULL, argc, argv, 0, 0);

  app = gnome_app_new("testing dentry edit", "Testing");

  notebook = gtk_notebook_new();

  gnome_app_set_contents(GNOME_APP(app), notebook);

  dee = gnome_dentry_edit_new(GTK_NOTEBOOK(notebook));

  gnome_dentry_edit_load_file(GNOME_DENTRY_EDIT(dee),
			      "/usr/local/share/gnome/apps/grun.desktop");

#ifdef GNOME_ENABLE_DEBUG
  g_print("Dialog (main): %p\n", GNOME_DENTRY_EDIT(dee)->icon_dialog);
#endif

  gtk_signal_connect_object(GTK_OBJECT(app), "delete_event", 
			    GTK_SIGNAL_FUNC(gtk_widget_destroy),
			    GTK_OBJECT(app));

  gtk_signal_connect(GTK_OBJECT(app), "destroy",
		     GTK_SIGNAL_FUNC(gtk_main_quit),
		     NULL);

  gtk_signal_connect(GTK_OBJECT(dee), "changed",
		     GTK_SIGNAL_FUNC(changed_callback),
		     NULL);

  gtk_signal_connect(GTK_OBJECT(dee), "icon_changed",
		     GTK_SIGNAL_FUNC(icon_changed_callback),
		     NULL);

  gtk_signal_connect(GTK_OBJECT(dee), "name_changed",
		     GTK_SIGNAL_FUNC(name_changed_callback),
		     NULL);

#ifdef GNOME_ENABLE_DEBUG
  g_print("Dialog (main 2): %p\n", GNOME_DENTRY_EDIT(dee)->icon_dialog);
#endif

  gtk_widget_show(notebook);
  gtk_widget_show(app);

#ifdef GNOME_ENABLE_DEBUG
  g_print("Dialog (main 3): %p\n", GNOME_DENTRY_EDIT(dee)->icon_dialog);
#endif

  gtk_main();

  return 0;
}

#endif

