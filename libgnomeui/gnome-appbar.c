/* gnome-appbar.h: statusbar/progress/minibuffer widget for Gnome apps
 *
 * This version by Havoc Pennington
 * Based on MozStatusbar widget, by Chris Toshok
 * In turn based on GtkStatusbar widget, from Gtk+,
 * Copyright (C) 1995-1998 Peter Mattis, Spencer Kimball and Josh MacDonald
 * GtkStatusbar Copyright (C) 1998 Shawn T. Amundson
 * All rights reserved.
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
/*
  @NOTATION@
 */

#include "config.h"
#include <libgnome/gnome-macros.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>

#include <string.h> /* for strlen */

#include "gnome-appbar.h"
#include "gnome-gconf-ui.h"

#include <libgnome/gnome-util.h>
#include <libgnome/gnome-i18n.h>

#include "gnome-uidefs.h"
#include "gnometypebuiltins.h"

struct _GnomeAppBarPrivate
{
  /* Private; there's no guarantee on the type of these in the
     future. Statusbar could be a label, entry, GtkStatusbar, or
     something else; progress could be a label or progress bar; it's
     all up in the air for now. */
  /* there is no reason for these not to be properties */
  GtkWidget * progress;
  GtkWidget * status;
  gchar * prompt; /* The text of a prompt, if any. */

  /* Keep it simple; no contexts.
     if (status_stack) display_top_of_stack;
     else if (default_status) display_default;
     else display_nothing;      */
  /* Also private by the way */
  GSList * status_stack;
  gchar  * default_status;

  gint16 editable_start; /* The first editable position in the interactive
			  buffer. */

  /* these are construct only currently. there is no reason this
   * couldn't be changed. */

  GnomePreferencesType interactivity;
  gboolean interactive : 1; /* This means status is an entry rather than a
			       label, for the moment. */
  gboolean has_progress : 1;
  gboolean has_status : 1;
};

enum {
  USER_RESPONSE,
  CLEAR_PROMPT,
  LAST_SIGNAL
};

enum {
	PROP_0,
	PROP_HAS_PROGRESS,
	PROP_HAS_STATUS,
	PROP_INTERACTIVITY
};

static gint appbar_signals[LAST_SIGNAL] = { 0 };

