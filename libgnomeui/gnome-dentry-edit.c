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
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Cambridge, MA 02139, USA.
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
#include "gnome-icon-entry.h"

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

enum {
  CHANGED,
  ICON_CHANGED,
  NAME_CHANGED,
  LAST_SIGNAL
};

static gint dentry_edit_signals[LAST_SIGNAL] = { 0 };

static GtkObjectClass *parent_class;

guint
gnome_dentry_edit_get_type (void)
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

static void 
table_attach_entry(GtkTable * table, GtkWidget * w,
		   gint l, gint r, gint t, gint b)
{
  gtk_table_attach(table, w, l, r, t, b,
		   GTK_EXPAND | GTK_FILL | GTK_SHRINK,
		   GTK_FILL,
		   0, 0);
}

static void
table_attach_label(GtkTable * table, GtkWidget * w,
		   gint l, gint r, gint t, gint b)
{
  gtk_table_attach(table, w, l, r, t, b,
		   GTK_FILL,
		   GTK_FILL,
		   0, 0);
}

static void 
table_attach_list(GtkTable * table, GtkWidget * w,
		  gint l, gint r, gint t, gint b)
{
  gtk_table_attach(table, w, l, r, t, b,
		   GTK_EXPAND | GTK_FILL | GTK_SHRINK,
		   GTK_EXPAND | GTK_FILL | GTK_SHRINK,
		   0, 0);
}

static GtkWidget *
label_new (char *text)
{
  GtkWidget *label;

  label = gtk_label_new (text);
  gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
  return label;
}

static void
fill_easy_page(GnomeDEntryEdit * dee, GtkWidget * table)
{
  GtkWidget *label;
  GList *types = NULL;
  GtkWidget *e;
  GtkWidget *hbox;
  GtkWidget *align;

  label = label_new(_("Name:"));
  table_attach_label(GTK_TABLE(table), label, 0, 1, 0, 1);

  dee->name_entry = gtk_entry_new();
  table_attach_entry(GTK_TABLE(table),dee->name_entry, 1, 2, 0, 1);
  gtk_signal_connect_object_while_alive(GTK_OBJECT(dee->name_entry), "changed",
					GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
					GTK_OBJECT(dee));
  gtk_signal_connect_object_while_alive(GTK_OBJECT(dee->name_entry), "changed",
					GTK_SIGNAL_FUNC(gnome_dentry_edit_name_changed),
					GTK_OBJECT(dee));


  label = label_new(_("Comment:"));
  table_attach_label(GTK_TABLE(table),label, 0, 1, 1, 2);

  dee->comment_entry = gtk_entry_new();
  table_attach_entry(GTK_TABLE(table),dee->comment_entry, 1, 2, 1, 2);
  gtk_signal_connect_object_while_alive(GTK_OBJECT(dee->comment_entry), "changed",
					GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
					GTK_OBJECT(dee));


  label = label_new(_("Command:"));
  table_attach_label(GTK_TABLE(table),label, 0, 1, 2, 3);

  dee->exec_entry = gtk_entry_new();
  table_attach_entry(GTK_TABLE(table),dee->exec_entry, 1, 2, 2, 3);
  gtk_signal_connect_object_while_alive(GTK_OBJECT(dee->exec_entry), "changed",
					GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
					GTK_OBJECT(dee));


  label = label_new(_("Type:"));
  table_attach_label(GTK_TABLE(table), label, 0, 1, 3, 4);

  types = g_list_prepend(types, "Application");
  types = g_list_prepend(types, "Directory");
  dee->type_combo = gtk_combo_new();
  gtk_combo_set_popdown_strings(GTK_COMBO(dee->type_combo), types);
  g_list_free(types);
  gtk_combo_set_value_in_list(GTK_COMBO(dee->type_combo), 
			      FALSE, TRUE);
  table_attach_entry(GTK_TABLE(table),dee->type_combo, 1, 2, 3, 4);
  gtk_signal_connect_object_while_alive(GTK_OBJECT(GTK_COMBO(dee->type_combo)->entry), 
					"changed",
					GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
					GTK_OBJECT(dee));

  label = label_new(_("Icon:"));
  table_attach_label(GTK_TABLE(table), label, 0, 1, 4, 5);

  hbox = gtk_hbox_new(FALSE, GNOME_PAD_BIG);
  gtk_table_attach(GTK_TABLE(table), hbox,
		   1, 2, 4, 5,
		   GTK_EXPAND | GTK_FILL,
		   0,
		   0, 0);

  dee->icon_entry = gnome_icon_entry_new("icon",_("Choose an icon"));
  e = gnome_icon_entry_gtk_entry(GNOME_ICON_ENTRY(dee->icon_entry));
  gtk_signal_connect_object_while_alive(GTK_OBJECT(e),"changed",
					GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
					GTK_OBJECT(dee));
  gtk_signal_connect_object_while_alive(GTK_OBJECT(e),"changed",
					GTK_SIGNAL_FUNC(gnome_dentry_edit_icon_changed),
					GTK_OBJECT(dee));
  gtk_box_pack_start(GTK_BOX(hbox), dee->icon_entry, FALSE, FALSE, 0);

  align = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
  gtk_box_pack_start(GTK_BOX(hbox), align, FALSE, FALSE, 0);

  dee->terminal_button = gtk_check_button_new_with_label (_("Run in Terminal"));
  gtk_signal_connect_object(GTK_OBJECT(dee->terminal_button), 
			    "clicked",
			    GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
			    GTK_OBJECT(dee));
  gtk_container_add(GTK_CONTAINER(align), dee->terminal_button);
}

