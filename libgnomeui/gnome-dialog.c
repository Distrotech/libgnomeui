/* GNOME GUI Library
 * Copyright (C) 1997, 1998 Jay Painter
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
#include <stdarg.h>

#include "gnome-dialog.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-i18nP.h"
#include <string.h> /* for strcmp */
#include <gtk/gtk.h>
#include "libgnomeui/gnome-stock.h"
#include "libgnomeui/gnome-uidefs.h"

#define GNOME_DIALOG_BUTTON_WIDTH 100
#define GNOME_DIALOG_BUTTON_HEIGHT 40

#define GNOME_DIALOG_BORDER_WIDTH 5

enum {
  CLICKED,
  LAST_SIGNAL
};

typedef void (*GnomeDialogSignal1) (GtkObject *object,
				    gint       arg1,
				    gpointer   data);

static void gnome_dialog_marshal_signal_1 (GtkObject         *object,
					   GtkSignalFunc      func,
					   gpointer           func_data,
					   GtkArg            *args);

static void gnome_dialog_class_init (GnomeDialogClass *klass);
static void gnome_dialog_init       (GnomeDialog      *messagebox);

static void gnome_dialog_button_clicked (GtkWidget   *button, 
					 GtkWidget   *messagebox);
static void gnome_dialog_destroy (GtkObject *dialog);

static GtkWindowClass *parent_class;
static gint dialog_signals[LAST_SIGNAL] = { 0 };

guint
gnome_dialog_get_type ()
{
  static guint dialog_type = 0;

  if (!dialog_type)
    {
      GtkTypeInfo dialog_info =
      {
	"GnomeDialog",
	sizeof (GnomeDialog),
	sizeof (GnomeDialogClass),
	(GtkClassInitFunc) gnome_dialog_class_init,
	(GtkObjectInitFunc) gnome_dialog_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
      };

      dialog_type = gtk_type_unique (gtk_window_get_type (), &dialog_info);
    }

  return dialog_type;
}

static void
gnome_dialog_class_init (GnomeDialogClass *klass)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkWindowClass *window_class;

  object_class = (GtkObjectClass*) klass;
  widget_class = (GtkWidgetClass*) klass;
  window_class = (GtkWindowClass*) klass;

  parent_class = gtk_type_class (gtk_window_get_type ());

  dialog_signals[CLICKED] =
    gtk_signal_new ("clicked",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeDialogClass, clicked),
		    gnome_dialog_marshal_signal_1,
		    GTK_TYPE_NONE, 1, GTK_TYPE_INT);

  gtk_object_class_add_signals (object_class, dialog_signals, 
				LAST_SIGNAL);

  klass->clicked = NULL;
  object_class->destroy = gnome_dialog_destroy;
}

static void
gnome_dialog_marshal_signal_1 (GtkObject      *object,
			       GtkSignalFunc   func,
			       gpointer        func_data,
			       GtkArg         *args)
{
  GnomeDialogSignal1 rfunc;

  rfunc = (GnomeDialogSignal1) func;

  (* rfunc) (object, GTK_VALUE_INT (args[0]), func_data);
}

static void
gnome_dialog_init (GnomeDialog *dialog)
{
  GtkWidget * vbox;
  GtkWidget * separator;
  GtkWidget * bf;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  dialog->modal = FALSE;
  dialog->self_destruct = FALSE;
  dialog->buttons = NULL;
  
  bf = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (bf), GTK_SHADOW_OUT);
  gtk_container_add(GTK_CONTAINER(dialog), bf);
  gtk_widget_show(bf);

  vbox = gtk_vbox_new(FALSE, GNOME_PAD);
  gtk_container_border_width (GTK_CONTAINER (vbox), 
			      GNOME_DIALOG_BORDER_WIDTH);
  gtk_container_add(GTK_CONTAINER(bf), vbox);
  gtk_widget_show(vbox);

  gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, 
			 FALSE, FALSE);

  dialog->vbox = gtk_vbox_new(FALSE, GNOME_PAD);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->vbox, 
		      FALSE, TRUE,
		      GNOME_DIALOG_BORDER_WIDTH);
  gtk_widget_show(dialog->vbox);

  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), separator, 
		      FALSE, TRUE,
		      GNOME_DIALOG_BORDER_WIDTH);
  gtk_widget_show (separator);

  dialog->action_area = gtk_hbutton_box_new ();
  gtk_button_box_set_layout ( GTK_BUTTON_BOX (dialog->action_area),
			      GTK_BUTTONBOX_END );
  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog->action_area), 
			      GNOME_PAD);

  gtk_box_pack_start (GTK_BOX (vbox), dialog->action_area, 
		      FALSE, TRUE, 0);
  gtk_widget_show (dialog->action_area);
}


