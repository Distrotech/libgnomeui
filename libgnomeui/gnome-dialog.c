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

static void gnome_dialog_class_init (GnomeDialogClass *klass);
static void gnome_dialog_init       (GnomeDialog      *messagebox);

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
  GtkWidget * separator;
  GtkWidget * bf;

  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  dialog->modal = FALSE;
  dialog->just_hide = FALSE;
  dialog->click_closes = FALSE;
  dialog->buttons = NULL;

#ifdef GTK_HAVE_ACCEL_GROUP
  dialog->accelerators = gtk_accel_group_new();
  gtk_window_add_accel_group (GTK_WINDOW(dialog), 
				    dialog->accelerators);
#else
  dialog->accelerators = gtk_accelerator_table_new();
  gtk_window_add_accelerator_table (GTK_WINDOW(dialog),
                                    dialog->accelerators);
#endif

  bf = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (bf), GTK_SHADOW_OUT);
  gtk_container_add(GTK_CONTAINER(dialog), bf);
  gtk_widget_show(bf);

  vbox = gtk_vbox_new(FALSE, GNOME_PAD);
  gtk_container_border_width (GTK_CONTAINER (vbox), 
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

  separator = gtk_hseparator_new ();
  gtk_box_pack_start (GTK_BOX (vbox), separator, 
		      FALSE, TRUE,
		      GNOME_PAD_SMALL);
  gtk_widget_show (separator);

  dialog->action_area = gtk_hbutton_box_new ();
  gtk_button_box_set_layout (GTK_BUTTON_BOX (dialog->action_area),
			     gnome_preferences_get_button_layout());

  gtk_button_box_set_spacing (GTK_BUTTON_BOX (dialog->action_area), 
			      GNOME_PAD);

  gtk_box_pack_start (GTK_BOX (vbox), dialog->action_area, 
		      FALSE, TRUE, 0);
  gtk_widget_show (dialog->action_area);
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
    
    gnome_dialog_append_buttons( dialog, 
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
    
    gnome_dialog_append_buttons( dialog, 
				 button_name);
  };  
}



GtkWidget* gnome_dialog_new            (const gchar * title,
					...)
{
  va_list ap;
  GnomeDialog *dialog;
  int count;
  gchar **buttons;
  gchar *button_name;
	
  dialog = gtk_type_new (gnome_dialog_get_type ());

  va_start (ap, title);
  count = 0;
  while (TRUE) {
    button_name = va_arg (ap, gchar *);
    count++;
    if (button_name == NULL) {
      break;
    }
  }
  va_end (ap);
  
  buttons = g_new (gchar *, count);
  count = 0;

  va_start (ap, title);
  
  while (TRUE) {
    buttons[count] = va_arg (ap, gchar *);
    if (buttons[count] == NULL) {
      break;
    }
    count++;
  }
  va_end (ap);
  
  gnome_dialog_constructv(dialog, title, (const gchar **)buttons);
  g_free(buttons);

  return GTK_WIDGET (dialog);
}

GtkWidget* gnome_dialog_newv            (const gchar * title,
					 const gchar ** buttons)
{
  va_list ap;
  GnomeDialog *dialog;
	
  dialog = gtk_type_new (gnome_dialog_get_type ());

  gnome_dialog_constructv(dialog, title, buttons);

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
    
    button = gnome_stock_or_ordinary_button (button_name);
    gnome_button_can_default (GTK_BUTTON(button), TRUE);
    gtk_container_add (GTK_CONTAINER(dialog->action_area), 
		       button);
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

void
gnome_dialog_set_modal (GnomeDialog * dialog)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  dialog->modal = TRUE;

  /* This is needed if someone shows before calling the function. */
  if (GTK_WIDGET_VISIBLE(GTK_WIDGET(dialog))) gtk_grab_add(GTK_WIDGET(dialog));
}

struct GnomeDialogRunInfo {
  gint button_number;
  gint close_id, clicked_id;
};

static void
gnome_dialog_setbutton_callback(GnomeDialog *dialog,
				gint button_number,
				struct GnomeDialogRunInfo *runinfo)
{
  if(runinfo->close_id < 0)
    return;

  runinfo->button_number = button_number;

  gtk_signal_disconnect(GTK_OBJECT(dialog),
			runinfo->close_id);
  gtk_signal_disconnect(GTK_OBJECT(dialog),
			runinfo->clicked_id);

  runinfo->close_id = runinfo->clicked_id = -1;

  gtk_main_quit();
}

static gboolean
gnome_dialog_quit_run(GnomeDialog *dialog,
		      struct GnomeDialogRunInfo *runinfo)
{
  if(runinfo->close_id < 0)
    return FALSE;

  gtk_signal_disconnect(GTK_OBJECT(dialog),
			runinfo->close_id);
  gtk_signal_disconnect(GTK_OBJECT(dialog),
			runinfo->clicked_id);