static void
add_mime_type(GtkWidget *button, GnomeDEntryEdit *dee)
{
	int i;
	char *mime;
	char *exec;
	char *text[2];
	GtkCList *cl = GTK_CLIST(dee->dnd_list);
	mime = gtk_entry_get_text(GTK_ENTRY(dee->dnd_mime_entry));
	exec = gtk_entry_get_text(GTK_ENTRY(dee->dnd_exec_entry));
	if(!mime || !*mime || !exec || !*exec)
		return;
	for(i=0;i<cl->rows;i++) {
		char *s;
		gtk_clist_get_text(cl,i,0,&s);
		if(strcmp(mime,s)==0) {
			gtk_clist_set_text(cl,i,1,exec);
			gtk_signal_emit(GTK_OBJECT(dee),
					dentry_edit_signals[CHANGED], NULL);
			return;
		}
	}
	text[0]=mime;
	text[1]=exec;
	gtk_clist_append(cl,text);
	gtk_signal_emit(GTK_OBJECT(dee), dentry_edit_signals[CHANGED], NULL);
}
static void
dnd_list_select_row(GtkCList *cl, int row, int column, GdkEvent *event, GnomeDEntryEdit *dee)
{
	char *mime;
	char *exec;
	gtk_clist_get_text(cl,row,0,&mime);
	gtk_clist_get_text(cl,row,1,&exec);
	
	gtk_entry_set_text(GTK_ENTRY(dee->dnd_mime_entry),
			   mime);
	gtk_entry_set_text(GTK_ENTRY(dee->dnd_exec_entry),
			   exec);
}
static void
remove_mime_type(GtkWidget *button, GnomeDEntryEdit *dee)
{
	GtkCList *cl = GTK_CLIST(dee->dnd_list);
	if(cl->selection == NULL)
		return;
	gtk_clist_remove(cl,GPOINTER_TO_INT(cl->selection->data));
	gtk_signal_emit(GTK_OBJECT(dee), dentry_edit_signals[CHANGED], NULL);
}