GtkWidget* gnome_dialog_new            (const gchar * title,
					...)
{
  va_list ap;
  GnomeDialog *dialog;
  gchar * button_name;
	
  va_start (ap, title);
	
  dialog = gtk_type_new (gnome_dialog_get_type ());

  if (title)
    gtk_window_set_title (GTK_WINDOW (dialog), title);

  while (TRUE) {

    button_name = va_arg (ap, gchar *);

    if (button_name == NULL) {
      break;
    }
	  
    gnome_dialog_append_buttons( dialog, 
				 button_name, 
				 NULL );
  };

  va_end (ap);

  return GTK_WIDGET (dialog);
}

void       gnome_dialog_append_buttons (GnomeDialog * dialog,
					const gchar * first,
					...)
{
  va_list ap;
  const gchar * button_name = first;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  va_start(ap, first);

  while(button_name != NULL) {
    GtkWidget *button;
    
    button = gnome_stock_or_ordinary_button (button_name);
    GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
    gtk_widget_set_usize (button, GNOME_DIALOG_BUTTON_WIDTH,
			  GNOME_DIALOG_BUTTON_HEIGHT);
    gtk_container_add (GTK_CONTAINER(dialog->action_area), 
		       button);
    gtk_widget_grab_default (button);
    gtk_widget_show (button);
    
    gtk_signal_connect (GTK_OBJECT (button), "clicked",
			(GtkSignalFunc) gnome_dialog_button_clicked,
			dialog);
    
    dialog->buttons = g_list_append (dialog->buttons, button);

    button_name = va_arg (ap, gchar *);
  }
  va_end(ap);
}

void
gnome_dialog_set_modal (GnomeDialog * dialog)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  dialog->modal = TRUE;
  gtk_grab_add (GTK_WIDGET (dialog));
}

void
gnome_dialog_set_default (GnomeDialog *dialog,
			  gint button)
{
  GList *list;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data) {
    gtk_widget_grab_default (GTK_WIDGET (list->data));
    return;
  }
#ifdef GNOME_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning("Button number %d does not appear to exist\n", button); 
#endif
}

void       gnome_dialog_set_destroy    (GnomeDialog * dialog,
					gboolean self_destruct)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  dialog->self_destruct = self_destruct;
}

void       gnome_dialog_set_sensitive  (GnomeDialog *dialog,
					gint         button,
					gboolean     setting)
{
  GList *list;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data) {
    gtk_widget_set_sensitive(GTK_WIDGET(list->data), setting);
    return;
  }
#ifdef GNOME_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning("Button number %d does not appear to exist\n", button); 
#endif
}

void       gnome_dialog_button_connect (GnomeDialog *dialog,
					gint button,
					GtkSignalFunc callback,
					gpointer data)
{
  GList * list;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data) {
    gtk_signal_connect(GTK_OBJECT(list->data), "clicked",
		       callback, data);
    return;
  }
#ifdef GNOME_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning("Button number %d does not appear to exist\n", button); 
#endif
}


static void
gnome_dialog_button_clicked (GtkWidget   *button, 
			     GtkWidget   *dialog)
{
  GList *list;
  int which = 0;
  gboolean self_destruct;
  
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  self_destruct = GNOME_DIALOG(dialog)->self_destruct;
  list = GNOME_DIALOG (dialog)->buttons;

  while (list){
    if (list->data == button) {
      gtk_signal_emit (GTK_OBJECT (dialog), dialog_signals[CLICKED], 
		       which);
      break;
    }
    list = list->next;
    which ++;
  }
  
  /* The dialog may have been destroyed by the clicked
     signal, which is why we had to save self_destruct.
     Users should be careful not to set self_destruct 
     and then destroy the dialog themselves too. */

  if (self_destruct) {
    gtk_widget_destroy(dialog);
  }
}

static void gnome_dialog_destroy (GtkObject *dialog)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  g_list_free(GNOME_DIALOG (dialog)->buttons);

  if (GTK_OBJECT_CLASS(parent_class)->destroy)
    (* (GTK_OBJECT_CLASS(parent_class)->destroy))(dialog);
}




