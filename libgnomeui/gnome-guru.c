/* gnome-guru.c: Copyright (C) 1998 Free Software Foundation
 * Written by: Havoc Pennington
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

#include "libgnome/libgnome.h"
#include "gnome-uidefs.h"
#include "gnome-stock.h"

#include "gnome-guru.h"

struct _GnomeGuruPage {
  GtkWidget * widget;
  gchar * name;
};

typedef struct _GnomeGuruPage GnomeGuruPage;

static void gnome_guru_class_init (GnomeGuruClass *klass);
static void gnome_guru_init       (GnomeGuru      *guru);

static void gnome_guru_destroy (GtkObject *guru);

static void     gnome_guru_next_clicked  (GtkButton* next, GnomeGuru* guru);
static void     gnome_guru_back_clicked  (GtkButton* back, GnomeGuru* guru);
static void     gnome_guru_cancel_clicked(GtkButton* cancel, GnomeGuru* guru); 
static gboolean gnome_guru_dialog_closed(GnomeDialog* dialog, GnomeGuru* guru);

static GtkVBoxClass *parent_class;


enum {
  CANCELLED,
  FINISHED,
  LAST_SIGNAL
};

guint guru_signals[LAST_SIGNAL] = { 0, 0 };

guint
gnome_guru_get_type ()
{
  static guint guru_type = 0;

  if (!guru_type)
    {
      GtkTypeInfo guru_info =
      {
	"GnomeGuru",
	sizeof (GnomeGuru),
	sizeof (GnomeGuruClass),
	(GtkClassInitFunc) gnome_guru_class_init,
	(GtkObjectInitFunc) gnome_guru_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
      };

      guru_type = gtk_type_unique (gtk_vbox_get_type (), &guru_info);
    }

  return guru_type;
}

static void
gnome_guru_class_init (GnomeGuruClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;

  object_class = (GtkObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  guru_signals[CANCELLED] =
    gtk_signal_new ("cancelled",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeGuruClass, cancelled),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);

  guru_signals[FINISHED] =
    gtk_signal_new ("finished",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeGuruClass, finished),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, guru_signals, 
				LAST_SIGNAL);

  object_class->destroy = gnome_guru_destroy;
}

static void
gnome_guru_init (GnomeGuru *guru)
{
  GtkWidget* title_rule;

  guru->graphic = NULL;
  guru->pages = NULL;
  guru->current_page = NULL;
  guru->has_dialog = FALSE;
  guru->buttonbox = NULL;

  guru->page_box = gtk_vbox_new(FALSE, 0);

  guru->next = gnome_stock_button(GNOME_STOCK_BUTTON_NEXT);
  guru->back = gnome_stock_button(GNOME_STOCK_BUTTON_PREV);
  guru->finish = gnome_stock_button(GNOME_STOCK_BUTTON_OK);

  gtk_signal_connect(GTK_OBJECT(guru->next),
		     "clicked",
		     GTK_SIGNAL_FUNC(gnome_guru_next_clicked),
		     guru);

  gtk_signal_connect(GTK_OBJECT(guru->finish),
		     "clicked",
		     GTK_SIGNAL_FUNC(gnome_guru_next_clicked),
		     guru);

  gtk_signal_connect(GTK_OBJECT(guru->back),
		     "clicked",
		     GTK_SIGNAL_FUNC(gnome_guru_back_clicked),
		     guru);

  gtk_widget_show(guru->next);
  gtk_widget_show(guru->back);
  gtk_widget_show(guru->finish);

  guru->page_title = gtk_label_new("");

  gtk_widget_set_name(guru->page_title, "PageTitle");

  gtk_misc_set_alignment(GTK_MISC(guru->page_title), 0.0, 0.5);

  gtk_widget_show(guru->page_title);

  title_rule = gtk_hseparator_new();

  gtk_widget_show(title_rule);

  gtk_box_pack_start(GTK_BOX(guru->page_box),
		     guru->page_title,
		     FALSE, FALSE, 0);

  gtk_box_pack_start(GTK_BOX(guru->page_box),
		     title_rule,
		     FALSE, FALSE, GNOME_PAD_SMALL);

}

void        
gnome_guru_construct                 (GnomeGuru   * guru,
				      const gchar * name, 
				      GtkWidget   * graphic,
				      GnomeDialog * dialog)
{
  GtkWidget* cancel;
  GtkWidget* hbox;

  g_return_if_fail(guru != NULL);
  g_return_if_fail(GNOME_IS_GURU(guru));

  cancel = gnome_stock_button(GNOME_STOCK_BUTTON_CANCEL);

  gtk_widget_show(cancel);

  gtk_signal_connect(GTK_OBJECT(cancel),
		     "clicked",
		     GTK_SIGNAL_FUNC(gnome_guru_cancel_clicked),
		     guru);

  if (dialog != NULL) {
    guru->has_dialog = TRUE;

    /* Don't copy this dialog code; it uses the internals of the widget,
       and is evil. */
    guru->buttonbox = dialog->action_area;

    gnome_dialog_set_close(dialog, FALSE);

    gtk_signal_connect(GTK_OBJECT(dialog),
		       "close",
		       GTK_SIGNAL_FUNC(gnome_guru_dialog_closed),
		       guru);
    
    gtk_box_pack_start(GTK_BOX(dialog->vbox),
		       GTK_WIDGET(guru), TRUE, TRUE, 0);
  }
  else {
    GtkWidget* seplabel;
    GtkWidget* sepbox;
    GtkWidget* sep;

    guru->buttonbox = gtk_hbutton_box_new();

    gtk_container_set_border_width(GTK_CONTAINER(guru->buttonbox),
			       GNOME_PAD_SMALL);

    gtk_box_pack_end(GTK_BOX(guru),
		     guru->buttonbox,
		     FALSE, FALSE, 0);

    sep = gtk_hseparator_new();    

    gtk_widget_show(sep);

    if (name) {
      seplabel = gtk_label_new(name);
      sepbox   = gtk_hbox_new(FALSE, GNOME_PAD_SMALL);

      gtk_widget_show(seplabel);
      gtk_widget_show(sepbox);

      gtk_box_pack_start(GTK_BOX(sepbox),
			 seplabel,
			 FALSE, FALSE, GNOME_PAD_SMALL);
      gtk_box_pack_end  (GTK_BOX(sepbox),
			 sep,
			 TRUE, TRUE, GNOME_PAD_SMALL);
      gtk_box_pack_end  (GTK_BOX(guru),
			 sepbox,
			 FALSE, FALSE, GNOME_PAD_SMALL);
    }
    else {
      gtk_box_pack_end  (GTK_BOX(guru),
			 sep,
			 FALSE, FALSE, GNOME_PAD_SMALL);
    }
  }

  gtk_box_pack_start(GTK_BOX(guru->buttonbox),
		     cancel,
		     FALSE, FALSE, 0);
  gtk_box_pack_end  (GTK_BOX(guru->buttonbox),
		     guru->back,
		     FALSE, FALSE, 0);
  gtk_box_pack_end  (GTK_BOX(guru->buttonbox),
		     guru->next,
		     FALSE, FALSE, 0);
  gtk_button_box_set_layout(GTK_BUTTON_BOX(guru->buttonbox),
			    GTK_BUTTONBOX_EDGE);

  gtk_button_box_set_spacing(GTK_BUTTON_BOX(guru->buttonbox),
			     GNOME_PAD);

  gtk_widget_show_all(guru->buttonbox);
  
  if (graphic) {
    GtkWidget* frame;
    frame = gtk_frame_new(NULL);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

    gtk_container_add(GTK_CONTAINER(frame),
		      graphic);

    hbox = gtk_hbox_new(FALSE,0);
    
    gtk_box_pack_start(GTK_BOX(dialog ? dialog->vbox : GTK_WIDGET(guru)),
		       hbox,
		       TRUE, TRUE, GNOME_PAD_SMALL);
    gtk_box_pack_start(GTK_BOX(hbox),
		       frame,
		       FALSE, FALSE, GNOME_PAD_SMALL);
    gtk_box_pack_end  (GTK_BOX(hbox),
		       guru->page_box,
		       TRUE, TRUE, GNOME_PAD_SMALL);
    gtk_widget_show_all(hbox);
  }
  else {
    gtk_box_pack_start(GTK_BOX(dialog ? dialog->vbox : GTK_WIDGET(guru)),
		       guru->page_box,
		       TRUE, TRUE, 0);
    gtk_widget_show_all(guru->page_box);
  }

  /* Since we will keep pulling these out of containers */
  gtk_widget_ref(guru->finish);
  gtk_widget_ref(guru->next);
}