static void
fill_advanced_page(GnomeDEntryEdit * dee, GtkWidget * page)
{
  GtkWidget * label;
  GtkWidget * button;
  GtkWidget * box;
  GtkWidget * entry;
  GtkWidget * sw;
  char *titles[2];

  label = label_new(_("Try this before using:"));
  table_attach_label(GTK_TABLE(page),label, 0, 1, 0, 1);

  dee->tryexec_entry = gtk_entry_new();
  table_attach_entry(GTK_TABLE(page),dee->tryexec_entry, 1, 2, 0, 1);
  gtk_signal_connect_object(GTK_OBJECT(dee->tryexec_entry), 
			    "changed",
			    GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
			    GTK_OBJECT(dee));

  label = label_new(_("Documentation:"));
  table_attach_label(GTK_TABLE(page),label, 0, 1, 1, 2);

  dee->doc_entry = gtk_entry_new_with_max_length(255);
  table_attach_entry(GTK_TABLE(page),dee->doc_entry, 1, 2, 1, 2);
  gtk_signal_connect_object(GTK_OBJECT(dee->doc_entry), 
			    "changed",
			    GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
			    GTK_OBJECT(dee));

  label = label_new(_("WM classes of windows that pop up:"));
  table_attach_label(GTK_TABLE(page),label, 0, 1, 2, 3);

  dee->wm_classes_entry = gtk_entry_new();
  table_attach_entry(GTK_TABLE(page),dee->wm_classes_entry, 1, 2, 2, 3);
  gtk_signal_connect_object(GTK_OBJECT(dee->wm_classes_entry), 
			    "changed",
			    GTK_SIGNAL_FUNC(gnome_dentry_edit_changed),
			    GTK_OBJECT(dee));

  label = gtk_label_new(_("Drag and drop actions:"));
  table_attach_label(GTK_TABLE(page),label, 0, 2, 3, 4);
  
  sw = gtk_scrolled_window_new(NULL,NULL);
  titles[0]=_("Mime type");
  titles[1]=_("Action");
  dee->dnd_list = gtk_clist_new_with_titles(2,titles);
  gtk_clist_column_titles_passive(GTK_CLIST(dee->dnd_list));
  gtk_signal_connect(GTK_OBJECT(dee->dnd_list),"select_row",
		     GTK_SIGNAL_FUNC(dnd_list_select_row),
		     dee);
  gtk_container_add(GTK_CONTAINER(sw),dee->dnd_list);

  gtk_widget_set_usize(sw,0,100);
  table_attach_list(GTK_TABLE(page),sw, 0, 2, 4, 5);
  
  box = gtk_hbox_new(FALSE,GNOME_PAD_SMALL);
  table_attach_entry(GTK_TABLE(page),box, 0, 2, 5, 6);

  label = gtk_label_new(_("Mime:"));
  gtk_box_pack_start(GTK_BOX(box),label,FALSE,FALSE,0);
  
  dee->dnd_mime_entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(box),dee->dnd_mime_entry,TRUE,TRUE,0);
  
  label = gtk_label_new(_("Exec:"));
  gtk_box_pack_start(GTK_BOX(box),label,FALSE,FALSE,0);
  
  dee->dnd_exec_entry = gtk_entry_new();
  gtk_box_pack_start(GTK_BOX(box),dee->dnd_exec_entry,TRUE,TRUE,0);

  dee->dnd_button_box = box = gtk_hbox_new(FALSE,GNOME_PAD_SMALL);
  table_attach_entry(GTK_TABLE(page),box, 0, 2, 6, 7);
  
  button = gtk_button_new_with_label(_("Add"));
  gtk_signal_connect(GTK_OBJECT(button),"clicked",
		     GTK_SIGNAL_FUNC(add_mime_type),
		     dee);
  gtk_box_pack_start(GTK_BOX(box),button,FALSE,FALSE,0);

  button = gtk_button_new_with_label(_("Remove"));
  gtk_signal_connect(GTK_OBJECT(button),"clicked",
		     GTK_SIGNAL_FUNC(remove_mime_type),
		     dee);
  gtk_box_pack_start(GTK_BOX(box),button,FALSE,FALSE,0);
}

static GtkWidget *
make_page(void)
{
  GtkWidget * frame, * page;

  frame = gtk_frame_new (NULL);
  gtk_container_set_border_width (GTK_CONTAINER(frame), GNOME_PAD_SMALL);

  /*the rows field will expand as necessary*/
  page = gtk_table_new (1, 2, FALSE);
  gtk_container_set_border_width (GTK_CONTAINER (page), GNOME_PAD_SMALL);
  gtk_table_set_row_spacings (GTK_TABLE (page), GNOME_PAD_SMALL);
  gtk_table_set_col_spacings (GTK_TABLE (page), GNOME_PAD_SMALL);

  gtk_container_add (GTK_CONTAINER (frame), page);

  return frame;
} 

static void
gnome_dentry_edit_init (GnomeDEntryEdit *dee)
{
  dee->child1       = NULL;
  dee->child2       = NULL;
}