  runinfo->close_id = runinfo->clicked_id = -1;

  gtk_main_quit();

  return FALSE;
}

gint
gnome_dialog_run_modal(GnomeDialog *dialog)
{
  g_return_val_if_fail(dialog != NULL, -1);
  g_return_val_if_fail(GNOME_IS_DIALOG(dialog), -1);

  gnome_dialog_set_modal(dialog);
  return gnome_dialog_run(dialog);
}

gint
gnome_dialog_run(GnomeDialog *dialog)
{
  gboolean was_modal;
  struct GnomeDialogRunInfo ri = {-1,-1,-1};

  g_return_val_if_fail(dialog != NULL, -1);
  g_return_val_if_fail(GNOME_IS_DIALOG(dialog), -1);

  was_modal = dialog->modal;
  if (!was_modal) gnome_dialog_set_modal(dialog);

  ri.clicked_id =
    gtk_signal_connect(GTK_OBJECT(dialog), "clicked",
		       GTK_SIGNAL_FUNC(gnome_dialog_setbutton_callback),
		       &ri);

  ri.close_id =
    gtk_signal_connect(GTK_OBJECT(dialog), "close",
		       GTK_SIGNAL_FUNC(gnome_dialog_quit_run),
		       &ri);

  if ( ! GTK_WIDGET_VISIBLE(GTK_WIDGET(dialog)) )
    gtk_widget_show(GTK_WIDGET(dialog));

  gtk_main();

  if(!was_modal)
    {
      /* It would be nicer if gnome_dialog_set_modal() took a
	 bool value to indicate yes/no modal */
      dialog->modal = FALSE;
      gtk_grab_remove(GTK_WIDGET(dialog));
    }

  if(ri.close_id >= 0)
    {
      gtk_signal_disconnect(GTK_OBJECT(dialog), ri.close_id);
      gtk_signal_disconnect(GTK_OBJECT(dialog), ri.clicked_id);
    }

  return ri.button_number;
}

gint
gnome_dialog_run_and_hide (GnomeDialog * dialog)
{
  gint r;

  g_return_val_if_fail(dialog != NULL, -1);
  g_return_val_if_fail(GNOME_IS_DIALOG(dialog), -1);  

  gnome_dialog_close_hides(dialog, TRUE);

  r = gnome_dialog_run(dialog);

  gnome_dialog_close(dialog); /* Causes a hide. */
  return r;
}