GtkWidget* 
gnome_guru_new (const gchar * name,
		GtkWidget   * graphic,
		GnomeDialog * dialog)
{
  GnomeGuru * guru;
  
  guru = gtk_type_new(gnome_guru_get_type());

  gnome_guru_construct(guru, name, graphic, dialog);

  return GTK_WIDGET (guru);
}

static void 
gnome_guru_destroy (GtkObject *object)
{
  GList* tmp;
  GnomeGuru* guru = GNOME_GURU(object);

  g_return_if_fail(guru != NULL);
  g_return_if_fail(GNOME_IS_GURU(guru));

  tmp = guru->pages;
  while (tmp) {
    GnomeGuruPage* page = (GnomeGuruPage*)tmp->data;
    gtk_widget_unref(page->widget);
    g_free(page->name);
    g_free(page);
    tmp->data = NULL;
    tmp = g_list_next(tmp);
  }

  g_list_free(guru->pages);
  guru->pages = NULL;

  /* Should destroy the one that's not in a container? */
  gtk_widget_unref(guru->finish);
  gtk_widget_unref(guru->next);

  if (GTK_OBJECT_CLASS(parent_class)->destroy)
    (* (GTK_OBJECT_CLASS(parent_class)->destroy))(object);
}

static void
flip_to_page(GnomeGuru * guru,
	     GnomeGuruPage * page)
{
  GList* last;

  g_return_if_fail(guru->pages != NULL);

  if (guru->current_page && (page != guru->current_page)) {
    gtk_widget_hide(guru->current_page->widget);
    gtk_container_remove(GTK_CONTAINER(guru->page_box),
			 guru->current_page->widget);
  }
  
  if (page->widget->parent == NULL) {
    gtk_box_pack_end(GTK_BOX(guru->page_box),
		     page->widget,
		     TRUE, TRUE, 0);
    
    guru->current_page = page;
  }

  /* Install finish instead of next if the current page is the last item */
  last = g_list_last(guru->pages);
  if (last && (last->data == page)) {
    if (guru->finish->parent == NULL) {
      gtk_box_pack_end  (GTK_BOX(guru->buttonbox),
			 guru->finish,
			 FALSE, FALSE, 0);      
      gtk_box_reorder_child(GTK_BOX(guru->buttonbox),
			    guru->finish, 3);
    }
    gtk_widget_hide(guru->next);
    gtk_widget_show(guru->finish);
  }
  else {
    if (guru->next->parent == NULL) {
      gtk_box_pack_end  (GTK_BOX(guru->buttonbox),
			 guru->next,
			 FALSE, FALSE, 0);      
      gtk_box_reorder_child(GTK_BOX(guru->buttonbox),
			    guru->next, 3);
    }
    gtk_widget_hide(guru->finish);
    gtk_widget_show(guru->next);
  }

  /* Don't enable Back if this is the first item. */
  if (guru->pages->data == page) {
    gtk_widget_hide(guru->back);
  }
  else {
    gtk_widget_show(guru->back);
  }

  gtk_label_set_text(GTK_LABEL(guru->page_title), page->name);

  gtk_widget_show(page->widget);
}

