/* gnome-appbar.h: statusbar/progress/minibuffer widget for Gnome apps
 * 
 * This version by Havoc Pennington
 * Based on MozStatusbar widget, by Chris Toshok
 * In turn based on GtkStatusbar widget, from Gtk+,
 * Copyright (C) 1995-1998 Peter Mattis, Spencer Kimball and Josh MacDonald
 * GtkStatusbar Copyright (C) 1998 Shawn T. Amundson
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

#include "gnome-appbar.h"

#include "libgnome/gnome-util.h"
#include "libgnomeui/gnome-preferences.h"

#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <string.h> /* for strlen */

#include "gnome-uidefs.h"

#define GNOME_ENABLE_DEBUG /* to be sure */

static void gnome_appbar_class_init               (GnomeAppBarClass *class);
static void gnome_appbar_init                     (GnomeAppBar      *ab);
static void gnome_appbar_destroy                  (GtkObject         *object);
     
static GtkContainerClass *parent_class;

enum {
  USER_RESPONSE,
  CLEAR_PROMPT,
  LAST_SIGNAL
};

static gint appbar_signals[LAST_SIGNAL] = { 0 };

guint      
gnome_appbar_get_type ()
{
  static guint ab_type = 0;

  if (!ab_type)
    {
      GtkTypeInfo ab_info =
      {
        "GnomeAppBar",
        sizeof (GnomeAppBar),
        sizeof (GnomeAppBarClass),
        (GtkClassInitFunc) gnome_appbar_class_init,
        (GtkObjectInitFunc) gnome_appbar_init,
        (GtkArgSetFunc) NULL,
        (GtkArgGetFunc) NULL,
      };

      ab_type = gtk_type_unique (gtk_hbox_get_type (), &ab_info);
    }

  return ab_type;
}

static void
gnome_appbar_class_init (GnomeAppBarClass *class)
{
  GtkObjectClass *object_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  object_class = (GtkObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;
  container_class = (GtkContainerClass *) class;

  appbar_signals[USER_RESPONSE] =
    gtk_signal_new ("user_response",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeAppBarClass, user_response),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);

  appbar_signals[CLEAR_PROMPT] =
    gtk_signal_new ("clear_prompt",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeAppBarClass, clear_prompt),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);

  gtk_object_class_add_signals (object_class, appbar_signals, 
				LAST_SIGNAL);

  parent_class = gtk_type_class (gtk_hbox_get_type ());
  
  class->user_response = NULL;
  class->clear_prompt  = NULL; /* maybe should have a handler
				  and the clear_prompt function
				  just emits. */
  object_class->destroy = gnome_appbar_destroy;
}

static GSList * 
stringstack_push(GSList * stringstack, const gchar * s)
{
  return g_slist_prepend(stringstack, g_strdup(s));
}

static GSList *
stringstack_pop(GSList * stringstack)
{
  /* Could be optimized */
  if (stringstack) {
    g_free(stringstack->data);
    return g_slist_remove(stringstack, stringstack->data);
  }
  else return NULL;
}

static const gchar *
stringstack_top(GSList * stringstack)
{
  if (stringstack) return stringstack->data;
  else return NULL;
}

static void
stringstack_free(GSList * stringstack)
{
  GSList * tmp = stringstack;
  while (tmp) {
    g_free(tmp->data);
    tmp = g_slist_next(tmp);
  }
  g_slist_free(stringstack);
}