GtkObject *
gnome_dentry_edit_new (void)
{
  GnomeDEntryEdit * dee;

  dee = gtk_type_new(gnome_dentry_edit_get_type());

  dee->child1 = make_page();
  fill_easy_page(dee, GTK_BIN(dee->child1)->child);
  gtk_widget_show_all(dee->child1);

  dee->child2 = make_page();
  fill_advanced_page(dee, GTK_BIN(dee->child2)->child);
  gtk_widget_show_all(dee->child2);

  return GTK_OBJECT (dee);
}


GtkObject *
gnome_dentry_edit_new_notebook (GtkNotebook *notebook)
{
  GnomeDEntryEdit * dee;

  g_return_val_if_fail(notebook != NULL, NULL);
  
  dee = GNOME_DENTRY_EDIT(gnome_dentry_edit_new());

  gtk_notebook_append_page (GTK_NOTEBOOK(notebook), 
			    dee->child1, 
			    gtk_label_new(_("Basic")));

  gtk_notebook_append_page (GTK_NOTEBOOK(notebook), 
			    dee->child2, 
			    gtk_label_new(_("Advanced")));

  /* Destroy self with the notebook. */
  gtk_signal_connect_object(GTK_OBJECT(notebook), "destroy",
			    GTK_SIGNAL_FUNC(gtk_object_destroy),
			    GTK_OBJECT(dee));

  return GTK_OBJECT (dee);
}

static void
gnome_dentry_edit_destroy (GtkObject *dee)
{
  g_return_if_fail(dee != NULL);
  g_return_if_fail(GNOME_IS_DENTRY_EDIT(dee));

  if (GTK_OBJECT_CLASS(parent_class)->destroy)
    (* (GTK_OBJECT_CLASS(parent_class)->destroy))(dee);
}

/* Conform display to dentry */
static void
gnome_dentry_edit_sync_display(GnomeDEntryEdit *dee,
			       GnomeDesktopEntry *dentry)
{
  gchar * s = NULL;
  GList *li;

  g_return_if_fail(dee != NULL);
  g_return_if_fail(GNOME_IS_DENTRY_EDIT(dee));

  gtk_entry_set_text(GTK_ENTRY(dee->name_entry), 
		     dentry->name ? dentry->name : "");
  gtk_entry_set_text(GTK_ENTRY(dee->comment_entry),
		     dentry->comment ? dentry->comment : "");
  
  if (dentry->exec)
    s = g_flatten_vector(" ", dentry->exec);
  else
    s = NULL;
  gtk_entry_set_text(GTK_ENTRY(dee->exec_entry), s ? s : "");
  g_free(s);

  gtk_entry_set_text(GTK_ENTRY(dee->tryexec_entry), 
		     dentry->tryexec ? dentry->tryexec : "");

  gnome_icon_entry_set_icon(GNOME_ICON_ENTRY(dee->icon_entry),
			    dentry->icon ? dentry->icon : "");

  gtk_entry_set_text(GTK_ENTRY(dee->doc_entry), 
		     dentry->docpath ? dentry->docpath : "");

  gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(dee->type_combo)->entry),
		     dentry->type ? dentry->type : "");

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dee->terminal_button),
			       dentry->terminal);
  
  if (dentry->wm_classes)
    s = g_flatten_vector(" ", dentry->wm_classes);
  else
    s = NULL;
  gtk_entry_set_text(GTK_ENTRY(dee->wm_classes_entry), s ? s : "");
  g_free(s);
  
  gtk_clist_clear(GTK_CLIST(dee->dnd_list));
  for(li=dentry->dnd_entries;li;li=g_list_next(li)) {
	  char **argv = li->data;
	  char *text[2];
	  if(!argv || !argv[0] || !*argv[0] || !argv[1]) continue;
	  text[0] = argv[0];
	  text[1] = g_flatten_vector(" ", &argv[1]);
	  gtk_clist_append(GTK_CLIST(dee->dnd_list),text);
	  g_free(text[1]);
  }
  gtk_entry_set_text(GTK_ENTRY(dee->dnd_mime_entry),"");
  gtk_entry_set_text(GTK_ENTRY(dee->dnd_exec_entry),"");
}