gint 
gnome_dialog_run_and_destroy (GnomeDialog * dialog)
{
  gint r;

  g_return_val_if_fail(dialog != NULL, -1);
  g_return_val_if_fail(GNOME_IS_DIALOG(dialog), -1);  

  gnome_dialog_close_hides(dialog, FALSE);

  r = gnome_dialog_run(dialog);

  gnome_dialog_close(dialog); /* Causes a destroy. */
  return r;
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

void       gnome_dialog_set_destroy (GnomeDialog * d, gboolean self_destruct)
{
  g_warning("gnome_dialog_set_destroy() is deprecated.\n");

  gnome_dialog_set_close(d, self_destruct);
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
#ifdef GTK_HAVE_ACCEL_GROUP
/*FIXME*/
    gtk_widget_add_accelerator(GTK_WIDGET(list->data),
			       "clicked",
			       dialog->accelerators,
			       accelerator_key,
			       accelerator_mods,
			       GTK_ACCEL_VISIBLE);
#else
    gtk_widget_install_accelerator(GTK_WIDGET(list->data),
                                   dialog->accelerators,
                                   "clicked",
                                   accelerator_key,
                                   accelerator_mods);

#endif
    
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

#ifdef GTK_HAVE_ACCEL_GROUP
  if (GNOME_DIALOG(dialog)->accelerators) 
    gtk_window_remove_accel_group(GTK_WINDOW(dialog), GNOME_DIALOG(dialog)->accelerators);
#else
  if (GNOME_DIALOG(dialog)->accelerators)
    gtk_accelerator_table_unref(GNOME_DIALOG(dialog)->accelerators);
#endif

  if (GTK_OBJECT_CLASS(parent_class)->destroy)
    (* (GTK_OBJECT_CLASS(parent_class)->destroy))(dialog);
}

void gnome_dialog_close_real(GnomeDialog * dialog)
{
  g_return_if_fail(dialog != NULL);
  g_return_if_fail(GNOME_IS_DIALOG(dialog));

  gtk_widget_hide(GTK_WIDGET(dialog));

  if ( dialog->modal ) 
    gtk_grab_remove(GTK_WIDGET(dialog)); /* Otherwise the hidden
					    widget would still have
					    the grab - very weird. */

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
  /* _set_modal() could always do this on the first _show(), but
     if the dialog is hidden then shown it wouldn't work. Thus this
     function. */
  
  if (GNOME_DIALOG(d)->modal) gtk_grab_add (d);
  
  if (GTK_WIDGET_CLASS(parent_class)->show)
    (* (GTK_WIDGET_CLASS(parent_class)->show))(d);
}


/****************************************************************
  $Log$
  Revision 1.33  1998/07/17 23:37:03  yacc
  provide ...v-forms of functions using varargs.

  Revision 1.32  1998/07/15 03:59:15  hp


  Tue Jul 14 22:47:33 1998  Havoc Pennington  <hp@pobox.com>

  * gnome-app.h, gnome-app.c, gnome-dialog.h, gnome-dialog.c:
  Added "construct" functions. The gnome-dialog one takes a va_list
  which is kind of weird; if there's a good alternative I'd like
  to use it instead.

  Revision 1.31  1998/07/10 08:53:22  timj
  Fri Jul 10 10:19:38 1998  Tim Janik  <timj@gtk.org>

          * gnome-app-helper.c (gnome_app_do_menu_creation):
          * gnome-stock.c (gnome_stock_menu_item):
          create GtkAccelLabel if GTK_HAVE_ACCEL_GROUP, so accelerators are
          visible.

  Revision 1.30  1998/07/03 00:51:57  hp


  This fixes a couple of bugs in my Sunday commits, but breaks things: namely,
  your handlebox preferences if you've set them, and gnome_dialog_run_and_die
  is renamed if anyone had used it. Sorry it didn't get in on Monday, all that
  CVS confusion.

  Mon Jun 29 14:10:48 1998  Havoc Pennington  <hp@pobox.com>

  * gnome-preferences.h, gnome-preferences.c
  (gnome_preferences_get_toolbar_relief,
  gnome_preferences_set_toolbar_relief): Whether the toolbar buttons
  have the beveled edge.
  * gnome-app.c (gnome_app_set_toolbar): Turn off the toolbar button
  relief if user requested it.
  * gnome-preferences.c (gnome_preferences_load): Oops, forgot to
  push a new prefix for the GnomeApp stuff, it was being saved under
  Dialog prefs.
  * gnome-dialog.h, gnome-dialog.c (gnome_dialog_run_and_die):
  Renamed to run_and_destroy - run_and_die was cute at 2 am, but
  probably not a good name. ;-)

  Revision 1.29  1998/06/29 08:06:27  hp


  I think Elliot and I talked about the dialog changes this time, and there
  are now two functions to make both of us happy. The other change allows
  turning off the handlebox for the menubar and toolbar, I think that was
  mentioned recently.

  Mon Jun 29 03:04:30 1998  Havoc Pennington  <hp@pobox.com>

  * TODO: Updated a little.

  Mon Jun 29 02:58:15 1998  Havoc Pennington  <hp@pobox.com>

  * gnome-preferences.h, gnome-preferences.c
  (gnome_preferences_get_toolbar_handlebox,
  gnome_preferences_set_toolbar_handlebox,
  gnome_preferences_get_menubar_handlebox,
  gnome_preferences_set_menubar_handlebox): New functions.

  * gnome-app.c (gnome_app_set_menus): Obey preferences for
  handlebox.
  (gnome_app_set_toolbar): Obey handlebox prefs.

  Mon Jun 29 02:52:10 1998  Havoc Pennington  <hp@pobox.com>

  * gnome-dialog.h, gnome-dialog.c (gnome_dialog_run): Take out the
  hide stuff.
  (gnome_dialog_run_and_hide): Do what gnome_dialog_run used to do.
  (gnome_dialog_run_and_die):  Run and then destroy the dialog.
  (gnome_dialog_button_connect_object): Someone requested this.

  Revision 1.28  1998/06/11 02:26:50  yosh
  changed things to use GTK_HAVE_ACCEL_GROUP instead of HAVE_DEVGTK...
  installed headers depending on config.h stuff is bad.

  -Yosh

  Revision 1.27  1998/06/10 17:15:25  gregm
  Wed Jun 10 13:07:09 EDT 1998 Gregory McLean <gregm@comstar.net>

          * Wheee libgnomeui now compiles (with the exception of the
            canvas stuff) under 1.0.x again. It also compiles under 1.1
            for thoose of you that like to bleed. Please please if you
            add code that _requires_ gtk 1.1 shield it with GTK_HAVE_ACCEL_GROUP
            so us boring folks can continue to get stuff done.

  Revision 1.26  1998/06/07 17:58:21  pavlov
  updates to make gnome-libs compile with new gtk1.1

  i havn't tested this yet, other than knowing it compiles.  i will in a minute,
  but my gtk is fooed at the moment, and button's arn't showing labels :)

  -pav

  Revision 1.25  1998/05/25 16:31:18  sopwith
  Move log msgs to bottom

  Revision 1.24  1998/05/25 16:15:57  sopwith


  gnome_dialog_run_*(): Fixes for Havoc's bugs
  Also added CVS log recording at the top :)

*****************************************************************/