static void
entry_delete_text_cb(GtkWidget * entry, gint start, 
		     gint stop, GnomeAppBar * ab)
{
  g_return_if_fail(GNOME_IS_APPBAR(ab));
  g_return_if_fail(GTK_IS_ENTRY(entry));
  g_return_if_fail(ab->interactive);

  if (ab->prompt == NULL) return; /* not prompting, so don't 
				     interfere. */
#ifdef GNOME_ENABLE_DEBUG
    g_print("Start is %d, stop is %d, start of editable is %d\n", 
	    start, stop, ab->editable_start);
#endif
  
  if (start < ab->editable_start) {
    /* Block the signal, since it's trying to delete text
       that shouldn't be deleted. */
    gtk_signal_emit_stop_by_name(GTK_OBJECT(entry), "delete_text");
    gdk_beep();
  }
}
static void 
entry_insert_text_cb  (GtkEditable    *entry,
		       const gchar    *text,
		       gint            length,
		       gint           *position,
		       GnomeAppBar * ab)
{
  gint pos; 

  if (ab->prompt == NULL) return; /* not prompting, so don't 
				     interfere. */

#ifdef HAVE_GTK_EDITABLE_GET_POSITION
  pos = gtk_editable_get_position(entry);
#else
  pos = GTK_EDITABLE(entry)->current_pos;
#endif

#ifdef GNOME_ENABLE_DEBUG
    g_print("Position is %d, start of editable is %d\n", 
	    pos, ab->editable_start);
#endif

  if (pos < ab->editable_start) {

    /*    gtk_editable_set_position(entry, ab->editable_start); */
    gtk_signal_emit_stop_by_name(GTK_OBJECT(entry), "insert_text");
    /*    gtk_signal_emit_by_name(GTK_OBJECT(entry), "insert_text",
			    text, length, position); */
  }
}

static gint
entry_key_press_cb(GtkWidget * entry, GdkEventKey * e, GnomeAppBar * ab)
{
  /* This should be some kind of configurable binding
      or accelerator, probably. */
  if ( (e->keyval == GDK_g) && 
       (e->state & GDK_CONTROL_MASK) ) {
    /* Cancel */
    gnome_appbar_clear_prompt(ab);
    return TRUE; /* Override other handlers */
  }
  else return FALSE; /* let entry handle it. */
}

static void
entry_activate_cb(GtkWidget * entry, GnomeAppBar * ab)
{
  gtk_signal_emit(GTK_OBJECT(ab), appbar_signals[USER_RESPONSE]);
}

static void
gnome_appbar_init (GnomeAppBar *ab)
{
  ab->default_status = NULL;
  ab->status_stack   = NULL;
  ab->editable_start = 0;
  ab->prompt         = NULL;
}


/**
 * gnome_appbar_new
 * @has_progress: %TRUE if appbar needs progress bar widget, %FALSE if not
 * @has_status: %TRUE if appbar needs status bar widget, %FALSE if not
 * @interactivity: Level of user activity required
 *
 * Description:
 * Create a new GNOME application status bar.  If @has_progress is
 * %TRUE, a small progress bar widget will be created, and placed on the
 * left side of the appbar.  If @has_status is %TRUE, a status bar,
 * possibly an editable one, is created.
 *
 * @interactivity determines whether the appbar is an interactive
 * "minibuffer" or just a status bar.  If it is set to
 * %GNOME_PREFERENCES_NEVER, it is never interactive.  If it is set to
 * %GNOME_PREFERENCES_USER we respect user preferences from
 * ui-properties. If it's %GNOME_PREFERENCES_ALWAYS we are interactive
 * whether the user likes it or not. Basically, if your app supports
 * both interactive and not (for example, if you use the
 * gnome-app-util interfaces), you should use
 * %GNOME_PREFERENCES_USER. Otherwise, use the setting you
 * support. Please note that "interactive" mode is not functional now;
 * GtkEntry is inadequate and so a custom widget will be written
 * eventually.
 *
 *
 * Returns:  Pointer to new GNOME appbar widget.
 **/

GtkWidget* 
gnome_appbar_new (gboolean has_progress,
		  gboolean has_status,
		  GnomePreferencesType interactivity)
{
  GnomeAppBar * ab = gtk_type_new (gnome_appbar_get_type ());

  gnome_appbar_construct(ab, has_progress, has_status, interactivity);

  return GTK_WIDGET(ab);
}


/**
 * gnome_appbar_construct
 * @ab: Pointer to GNOME appbar object.
 * @has_progress: %TRUE if appbar needs progress bar widget.
 * @has_status: %TRUE if appbar needs status bar widget.
 * @interactivity: See gnome_appbar_new() explanation.
 *
 * Description:
 * For use to bindings in languages other than C. Don't use.
 **/