GNOME_CLASS_BOILERPLATE (GnomeAppBar, gnome_appbar,
			 GtkHBox, GTK_TYPE_HBOX)

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
  g_return_if_fail(ab->_priv->interactive);

  if (ab->_priv->prompt == NULL) return; /* not prompting, so don't
				     interfere. */
  if (start < ab->_priv->editable_start) {
    /* Block the signal, since it's trying to delete text
       that shouldn't be deleted. */
    g_signal_stop_emission_by_name(entry, "delete_text");
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

  if (ab->_priv->prompt == NULL) return; /* not prompting, so don't
				     interfere. */

  pos = gtk_editable_get_position(entry);

  if (pos < ab->_priv->editable_start) {

    /*    gtk_editable_set_position(entry, ab->_priv->editable_start); */
    g_signal_stop_emission_by_name(entry, "insert_text");
    /*    g_signal_emit_by_name(entry, "insert_text", text, length, position); */
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
  g_signal_emit(ab, appbar_signals[USER_RESPONSE], 0);
}

static void
gnome_appbar_instance_init (GnomeAppBar *ab)
{
  ab->_priv                 = g_new0(GnomeAppBarPrivate, 1);
  ab->_priv->default_status = NULL;
  ab->_priv->status_stack   = NULL;
  ab->_priv->editable_start = 0;
  ab->_priv->prompt         = NULL;
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
  return GTK_WIDGET (g_object_new (GNOME_TYPE_APPBAR,
				   "has_progress", has_progress,
				   "has_status", has_status,
				   "interactivity", interactivity,
				   NULL));
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
  g_return_if_fail(appbar->_priv->interactive);

  /* Remove anything old. */
  if (appbar->_priv->prompt) {
    gnome_appbar_clear_prompt(appbar);
  }

  appbar->_priv->prompt = g_strconcat(prompt, "  ", NULL);

  if (modal) gtk_grab_add(appbar->_priv->status);

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
  g_return_if_fail(appbar->_priv->interactive);

  /* This isn't really right; most of Gtk would only emit here,
     and then have a clear_prompt_real as default handler. */

  g_free(appbar->_priv->prompt);
  appbar->_priv->prompt = NULL;

  gnome_appbar_refresh(appbar);

  g_signal_emit (appbar, appbar_signals[CLEAR_PROMPT], 0);
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
  g_return_val_if_fail(appbar->_priv->interactive, NULL);

  /* This returns an allocated string. */
  return gtk_editable_get_chars(GTK_EDITABLE(appbar->_priv->status),
				appbar->_priv->editable_start,
				GTK_ENTRY(appbar->_priv->status)->text_length);
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

  if (appbar->_priv->prompt) {
    g_return_if_fail(appbar->_priv->interactive); /* Just a consistency check */
    gtk_editable_set_editable(GTK_EDITABLE(appbar->_priv->status), TRUE);
    /* Allow insert_text to work, so we can set the prompt. */
    appbar->_priv->editable_start = 0;
    gtk_entry_set_text(GTK_ENTRY(appbar->_priv->status), appbar->_priv->prompt);
    /* This has to be after setting the text. */
    appbar->_priv->editable_start = strlen(appbar->_priv->prompt);
    gtk_editable_set_position(GTK_EDITABLE(appbar->_priv->status),
			      appbar->_priv->editable_start);
    gtk_widget_grab_focus(appbar->_priv->status);
  }
  else {
    if (appbar->_priv->interactive) {
      appbar->_priv->editable_start = 0;
      gtk_editable_set_editable(GTK_EDITABLE(appbar->_priv->status), FALSE);
      gtk_grab_remove(appbar->_priv->status); /* In case */
    }

    if (appbar->_priv->status_stack)
      gnome_appbar_set_status(appbar, stringstack_top(appbar->_priv->status_stack));
    else if (appbar->_priv->default_status)
      gnome_appbar_set_status(appbar, appbar->_priv->default_status);
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

  if (appbar->_priv->interactive)
    gtk_entry_set_text(GTK_ENTRY(appbar->_priv->status), status);
  else
    gtk_label_set_text(GTK_LABEL(appbar->_priv->status), status);
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

  /* g_free handles NULL */
  g_free(appbar->_priv->default_status);
  appbar->_priv->default_status = g_strdup(default_status);
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

  appbar->_priv->status_stack = stringstack_push(appbar->_priv->status_stack, status);
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

  appbar->_priv->status_stack = stringstack_pop(appbar->_priv->status_stack);
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

  stringstack_free(appbar->_priv->status_stack);
  appbar->_priv->status_stack = NULL;
  gnome_appbar_refresh(appbar);
}


/**
 * gnome_appbar_set_progress_precentage
 * @appbar: Pointer to GNOME appbar object
 * @percentage: Percentage to which progress bar should be set.
 *
 * Description:
 * Sets progress bar to the given percentage.
 **/

void
gnome_appbar_set_progress_percentage(GnomeAppBar *appbar,
				     gfloat percentage)
{
  g_return_if_fail (appbar != NULL);
  g_return_if_fail (appbar->_priv->progress != NULL);
  g_return_if_fail (GNOME_IS_APPBAR(appbar));

  gtk_progress_bar_set_fraction(GTK_PROGRESS_BAR(appbar->_priv->progress), percentage);
}


/**
 * gnome_appbar_get_progress
 * @appbar: Pointer to GNOME appbar object
 *
 * Description:
 * Returns &GtkProgressBar widget pointer, so that the progress bar may be
 * manipulated further.
 *
 * Returns:
 * Pointer to appbar's progress bar object. May be NULL if the appbar
 * has no progress object.
 **/

GtkProgressBar*
gnome_appbar_get_progress    (GnomeAppBar * appbar)
{
  g_return_val_if_fail(appbar != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_APPBAR(appbar), NULL);
  g_return_val_if_fail(appbar->_priv->progress != NULL, NULL);

  return GTK_PROGRESS_BAR(appbar->_priv->progress);
}

/**
 * gnome_appbar_get_status
 * @appbar: Pointer to GNOME appbar object
 *
 * Description:
 * Returns the statusbar widget.
 *
 * Returns:
 * A pointer to the statusbar widget.
 **/
GtkWidget *
gnome_appbar_get_status    (GnomeAppBar * appbar)
{
  g_return_val_if_fail(appbar != NULL, NULL);
  g_return_val_if_fail(GNOME_IS_APPBAR(appbar), NULL);

  return (appbar->_priv->status);
}

static void
gnome_appbar_finalize (GObject *object)
{
  GnomeAppBar *ab;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GNOME_IS_APPBAR (object));

  ab = GNOME_APPBAR (object);

  /* stringstack_free handles null correctly */
  stringstack_free (ab->_priv->status_stack);
  ab->_priv->status_stack = NULL;

  /* g_free checks if these are NULL */
  g_free (ab->_priv->default_status);
  ab->_priv->default_status = NULL;
  g_free (ab->_priv->prompt);
  ab->_priv->prompt = NULL;

  g_free (ab->_priv);
  ab->_priv = NULL;

  GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

static GObject *
gnome_appbar_constructor (GType                  type,
			  guint                  n_properties,
			  GObjectConstructParam *properties)
{
  GObject *object;
  GnomeAppBar *ab;
  GtkBox *box;
  gboolean has_status, has_progress, interactive;

  object = G_OBJECT_CLASS (parent_class)->constructor (type,
						       n_properties,
						       properties);

  ab = GNOME_APPBAR (object);

  has_status   = ab->_priv->has_status;
  has_progress = ab->_priv->has_progress;
  interactive  = ab->_priv->interactive;

  box = GTK_BOX (ab);

  box->spacing = GNOME_PAD_SMALL;
  box->homogeneous = FALSE;

  if (has_progress)
    ab->_priv->progress = gtk_progress_bar_new ();

  /*
   * If the progress meter goes on the right then we place it after we
   * create the status line.
   */
  if (has_progress &&
      /* FIXME: this should listen to changes! */
      ! gnome_gconf_get_bool ("/desktop/gnome/interface/statusbar_meter_on_right"))
    gtk_box_pack_start (box, ab->_priv->progress, FALSE, FALSE, 0);

  if ( has_status ) {
    if ( interactive ) {
      ab->_priv->status = gtk_entry_new();

      g_signal_connect (ab->_priv->status, "delete_text",
			G_CALLBACK(entry_delete_text_cb),
			ab);
      g_signal_connect (ab->_priv->status, "insert_text",
			G_CALLBACK(entry_insert_text_cb),
			ab);
      g_signal_connect_after (ab->_priv->status, "key_press_event",
			      G_CALLBACK(entry_key_press_cb),
			      ab);
      g_signal_connect (ab->_priv->status, "activate",
			G_CALLBACK(entry_activate_cb),
			ab);

      /* no prompt now */
      gtk_editable_set_editable(GTK_EDITABLE(ab->_priv->status), FALSE);

      gtk_box_pack_start (box, ab->_priv->status, TRUE, TRUE, 0);
    } else {
      GtkWidget * frame;

      frame = gtk_frame_new (NULL);
      gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_IN);

      ab->_priv->status = gtk_label_new ("");
      gtk_misc_set_alignment (GTK_MISC (ab->_priv->status), 0.0, 0.0);
      gtk_widget_set_size_request (ab->_priv->status, 1, -1);

      gtk_box_pack_start (box, frame, TRUE, TRUE, 0);
      gtk_container_add (GTK_CONTAINER(frame), ab->_priv->status);

      gtk_widget_show (frame);
    }
  }

  if (has_progress &&
      /* FIXME: this should listen to changes! */
      gnome_gconf_get_bool ("/desktop/gnome/interface/statusbar_meter_on_right"))
    gtk_box_pack_start (box, ab->_priv->progress, FALSE, FALSE, 0);

  if (ab->_priv->status) gtk_widget_show (ab->_priv->status);
  if (ab->_priv->progress) gtk_widget_show(ab->_priv->progress);

  return object;
}

static void
gnome_appbar_set_property (GObject      *object,
			   guint         prop_id,
			   const GValue *value,
			   GParamSpec   *pspec)
{
  GnomeAppBarPrivate *priv = GNOME_APPBAR (object)->_priv;
  switch (prop_id) {
  case PROP_HAS_STATUS:
    priv->has_status = g_value_get_boolean (value);
    break;
  case PROP_HAS_PROGRESS:
    priv->has_progress = g_value_get_boolean (value);
    break;
  case PROP_INTERACTIVITY:
    priv->interactivity = g_value_get_enum (value);
    switch (priv->interactivity) {
    case GNOME_PREFERENCES_NEVER:
      priv->interactive = FALSE;
      break;
    case GNOME_PREFERENCES_ALWAYS:
      priv->interactive = TRUE;
      break;
    default:
      priv->interactive = gnome_gconf_get_bool ("/desktop/gnome/interface/statusbar-interactive");
      break;
    }
  }
}

static void
gnome_appbar_get_property (GObject    *object,
			   guint       prop_id,
			   GValue     *value,
			   GParamSpec *pspec)
{
  GnomeAppBarPrivate *priv = GNOME_APPBAR (object)->_priv;

  switch (prop_id) {
  case PROP_HAS_STATUS:
    g_value_set_boolean (value, priv->has_status);
    break;
  case PROP_HAS_PROGRESS:
    g_value_set_boolean (value, priv->has_progress);
    break;
  case PROP_INTERACTIVITY:
    g_value_set_enum (value, priv->interactivity);
    break;
  }
}

static void
gnome_appbar_class_init (GnomeAppBarClass *class)
{
  GtkObjectClass *object_class;
  GObjectClass *gobject_class;
  GtkWidgetClass *widget_class;
  GtkContainerClass *container_class;

  object_class = (GtkObjectClass *) class;
  gobject_class = (GObjectClass *) class;
  widget_class = (GtkWidgetClass *) class;
  container_class = (GtkContainerClass *) class;

  parent_class = GTK_HBOX_CLASS (gtk_type_class (GTK_TYPE_HBOX));

  gobject_class->constructor  = gnome_appbar_constructor;
  gobject_class->get_property = gnome_appbar_get_property;
  gobject_class->set_property = gnome_appbar_set_property;

  appbar_signals[USER_RESPONSE] =
    g_signal_new ("user_response",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GnomeAppBarClass, user_response),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  appbar_signals[CLEAR_PROMPT] =
    g_signal_new ("clear_prompt",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GnomeAppBarClass, clear_prompt),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  g_object_class_install_property (
	  gobject_class,
	  PROP_HAS_PROGRESS,
	  g_param_spec_boolean ("has_progress",
				_("Has Progress"),
				_("Create a progress widget."),
				FALSE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (
	  gobject_class,
	  PROP_HAS_STATUS,
	  g_param_spec_boolean ("has_status",
				_("Has Status"),
				_("Create a status widget."),
				FALSE, G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  g_object_class_install_property (
	  gobject_class,
	  PROP_INTERACTIVITY,
	  g_param_spec_enum ("interactivity",
			     _("Interactivity"),
			     _("Level of user activity required."),
			     GNOME_TYPE_PREFERENCES_TYPE,
			     GNOME_PREFERENCES_NEVER,
			     G_PARAM_READWRITE | G_PARAM_CONSTRUCT));

  class->user_response = NULL;
  class->clear_prompt  = NULL; /* maybe should have a handler
				  and the clear_prompt function
				  just emits. */
  gobject_class->finalize = gnome_appbar_finalize;
}
