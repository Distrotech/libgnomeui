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

#include "gnome-dialog.h"
#include "libgnome/gnome-util.h"
#include "libgnome/gnome-i18nP.h"
#include <string.h> /* for strcmp */
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "gnome-stock.h"
#include "gnome-uidefs.h"
#include "gnome-preferences.h"
#include "gnome-dialog-util.h"

enum {
  CLICKED,
  CLOSE,
  LAST_SIGNAL
};

typedef void (*GnomeDialogSignal1) (GtkObject *object,
				    gint       arg1,
				    gpointer   data);

typedef gboolean (*GnomeDialogSignal2) (GtkObject *object,
					gpointer   data);

static void gnome_dialog_marshal_signal_1 (GtkObject         *object,
					   GtkSignalFunc      func,
					   gpointer           func_data,
					   GtkArg            *args);
static void gnome_dialog_marshal_signal_2 (GtkObject         *object,
					   GtkSignalFunc      func,
					   gpointer           func_data,
					   GtkArg            *args);

static void gnome_dialog_class_init       (GnomeDialogClass *klass);
static void gnome_dialog_init             (GnomeDialog * dialog);
static void gnome_dialog_init_action_area (GnomeDialog * dialog);


static void gnome_dialog_button_clicked (GtkWidget   *button, 
					 GtkWidget   *messagebox);
static gint gnome_dialog_key_pressed (GtkWidget * d, GdkEventKey * e);
static gint gnome_dialog_delete_event (GtkWidget * d, GdkEventAny * e);
static void gnome_dialog_destroy (GtkObject *dialog);
static void gnome_dialog_show (GtkWidget * d);
static void gnome_dialog_close_real(GnomeDialog * d);

static GtkWindowClass *parent_class;
static gint dialog_signals[LAST_SIGNAL] = { 0, 0 };

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

  dialog_signals[CLOSE] =
    gtk_signal_new ("close",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeDialogClass, close),
		    gnome_dialog_marshal_signal_2,
		    GTK_TYPE_INT, 0);

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
  klass->close = NULL;
  object_class->destroy = gnome_dialog_destroy;
  widget_class->key_press_event = gnome_dialog_key_pressed;
  widget_class->delete_event = gnome_dialog_delete_event;
  widget_class->show = gnome_dialog_show;
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
gnome_dialog_marshal_signal_2 (GtkObject	    *object,
			       GtkSignalFunc        func,
			       gpointer	            func_data,
			       GtkArg	            *args)
{
  GnomeDialogSignal2 rfunc;
  gint * return_val;
  
  rfunc = (GnomeDialogSignal2) func;
  return_val = GTK_RETLOC_INT (args[0]);
  
  *return_val = (* rfunc) (object,
			   func_data);
}

static void
gnome_dialog_init (GnomeDialog *dialog)
{
  GtkWidget * vbox;
  GtkWidget * bf;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  dialog->just_hide = FALSE;
  dialog->click_closes = FALSE;
  dialog->buttons = NULL;

  GTK_WINDOW(dialog)->type = gnome_preferences_get_dialog_type();
  gtk_window_set_position(GTK_WINDOW(dialog), 
		      gnome_preferences_get_dialog_position());
  
  dialog->accelerators = gtk_accel_group_new();
  gtk_window_add_accel_group (GTK_WINDOW(dialog), 
				    dialog->accelerators);

  bf = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (bf), GTK_SHADOW_OUT);
  gtk_container_add(GTK_CONTAINER(dialog), bf);
  gtk_widget_show(bf);

  vbox = gtk_vbox_new(FALSE, GNOME_PAD);
  gtk_container_set_border_width (GTK_CONTAINER (vbox), 
			      GNOME_PAD_SMALL);
  gtk_container_add(GTK_CONTAINER(bf), vbox);
  gtk_widget_show(vbox);

  gtk_window_set_policy (GTK_WINDOW (dialog), FALSE, 
			 FALSE, FALSE);

  dialog->vbox = gtk_vbox_new(FALSE, GNOME_PAD);
  gtk_box_pack_start (GTK_BOX (vbox), dialog->vbox, 
		      TRUE, TRUE,
		      GNOME_PAD_SMALL);
  gtk_widget_show(dialog->vbox);
}

static void
gnome_dialog_init_action_area (GnomeDialog * dialog)
{
  GtkWidget * separator;

  if (dialog->action_area)
    return;

  dialog->action_area = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog->action_area),
			     gnome_preferences_get_button_layout());

  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog->action_area), 
			      GNOME_PAD);

  gtk_box_pack_end (GTK_BOX (dialog->vbox), dialog->action_area, 
		    FALSE, TRUE, 0);
  gtk_widget_show (dialog->action_area);

  separator = gtk_hseparator_new ();
  gtk_box_pack_end (GTK_BOX (dialog->vbox), separator, 
		      FALSE, TRUE,
		      GNOME_PAD_SMALL);
  gtk_widget_show (separator);
}