void
gnome_appbar_construct(GnomeAppBar * ab,
		       gboolean has_progress,
		       gboolean has_status,
		       GnomePreferencesType interactivity)
{
  GtkBox *box;

  /* These checks are kind of gross because an unfinished object will
     be returned from _new instead of NULL */

  /* Can't be interactive if there's no status bar */
  g_return_if_fail( ((has_status == FALSE) && 
		     (interactivity == GNOME_PREFERENCES_NEVER)) ||
		    (has_status == TRUE)); 

  box = GTK_BOX (ab);

  box->spacing = GNOME_PAD_SMALL;
  box->homogeneous = FALSE;

  if (has_progress)
    ab->progress = gtk_progress_bar_new();
  else
    ab->progress = NULL;

  /*
   * If the progress meter goes on the right then we place it after we
   * create the status line.
   */
  if (has_progress && !gnome_preferences_get_statusbar_meter_on_right ())
    gtk_box_pack_start (box, ab->progress, FALSE, FALSE, 0);

  if ( has_status ) {
    if ( (interactivity == GNOME_PREFERENCES_ALWAYS) ||
	 ( (interactivity == GNOME_PREFERENCES_USER) &&
	   gnome_preferences_get_statusbar_interactive()) ) {
      ab->interactive = TRUE;
   
      ab->status = gtk_entry_new();

      gtk_signal_connect (GTK_OBJECT(ab->status), "delete_text",
			  GTK_SIGNAL_FUNC(entry_delete_text_cb),
			  ab);
      gtk_signal_connect (GTK_OBJECT(ab->status), "insert_text",
			  GTK_SIGNAL_FUNC(entry_insert_text_cb),
			  ab);
      gtk_signal_connect_after(GTK_OBJECT(ab->status), "key_press_event",
			       GTK_SIGNAL_FUNC(entry_key_press_cb),
			       ab);
      gtk_signal_connect(GTK_OBJECT(ab->status), "activate",
			 GTK_SIGNAL_FUNC(entry_activate_cb),
			 ab);

      /* no prompt now */
      gtk_entry_set_editable(GTK_ENTRY(ab->status), FALSE);

      gtk_box_pack_start (box, ab->status, TRUE, TRUE, 0);
    }
    else {
      GtkWidget * frame;
      
      ab->interactive = FALSE;

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_IN);
      
      ab->status = gtk_label_new ("");
      gtk_misc_set_alignment (GTK_MISC (ab->status), 0.0, 0.0);
      gtk_widget_set_usize (ab->status, 1, -1);
      
      gtk_box_pack_start (box, frame, TRUE, TRUE, 0);
      gtk_container_add (GTK_CONTAINER(frame), ab->status);
      
      gtk_widget_show (frame);
    }
  }
  else {
    ab->status = NULL;
    ab->interactive = FALSE;
  }

  if (has_progress && gnome_preferences_get_statusbar_meter_on_right ())
    gtk_box_pack_start (box, ab->progress, FALSE, FALSE, 0);

  if (ab->status) gtk_widget_show (ab->status);
  if (ab->progress) gtk_widget_show(ab->progress);
}


/**
 * gnome_appbar_set_prompt
 * @appbar: Pointer to GNOME appbar object.
 * @prompt: Text of the prompt message.
 * @modal: If %TRUE, grabs input.
 *
 * Description:
 * Put a prompt in the appbar and wait for a response. When the
 * user responds or cancels, a user_response signal is emitted.
 **/

void
gnome_appbar_set_prompt (GnomeAppBar * appbar, 
			 const gchar * prompt,
			 gboolean modal)
{
  g_return_if_fail(appbar != NULL);
  g_return_if_fail(prompt != NULL);
  g_return_if_fail(appbar->interactive);

  /* Remove anything old. */
  if (appbar->prompt) {
    gnome_appbar_clear_prompt(appbar);
  }

  appbar->prompt = g_strconcat(prompt, "  ", NULL);

  if (modal) gtk_grab_add(appbar->status);

  gnome_appbar_refresh(appbar);
}