/* Conform dentry to display */
static void
gnome_dentry_edit_sync_dentry(GnomeDEntryEdit *dee,
			      GnomeDesktopEntry *dentry)
{
  gchar * text;
  int i;

  g_return_if_fail(dee != NULL);
  g_return_if_fail(GNOME_IS_DENTRY_EDIT(dee));
  g_return_if_fail(dentry != NULL);

  text = gtk_entry_get_text(GTK_ENTRY(dee->name_entry));
  g_free(dentry->name);
  if (text[0] != '\0')
    dentry->name = g_strdup(text);
  else
    dentry->name = NULL;

  text = gtk_entry_get_text(GTK_ENTRY(dee->comment_entry));
  g_free(dentry->comment);
  if (text[0] != '\0')
    dentry->comment = g_strdup(text);
  else
    dentry->comment = NULL;

  text = gtk_entry_get_text(GTK_ENTRY(dee->exec_entry));
  g_strfreev(dentry->exec);
  if (text[0] != '\0')
    gnome_config_make_vector(text, &dentry->exec_length, &dentry->exec);
  else {
    dentry->exec_length = 0;
    dentry->exec = NULL;
  }

  text = gtk_entry_get_text(GTK_ENTRY(dee->tryexec_entry));
  g_free(dentry->tryexec);
  if (text[0] != '\0')
    dentry->tryexec = g_strdup(text);
  else
    dentry->tryexec = NULL;
  
  text = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(dee->type_combo)->entry));
  g_free(dentry->type);
  if (text[0] != '\0')
    dentry->type = g_strdup(text);
  else
    dentry->type = NULL;
  
  g_free(dentry->icon);
  dentry->icon = gnome_icon_entry_get_filename(GNOME_ICON_ENTRY(dee->icon_entry));

  text = gtk_entry_get_text(GTK_ENTRY(dee->doc_entry));
  g_free(dentry->docpath);
  if (text[0] != '\0')
    dentry->docpath = g_strdup(text);
  else
    dentry->docpath = NULL;

  dentry->terminal = GTK_TOGGLE_BUTTON(dee->terminal_button)->active;

  g_free(dentry->location);
  dentry->location = NULL;
  g_free(dentry->geometry);
  dentry->geometry = NULL;

  /*maybe in the future we should also edit KDE dentries*/
  dentry->is_kde = FALSE;

  text = gtk_entry_get_text(GTK_ENTRY(dee->wm_classes_entry));
  g_strfreev(dentry->wm_classes);
  if (text[0] != '\0')
    gnome_config_make_vector(text, &dentry->wm_classes_length, &dentry->wm_classes);
  else {
    dentry->wm_classes_length = 0;
    dentry->wm_classes = NULL;
  }
  
  if(dentry->dnd_entries) {
	  g_list_foreach(dentry->dnd_entries,(GFunc)g_strfreev,NULL);
	  g_list_free(dentry->dnd_entries);
	  dentry->dnd_entries = NULL;
  }
  for(i=0;i<GTK_CLIST(dee->dnd_list)->rows;i++) {
	  char *mime,*exec;
	  char **argv;
	  int argc;
	  gtk_clist_get_text(GTK_CLIST(dee->dnd_list),
			     i,0,&mime);
	  gtk_clist_get_text(GTK_CLIST(dee->dnd_list),
			     i,1,&exec);
	  text = g_strconcat(mime," ",exec,NULL);
	  gnome_config_make_vector(text,&argc,&argv);
	  g_free(text);
	  dentry->dnd_entries = g_list_append(dentry->dnd_entries,argv);
  }
}

void
gnome_dentry_edit_load_file(GnomeDEntryEdit *dee,
			    const gchar *path)
{
  GnomeDesktopEntry * newentry;

  g_return_if_fail(dee != NULL);
  g_return_if_fail(GNOME_IS_DENTRY_EDIT(dee));
  g_return_if_fail(path != NULL);

  newentry = gnome_desktop_entry_load_unconditional(path);

  if (newentry) {
    gnome_dentry_edit_sync_display(dee, newentry);
    gnome_desktop_entry_destroy(newentry);
  } else {
    g_warning("Failed to load file into GnomeDEntryEdit");
    return;
  }
}

/* Destroy existing dentry and replace it with this one,
   updating the DEntryEdit to reflect it. */
void
gnome_dentry_edit_set_dentry(GnomeDEntryEdit *dee,
			     GnomeDesktopEntry *dentry)
{
  g_return_if_fail(dee != NULL);
  g_return_if_fail(GNOME_IS_DENTRY_EDIT(dee));
  g_return_if_fail(dentry != NULL);

  gnome_dentry_edit_sync_display(dee, dentry);
}