void       
gnome_dialog_construct (GnomeDialog * dialog,
			const gchar * title,
			va_list ap)
{
  gchar * button_name;
  
  if (title)
    gtk_window_set_title (GTK_WINDOW (dialog), title);
  
  while (TRUE) {
    
    button_name = va_arg (ap, gchar *);
    
    if (button_name == NULL) {
      break;
    }
    
    gnome_dialog_append_button( dialog, 
				button_name);
  };  
}

void gnome_dialog_constructv (GnomeDialog * dialog,
			      const gchar * title,
			      const gchar ** buttons)
{
  const gchar * button_name;
  
  if (title)
    gtk_window_set_title (GTK_WINDOW (dialog), title);
  
  while (TRUE) {
    
    button_name = *buttons++;
    
    if (button_name == NULL) {
      break;
    }
    
    gnome_dialog_append_button( dialog, 
				button_name);
  };  
}



GtkWidget* gnome_dialog_new            (const gchar * title,
					...)
{
  va_list ap;
  GnomeDialog *dialog;
	
  dialog = gtk_type_new (gnome_dialog_get_type ());

  va_start (ap, title);
  
  gnome_dialog_construct(dialog, title, ap);

  va_end(ap);

  /* argument list may be null if the user wants to do weird things to the
   * dialog, but we need to make sure this is initialized */
  gnome_dialog_init_action_area(dialog);
  
  return GTK_WIDGET (dialog);
}

GtkWidget* gnome_dialog_newv            (const gchar * title,
					 const gchar ** buttons)
{
  GnomeDialog *dialog;
	
  dialog = gtk_type_new (gnome_dialog_get_type ());

  gnome_dialog_constructv(dialog, title, buttons);

  return GTK_WIDGET (dialog);
}

void       gnome_dialog_set_parent     (GnomeDialog * dialog,
					GtkWindow   * parent)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));
  g_return_if_fail(parent != NULL);
  g_return_if_fail(GTK_IS_WINDOW(parent));

  gtk_window_set_transient_for (GTK_WINDOW(dialog), parent);

  if ( gnome_preferences_get_dialog_centered() ) {

    /* User wants us to center over parent */

    gint x, y, w, h, dialog_x, dialog_y;

    if ( ! GTK_WIDGET_VISIBLE(parent)) return; /* Can't get its
						  size/pos */

    /* Throw out other positioning */
    gtk_window_set_position(GTK_WINDOW(dialog),GTK_WIN_POS_NONE);

    gdk_window_get_origin (GTK_WIDGET(parent)->window, &x, &y);
    gdk_window_get_size   (GTK_WIDGET(parent)->window, &w, &h);

    /* The problem here is we don't know how big the dialog is.
       So "centered" isn't really true. We'll go with 
       "kind of more or less on top" */

    dialog_x = x + w/4;
    dialog_y = y + h/4;

    gtk_widget_set_uposition(GTK_WIDGET(dialog), dialog_x, dialog_y); 
  }
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
    gnome_dialog_append_button (dialog, button_name);
    button_name = va_arg (ap, gchar *);
  }
  va_end(ap);
}

void       gnome_dialog_append_button (GnomeDialog * dialog,
				       const gchar * button_name)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  if (button_name != NULL) {
    GtkWidget *button;

    gnome_dialog_init_action_area (dialog);    

    button = gnome_stock_or_ordinary_button (button_name);
    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (button), GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (dialog->action_area), button, TRUE, TRUE, 0);

    gtk_widget_grab_default (button);
    gtk_widget_show (button);
    
    gtk_signal_connect_after (GTK_OBJECT (button), "clicked",
			      (GtkSignalFunc) gnome_dialog_button_clicked,
			      dialog);
    
    dialog->buttons = g_list_append (dialog->buttons, button);
  }
}

void       gnome_dialog_append_button_with_pixmap (GnomeDialog * dialog,
						   const gchar * button_name,
						   const gchar * pixmap_name)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  if (button_name != NULL) {
    GtkWidget *button;

    if (pixmap_name != NULL) {
      GtkWidget *pixmap;
 
      pixmap = gnome_stock_new_with_icon (pixmap_name);
      button = gnome_pixmap_button (pixmap, button_name);    
    } else {
      button = gnome_stock_or_ordinary_button (button_name);
    }

    gnome_dialog_init_action_area (dialog);    

    GTK_WIDGET_SET_FLAGS (GTK_WIDGET (button), GTK_CAN_DEFAULT);
    gtk_box_pack_start (GTK_BOX (dialog->action_area), button, TRUE, TRUE, 0);

    gtk_widget_grab_default (button);
    gtk_widget_show (button);
    
    gtk_signal_connect_after (GTK_OBJECT (button), "clicked",
			      (GtkSignalFunc) gnome_dialog_button_clicked,
			      dialog);
    
    dialog->buttons = g_list_append (dialog->buttons, button);
  }
}