/**
 * gnome_appbar_clear_prompt
 * @appbar: Pointer to GNOME appbar object
 *
 * Description:
 * Remove any prompt.
 **/

void       
gnome_appbar_clear_prompt    (GnomeAppBar * appbar)
{
  g_return_if_fail(appbar != NULL);
  g_return_if_fail(appbar->interactive);

  /* This isn't really right; most of Gtk would only emit here,
     and then have a clear_prompt_real as default handler. */
  
  g_free(appbar->prompt);
  appbar->prompt = NULL;

  gnome_appbar_refresh(appbar);

  gtk_signal_emit(GTK_OBJECT(appbar), 
		  appbar_signals[CLEAR_PROMPT]);  
}


/**
 * gnome_appbar_get_response
 * @appbar: Pointer to GNOME appbar object
 *
 * Description:
 * Get the response to the prompt, if any. Result must be g_free'd.
 *
 * Returns:
 * Text from appbar entry widget, as entered by user.
 **/

gchar *    
gnome_appbar_get_response    (GnomeAppBar * appbar)
{
  g_return_val_if_fail(appbar != NULL, NULL);
  g_return_val_if_fail(appbar->interactive, NULL);
  g_return_val_if_fail(appbar->prompt != NULL, NULL);
  
  /* This returns an allocated string. */
  return gtk_editable_get_chars(GTK_EDITABLE(appbar->status),
				appbar->editable_start, 
				GTK_ENTRY(appbar->status)->text_length);
} 


/**
 * gnome_appbar_refresh
 * @appbar: Pointer to GNOME appbar object
 *
 * Description:
 * Reflect the current state of stack/default. Useful to force a
 * set_status to disappear.
 **/

void 
gnome_appbar_refresh           (GnomeAppBar * appbar)
{
  g_return_if_fail(appbar != NULL);
  g_return_if_fail(GNOME_IS_APPBAR(appbar));
  
  if (appbar->prompt) {
    g_return_if_fail(appbar->interactive); /* Just a consistency check */
    gtk_entry_set_editable(GTK_ENTRY(appbar->status), TRUE);
    /* Allow insert_text to work, so we can set the prompt. */
    appbar->editable_start = 0;
    gtk_entry_set_text(GTK_ENTRY(appbar->status), appbar->prompt);
    /* This has to be after setting the text. */
    appbar->editable_start = strlen(appbar->prompt);   
    gtk_entry_set_position(GTK_ENTRY(appbar->status), 
			   appbar->editable_start);
    gtk_widget_grab_focus(appbar->status);
  }
  else {
    if (appbar->interactive) {
      appbar->editable_start = 0;
      gtk_entry_set_editable(GTK_ENTRY(appbar->status), FALSE);
      gtk_grab_remove(appbar->status); /* In case */
    }

    if (appbar->status_stack)
      gnome_appbar_set_status(appbar, stringstack_top(appbar->status_stack));
    else if (appbar->default_status)
      gnome_appbar_set_status(appbar, appbar->default_status);
    else 
      gnome_appbar_set_status(appbar, "");
  }
}


/**
 * gnome_appbar_set_status
 * @appbar: Pointer to GNOME appbar object.
 * @status: Text to which status label will be set.
 *
 * Description:
 * Sets the status label without changing widget state; next set or push
 * will destroy this permanently. 
 **/

void       
gnome_appbar_set_status       (GnomeAppBar * appbar,
			       const gchar * status)
{
  g_return_if_fail(appbar != NULL);
  g_return_if_fail(status != NULL);
  g_return_if_fail(GNOME_IS_APPBAR(appbar));

  if (appbar->interactive) 
    gtk_entry_set_text(GTK_ENTRY(appbar->status), status);
  else
    gtk_label_set_text(GTK_LABEL(appbar->status), status);
}


/**
 * gnome_appbar_set_default
 * @appbar: Pointer to GNOME appbar object
 * @default_status: Text for status label
 *
 * Description:
 * What to show when showing nothing else; defaults to nothing.
 **/