void        
gnome_guru_append_page               (GnomeGuru   * guru,
				      const gchar * name,
				      GtkWidget   * widget)
{
  GnomeGuruPage* new_page;

  g_return_if_fail(guru != NULL);
  g_return_if_fail(GNOME_IS_GURU(guru));
  g_return_if_fail(name != NULL);
  g_return_if_fail(widget != NULL);
  g_return_if_fail(GTK_IS_WIDGET(widget)); 
  g_return_if_fail(!GTK_WIDGET_VISIBLE(widget));

  new_page = g_new0(GnomeGuruPage,1);

  new_page->widget = widget;
  new_page->name   = g_strdup(name);

  gtk_widget_ref(widget);

  guru->pages = g_list_append(guru->pages, new_page);

  /* Gross */
  if ( (guru->pages->data == new_page) ||
       (guru->pages->next && (guru->pages->next->data == new_page)) ) {
    /* The first page was just added, or the second page was added so the 
       first page is no longer the last page; in either case we have to 
       rearrange our buttons etc. */
    flip_to_page(guru, (GnomeGuruPage*)guru->pages->data);
  }
}

void        
gnome_guru_next_set_sensitive        (GnomeGuru   * guru,
				      gboolean      sensitivity)
{
  g_return_if_fail(guru != NULL);
  g_return_if_fail(GNOME_IS_GURU(guru));

  gtk_widget_set_sensitive(guru->next, sensitivity);
  gtk_widget_set_sensitive(guru->finish, sensitivity);
}