void       gnome_dialog_append_buttonsv (GnomeDialog * dialog,
					 const gchar ** buttons)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  while(*buttons != NULL) {
    gnome_dialog_append_button (dialog, *buttons);
    buttons++;
  }
}

void       gnome_dialog_append_buttons_with_pixmaps (GnomeDialog * dialog,
						     const gchar **names,
						     const gchar **pixmaps)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  while(*names != NULL) {
    gnome_dialog_append_button_with_pixmap (dialog, *names, *pixmaps);
    names++; pixmaps++;
  }
}

struct GnomeDialogRunInfo {
  gint button_number;
  gint close_id, clicked_id, destroy_id;
  gboolean destroyed;
};

static void
gnome_dialog_shutdown_run(GnomeDialog* dialog,
                          struct GnomeDialogRunInfo* runinfo)
{
  if (!runinfo->destroyed) 
    {
      
      gtk_signal_disconnect(GTK_OBJECT(dialog),
                            runinfo->close_id);
      gtk_signal_disconnect(GTK_OBJECT(dialog),
                            runinfo->clicked_id);
  
      runinfo->close_id = runinfo->clicked_id = -1;
    }

  gtk_main_quit();
}

static void
gnome_dialog_setbutton_callback(GnomeDialog *dialog,
				gint button_number,
				struct GnomeDialogRunInfo *runinfo)
{
  if(runinfo->close_id < 0)
    return;

  runinfo->button_number = button_number;

  gnome_dialog_shutdown_run(dialog, runinfo);
}

static gboolean
gnome_dialog_quit_run(GnomeDialog *dialog,
		      struct GnomeDialogRunInfo *runinfo)
{
  if(runinfo->close_id < 0)
    return FALSE;

  gnome_dialog_shutdown_run(dialog, runinfo);

  return FALSE;
}

static void
gnome_dialog_mark_destroy(GnomeDialog* dialog,
                          struct GnomeDialogRunInfo* runinfo)
{
  runinfo->destroyed = TRUE;

  if(runinfo->close_id < 0)
    return;
  else gnome_dialog_shutdown_run(dialog, runinfo);
}

static gint
gnome_dialog_run_real(GnomeDialog* dialog, gboolean close_after)
{
  gboolean was_modal;
  struct GnomeDialogRunInfo ri = {-1,-1,-1,-1,FALSE};

  g_return_val_if_fail(dialog != NULL, -1);
  g_return_val_if_fail(GNOME_IS_DIALOG(dialog), -1);

  was_modal = GTK_WINDOW(dialog)->modal;
  if (!was_modal) gtk_window_set_modal(GTK_WINDOW(dialog),TRUE);

  /* There are several things that could happen to the dialog, and we
     need to handle them all: click, delete_event, close, destroy */

  ri.clicked_id =
    gtk_signal_connect(GTK_OBJECT(dialog), "clicked",
		       GTK_SIGNAL_FUNC(gnome_dialog_setbutton_callback),
		       &ri);

  ri.close_id =
    gtk_signal_connect(GTK_OBJECT(dialog), "close",
		       GTK_SIGNAL_FUNC(gnome_dialog_quit_run),
		       &ri);

  ri.destroy_id = 
    gtk_signal_connect(GTK_OBJECT(dialog), "destroy",
                       GTK_SIGNAL_FUNC(gnome_dialog_mark_destroy),
                       &ri);

  if ( ! GTK_WIDGET_VISIBLE(GTK_WIDGET(dialog)) )
    gtk_widget_show(GTK_WIDGET(dialog));

  gtk_main();

  if(!ri.destroyed) {

    gtk_signal_disconnect(GTK_OBJECT(dialog), ri.destroy_id);

    if(!was_modal)
      {
	gtk_window_set_modal(GTK_WINDOW(dialog),FALSE);
      }
    
    if(ri.close_id >= 0) /* We didn't shut down the run? */
      {
	gtk_signal_disconnect(GTK_OBJECT(dialog), ri.close_id);
	gtk_signal_disconnect(GTK_OBJECT(dialog), ri.clicked_id);
      }

    if (close_after)
      {
        gnome_dialog_close(dialog);
      }
  }

  return ri.button_number;
}

gint
gnome_dialog_run(GnomeDialog *dialog)
{
  return gnome_dialog_run_real(dialog,FALSE);
}