void	   
gnome_appbar_set_default      (GnomeAppBar * appbar,
			       const gchar * default_status)
{
  g_return_if_fail(appbar != NULL);
  g_return_if_fail(default_status != NULL);
  g_return_if_fail(GNOME_IS_APPBAR(appbar));
  
  if (appbar->default_status) g_free(appbar->default_status);
  appbar->default_status = g_strdup(default_status);
  gnome_appbar_refresh(appbar);
}


/**
 * gnome_appbar_push
 * @appbar: Pointer to GNOME appbar object
 * @status: Text of status message.
 *
 * Description:
 * Push a new status message onto the status bar stack, and
 * display it.
 **/

void       
gnome_appbar_push             (GnomeAppBar * appbar,
			       const gchar * status)
{
  g_return_if_fail(appbar != NULL);
  g_return_if_fail(status != NULL);
  g_return_if_fail(GNOME_IS_APPBAR(appbar));

  appbar->status_stack = stringstack_push(appbar->status_stack, status);
  gnome_appbar_refresh(appbar);
}


/**
 * gnome_appbar_pop
 * @appbar: Pointer to GNOME appbar object
 *
 * Description:
 * Remove current status message, and display previous status
 * message, if any.  It is OK to call this with an empty stack.
 **/

void       
gnome_appbar_pop              (GnomeAppBar * appbar)
{
  g_return_if_fail(appbar != NULL);
  g_return_if_fail(GNOME_IS_APPBAR(appbar));
  
  appbar->status_stack = stringstack_pop(appbar->status_stack);
  gnome_appbar_refresh(appbar);
}


/**
 * gnome_appbar_clear_stack
 * @appbar: Pointer to GNOME appbar object
 *
 * Description:
 * Remove all status messages from appbar, and display default status
 * message (if present).
 **/

void       
gnome_appbar_clear_stack      (GnomeAppBar * appbar)
{
  g_return_if_fail(appbar != NULL);
  g_return_if_fail(GNOME_IS_APPBAR(appbar));

  stringstack_free(appbar->status_stack);
  appbar->status_stack = NULL;
  gnome_appbar_refresh(appbar);
}


/**
 * gnome_appbar_set_progress
 * @appbar: Pointer to GNOME appbar object
 * @percentage: Percentage to which progress bar should be set.
 *
 * Description:
 * Sets progress bar to the given percentage.
 * Pure sugar - with a bad name, in light of the get_progress name
 * which is not the opposite of set_progress. Maybe this function
 * should die. 
 **/

void
gnome_appbar_set_progress(GnomeAppBar *appbar,
			  gfloat percentage)
{
  g_return_if_fail (appbar != NULL);
  g_return_if_fail (appbar->progress != NULL);
  g_return_if_fail (GNOME_IS_APPBAR(appbar));

  gtk_progress_bar_update(GTK_PROGRESS_BAR(appbar->progress), percentage);
}


/**
 * gnome_appbar_get_progress
 * @appbar: Pointer to GNOME appbar object
 *
 * Description:
 * Returns &GtkProgress widget pointer, so that the progress bar may be
 * manipulated further.
 *
 * Returns:
 * Pointer to appbar's progress bar object. May be NULL if the appbar
 * has no progress object.
 **/

GtkProgress* 
gnome_appbar_get_progress    (GnomeAppBar * appbar)
{
  g_return_val_if_fail(appbar != NULL, NULL);
  g_return_val_if_fail(appbar->progress != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_APPBAR(appbar), NULL);

  return GTK_PROGRESS(appbar->progress);
}

static void
gnome_appbar_destroy (GtkObject *object)
{
  GnomeAppBar *ab;
  GnomeAppBarClass *class;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GNOME_IS_APPBAR (object));

  ab = GNOME_APPBAR (object);
  class = GNOME_APPBAR_CLASS (GTK_OBJECT (ab)->klass);

  gnome_appbar_clear_stack(ab);
  /* g_free checks if these are NULL */
  g_free(ab->default_status);
  g_free(ab->prompt);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}