void        
gnome_guru_back_set_sensitive        (GnomeGuru   * guru,
				      gboolean      sensitivity)
{
  g_return_if_fail(guru != NULL);
  g_return_if_fail(GNOME_IS_GURU(guru));

  gtk_widget_set_sensitive(guru->back, sensitivity);
}

GtkWidget * 
gnome_guru_current_page              (GnomeGuru   * guru)
{
  g_return_val_if_fail(guru != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_GURU(guru), NULL);
  
  return guru->current_page->widget;
}

static void 
gnome_guru_next_clicked  (GtkButton* nextbutton, GnomeGuru* guru)
{
  GList* current;
  GList* next;
  GnomeGuruPage* newpage;

  g_assert(guru->current_page != NULL);

  current = g_list_find(guru->pages, guru->current_page);
  g_return_if_fail(current != NULL);

  next = g_list_next(current);

  if (next) {
    newpage = (GnomeGuruPage*)next->data;
    
    flip_to_page(guru,
		 newpage);
  }
  else {
    gtk_signal_emit(GTK_OBJECT(guru),
		    guru_signals[FINISHED]);
  }
}

static void 
gnome_guru_back_clicked  (GtkButton* back, GnomeGuru* guru)
{
  GList* current;
  GList* prev;
  GnomeGuruPage* newpage;

  g_assert(guru->current_page != NULL);

  current = g_list_find(guru->pages, guru->current_page);
  g_return_if_fail(current != NULL);

  prev = g_list_previous(current);

  newpage = (GnomeGuruPage*)prev->data;
  
  flip_to_page(guru,
	       newpage);
}

static void 
gnome_guru_cancel_clicked(GtkButton* cancel, GnomeGuru* guru)
{
  gtk_signal_emit(GTK_OBJECT(guru),
		  guru_signals[CANCELLED]);
}

static gboolean
gnome_guru_dialog_closed(GnomeDialog* dialog, GnomeGuru* guru)
{
  gtk_signal_emit(GTK_OBJECT(guru),
		  guru_signals[CANCELLED]);
  return TRUE; /* we never close the dialog; let the user do it. */
}