GnomeDesktopEntry *
gnome_dentry_get_dentry(GnomeDEntryEdit *dee)
{
  GnomeDesktopEntry * newentry;

  g_return_val_if_fail(dee != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_DENTRY_EDIT(dee), NULL);

  newentry = g_new0(GnomeDesktopEntry, 1);
  gnome_dentry_edit_sync_dentry(dee, newentry);
  return newentry;
}

void
gnome_dentry_edit_clear(GnomeDEntryEdit *dee)
{
  g_return_if_fail(dee != NULL);
  g_return_if_fail(GNOME_IS_DENTRY_EDIT(dee));

  gtk_entry_set_text(GTK_ENTRY(dee->name_entry), "");
  gtk_entry_set_text(GTK_ENTRY(dee->comment_entry),"");
  gtk_entry_set_text(GTK_ENTRY(dee->exec_entry), "");  
  gtk_entry_set_text(GTK_ENTRY(dee->tryexec_entry), "");
  gtk_entry_set_text(GTK_ENTRY(dee->doc_entry), "");
  gtk_entry_set_text(GTK_ENTRY(dee->wm_classes_entry), "");
  gtk_entry_set_text(GTK_ENTRY(dee->dnd_mime_entry), "");
  gtk_entry_set_text(GTK_ENTRY(dee->dnd_exec_entry), "");
  gtk_clist_clear(GTK_CLIST(dee->dnd_list));
  gnome_icon_entry_set_icon(GNOME_ICON_ENTRY(dee->icon_entry),"");
}

static void
gnome_dentry_edit_changed(GnomeDEntryEdit *dee)
{
  gtk_signal_emit(GTK_OBJECT(dee), dentry_edit_signals[CHANGED], NULL);
}

static void
gnome_dentry_edit_icon_changed(GnomeDEntryEdit *dee)
{
  gtk_signal_emit(GTK_OBJECT(dee), dentry_edit_signals[ICON_CHANGED], NULL);
}

static void
gnome_dentry_edit_name_changed(GnomeDEntryEdit *dee)
{
  gtk_signal_emit(GTK_OBJECT(dee), dentry_edit_signals[NAME_CHANGED], NULL);
}

gchar *
gnome_dentry_edit_get_icon(GnomeDEntryEdit *dee)
{
  g_return_val_if_fail(dee != NULL, NULL);
 
  return gnome_icon_entry_get_filename(GNOME_ICON_ENTRY(dee->icon_entry));
}

gchar *
gnome_dentry_edit_get_name (GnomeDEntryEdit *dee)
{
  gchar * name;

  name = gtk_entry_get_text(GTK_ENTRY(dee->name_entry));
  return g_strdup(name);
}

GtkWidget *
gnome_dentry_get_name_entry (GnomeDEntryEdit *dee)
{
  return dee->name_entry;
}

GtkWidget *
gnome_dentry_get_comment_entry (GnomeDEntryEdit *dee)
{
  return dee->comment_entry;
}

GtkWidget *
gnome_dentry_get_exec_entry (GnomeDEntryEdit *dee)
{
  return dee->exec_entry;
}

GtkWidget *
gnome_dentry_get_tryexec_entry (GnomeDEntryEdit *dee)
{
  return dee->tryexec_entry;
}

GtkWidget *
gnome_dentry_get_doc_entry (GnomeDEntryEdit *dee)
{
  return dee->doc_entry;
}

GtkWidget *
gnome_dentry_get_icon_entry (GnomeDEntryEdit *dee)
{
  return dee->icon_entry;
}

#ifdef TEST_DENTRY_EDIT

#include "libgnomeui.h"

static void
changed_callback(GnomeDEntryEdit *dee, gpointer data)
{
  g_print("Changed!\n");
  fflush(stdout);
}

static void
icon_changed_callback(GnomeDEntryEdit *dee, gpointer data)
{
  g_print("Icon changed!\n");
  fflush(stdout);
}

static void
name_changed_callback(GnomeDEntryEdit *dee, gpointer data)
{
  g_print("Name changed!\n");
  fflush(stdout);
}

int
main(int argc, char * argv[])
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