gint 
gnome_dialog_run_and_close(GnomeDialog* dialog)
{
  return gnome_dialog_run_real(dialog,TRUE);
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

void       gnome_dialog_set_close    (GnomeDialog * dialog,
				      gboolean click_closes)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  dialog->click_closes = click_closes;
}

void       gnome_dialog_close_hides (GnomeDialog * dialog, 
				     gboolean just_hide)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  dialog->just_hide = just_hide;
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

void       gnome_dialog_button_connect_object (GnomeDialog *dialog,
					       gint button,
					       GtkSignalFunc callback,
					       GtkObject * obj)
{
  GList * list;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data) {
    gtk_signal_connect_object (GTK_OBJECT(list->data), "clicked",
			       callback, obj);
    return;
  }
#ifdef GNOME_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning("Button number %d does not appear to exist\n", button); 
#endif
}


void       gnome_dialog_set_accelerator(GnomeDialog * dialog,
					gint button,
					const guchar accelerator_key,
					guint8       accelerator_mods)
{
  GList * list;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  list = g_list_nth (dialog->buttons, button);

  if (list && list->data) {
    /*FIXME*/
    gtk_widget_add_accelerator(GTK_WIDGET(list->data),
			       "clicked",
			       dialog->accelerators,
			       accelerator_key,
			       accelerator_mods,
			       GTK_ACCEL_VISIBLE);
    
    return;
  }
#ifdef GNOME_ENABLE_DEBUG
  /* If we didn't find the button, complain */
  g_warning("Button number %d does not appear to exist\n", button); 
#endif
}

void       gnome_dialog_editable_enters   (GnomeDialog * dialog,
					   GtkEditable * editable)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(editable != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));
  g_return_if_fail(GTK_IS_EDITABLE(editable));

  gtk_signal_connect_object(GTK_OBJECT(editable), "activate",
			    GTK_SIGNAL_FUNC(gtk_window_activate_default), 
			    GTK_OBJECT(dialog));
}


static void
gnome_dialog_button_clicked (GtkWidget   *button, 
			     GtkWidget   *dialog)
{
  GList *list;
  int which = 0;
  gboolean click_closes;
  
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  click_closes = GNOME_DIALOG(dialog)->click_closes;
  list = GNOME_DIALOG (dialog)->buttons;

  while (list){
    if (list->data == button) {
      gtk_signal_emit (GTK_OBJECT (dialog), dialog_signals[CLICKED], 
		       which);
      break;
    }
    list = list->next;
    ++which;
  }
  
  /* The dialog may have been destroyed by the clicked
     signal, which is why we had to save self_destruct.
     Users should be careful not to set self_destruct 
     and then destroy the dialog themselves too. */

  if (click_closes) {
    gnome_dialog_close(GNOME_DIALOG(dialog));
  }
}

static gint gnome_dialog_key_pressed (GtkWidget * d, GdkEventKey * e)
{
  g_return_val_if_fail(GNOME_IS_DIALOG(d), TRUE);

  if(e->keyval == GDK_Escape)
    {
      gnome_dialog_close(GNOME_DIALOG(d));

      return TRUE; /* Stop the event? is this TRUE or FALSE? */
    } 

  /* Have to call parent's handler, or the widget wouldn't get any 
     key press events. Note that this is NOT done if the dialog
     may have been destroyed. */
  if (GTK_WIDGET_CLASS(parent_class)->key_press_event)
    return (* (GTK_WIDGET_CLASS(parent_class)->key_press_event))(d, e);
  else return FALSE; /* Not handled. */
}

static gint gnome_dialog_delete_event (GtkWidget * d, GdkEventAny * e)
{  
  gnome_dialog_close(GNOME_DIALOG(d));
  return TRUE; /* We handled it. */
}

static void gnome_dialog_destroy (GtkObject *dialog)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  g_list_free(GNOME_DIALOG (dialog)->buttons);

  if (GTK_OBJECT_CLASS(parent_class)->destroy)
    (* (GTK_OBJECT_CLASS(parent_class)->destroy))(dialog);
}

void gnome_dialog_close_real(GnomeDialog * dialog)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  gtk_widget_hide(GTK_WIDGET(dialog));

  if ( ! dialog->just_hide ) {
    gtk_widget_destroy (GTK_WIDGET (dialog));
  }
}

void gnome_dialog_close(GnomeDialog * dialog)
{
  gint close_handled = FALSE;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  gtk_signal_emit (GTK_OBJECT(dialog), dialog_signals[CLOSE],
		   &close_handled);

  if ( ! close_handled ) {
    gnome_dialog_close_real(dialog);
  }
}

static void gnome_dialog_show (GtkWidget * d)
{  
  if (GTK_WIDGET_CLASS(parent_class)->show)
    (* (GTK_WIDGET_CLASS(parent_class)->show))(d);
}

