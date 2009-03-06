/* gnome-client.c - GNOME session management client support
 *
 * Copyright (C) 1998, 1999 Carsten Schaar
 * All rights reserved.
 *
 * Author: Carsten Schaar <nhadcasc@fs-maphy.uni-hannover.de>
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

#define _XOPEN_SOURCE 500

#include <config.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

/* Must be before all other gnome includes!! */
#include <glib/gi18n-lib.h>

#include "gnome-client.h"
#include "gnome-uidefs.h"
#include "gnome-ui-init.h"
#include "gnome-ice.h"
#include "gnome-marshal.h"
#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>

#include <libgnomeuiP.h>

#ifdef HAVE_LIBSM
#include <X11/SM/SMlib.h>
#endif /* HAVE_LIBSM */

static GtkWidget *client_grab_widget = NULL;
static gboolean sm_connect_default = TRUE;

enum {
  SAVE_YOURSELF,
  DIE,
  SAVE_COMPLETE,
  SHUTDOWN_CANCELLED,
  CONNECT,
  DISCONNECT,
  LAST_SIGNAL
};

static void gnome_real_client_destroy            (GtkObject        *object);
static void gnome_real_client_finalize           (GObject          *object);
static void gnome_real_client_save_complete      (GnomeClient      *client);
static void gnome_real_client_shutdown_cancelled (GnomeClient      *client);
static void gnome_real_client_connect            (GnomeClient      *client,
						  gboolean          restarted);
static void gnome_real_client_disconnect         (GnomeClient      *client);
static void master_client_connect (GnomeClient *client,
				   gboolean         restarted,
				   gpointer     client_data);
static void master_client_disconnect (GnomeClient *client,
				      gpointer     client_data);


static void   client_unset_config_prefix    (GnomeClient *client);

static gchar** array_init_from_arg           (gint argc,
					      gchar *argv[]);

static gint client_signals[LAST_SIGNAL] = { 0 };

static const char *sm_client_id_arg_name G_GNUC_UNUSED = "--sm-client-id";
static const char *sm_config_prefix_arg_name G_GNUC_UNUSED = "--sm-config-prefix";
#ifdef HAVE_GTK_MULTIHEAD
static const char *sm_screen G_GNUC_UNUSED = "--screen";
#endif

G_GNUC_INTERNAL extern void _gnome_ui_gettext_init (gboolean bind_codeset);

/* The master client.  */
static GnomeClient *master_client= NULL;

static gboolean master_client_restored= FALSE;

/*****************************************************************************/
/* Managing the interaction keys. */

/* The following function handle the interaction keys.  Each time an
   application requests interaction an interaction key is created.  If
   the session manager decides, that a client may interact now, the
   application is given the according interaction key.  No other
   interaction may take place, until the application returns the
   application key.  */

#ifdef HAVE_LIBSM

typedef struct _InteractionKey InteractionKey;


struct _InteractionKey
{
  gint                   tag;
  GnomeClient           *client;
  GnomeDialogType        dialog_type;
  gboolean               in_use;
  gboolean               interp;
  GnomeInteractFunction  function;
  gpointer               data;
  GDestroyNotify       destroy;
};


/* List of all existing interaction keys.  */
static GList *interact_functions = NULL;


static InteractionKey *
interaction_key_new (GnomeClient           *client,
		     GnomeDialogType        dialog_type,
		     gboolean               interp,
		     GnomeInteractFunction  function,
		     gpointer               data,
		     GDestroyNotify       destroy)
{
  static gint tag= 1;

  InteractionKey *key= g_new (InteractionKey, 1);

  if (key)
    {
      key->tag        = tag++;
      key->client     = client;
      key->dialog_type= dialog_type;
      key->in_use     = FALSE;
      key->interp     = interp;
      key->function   = function;
      key->data       = data;
      key->destroy    = destroy;

      interact_functions= g_list_append (interact_functions, key);
    }

  return key;
}


static void
interaction_key_destroy (InteractionKey *key)
{
  interact_functions= g_list_remove (interact_functions, key);

  if (key->destroy)
    (key->destroy) (key->data);

  g_free (key);
}


static void
interaction_key_destroy_if_possible (InteractionKey *key)
{
  if (key->in_use)
    key->client = NULL;
  else
    interaction_key_destroy (key);
}


static InteractionKey *
interaction_key_find_by_tag (gint tag)
{
  InteractionKey *key;
  GList          *tmp= interact_functions;

  while (tmp)
    {
      key= (InteractionKey *) tmp->data;

      if (key->tag == tag)
	return key;

      tmp= tmp->next;
    }

  return NULL;
}


static void
interaction_key_use (InteractionKey *key)
{
  key->in_use= TRUE;

  if (!key->interp)
    key->function (key->client, key->tag, key->dialog_type, key->data);
#if !defined(GNOME_DISABLE_DEPRECATED) && !defined(GTK_DISABLE_DEPRECATED)
  else
    {
      GtkArg args[4];

      args[0].name          = NULL;
      args[0].type          = GTK_TYPE_NONE;

      args[1].name          = NULL;
      args[1].type          = GTK_TYPE_OBJECT;
      args[1].d.pointer_data= &key->client;

      args[2].name          = NULL;
      args[2].type          = GTK_TYPE_INT;
      args[2].d.pointer_data= &key->tag;

      args[3].name          = "GnomeDialogType";
      args[3].type          = GNOME_TYPE_DIALOG_TYPE;
      args[3].d.pointer_data= &key->dialog_type;

      ((GtkCallbackMarshal)key->function) (NULL, key->data, 3, args);
    }
#endif
}

#endif /* HAVE_LIBSM */


/*****************************************************************************/
/* Helper functions, that set session management properties.  */

/* The following functions are used to set and unset session
   management properties of a special type.  */

#ifdef HAVE_LIBSM


static void
client_set_value (GnomeClient *client,
		  gchar       *name,
		  char        *type,
		  int          num_vals,
		  SmPropValue *vals)
{
  SmProp *proplist[1];
  SmProp prop;

  prop.name = name;
  prop.type = type;
  prop.num_vals = num_vals;
  prop.vals = vals;

  proplist[0]= &prop;
  SmcSetProperties ((SmcConn) client->smc_conn, 1, proplist);
}


static void
client_set_string (GnomeClient *client, gchar *name, gchar *value)
{
  SmPropValue val;

  g_return_if_fail (name);

  if (!GNOME_CLIENT_CONNECTED (client) || (value == NULL))
    return;

  val.length = strlen (value)+1;
  val.value  = value;

  client_set_value (client, name, SmARRAY8, 1, &val);
}


static void
client_set_gchar (GnomeClient *client, gchar *name, gchar value)
{
  SmPropValue val;

  g_return_if_fail (name);

  if (!GNOME_CLIENT_CONNECTED (client))
    return;

  val.length = 1;
  val.value  = &value;

  client_set_value (client, name, SmCARD8, 1, &val);
}


static void
client_set_ghash0 (gchar *key, gchar *value, SmPropValue **vals)
{
  (*vals)->length= strlen (key);
  (*vals)->value = key;
  (*vals)++;

  (*vals)->length= strlen (value);
  (*vals)->value = value;
  (*vals)++;
}

static void
client_set_ghash (GnomeClient *client, gchar *name, GHashTable *table)
{
  gint    argc;
  SmPropValue *vals;
  SmPropValue *tmp;

  g_return_if_fail (name);
  g_return_if_fail (table);

  if (!GNOME_CLIENT_CONNECTED (client))
    return;

  /* multiply by 2 because for each element
   * we have a key and a value
   */
  argc = 2 * g_hash_table_size (table);

  if (argc == 0)
    return;

  /* Now initialize the 'vals' array.  */
  vals = g_new (SmPropValue, argc);
  tmp = vals;

  g_hash_table_foreach (table, (GHFunc) client_set_ghash0, &tmp);

  client_set_value (client, name, SmLISTofARRAY8, argc, vals);

  g_free (vals);

}


static void
client_set_array (GnomeClient *client, gchar *name, gchar *array[])
{
  gint    argc;
  gchar **ptr;
  gint    i;

  SmPropValue *vals;

  g_return_if_fail (name);

  if (!GNOME_CLIENT_CONNECTED (client) || (array == NULL))
    return;

  /* We count the number of elements in our array.  */
  for (ptr = array, argc = 0; *ptr ; ptr++, argc++) /* LOOP */;

  /* Now initialize the 'vals' array.  */
  vals = g_new (SmPropValue, argc);
  for (ptr = array, i = 0 ; i < argc ; ptr++, i++)
    {
      vals[i].length = strlen (*ptr);
      vals[i].value  = *ptr;
    }

  client_set_value (client, name, SmLISTofARRAY8, argc, vals);

  g_free (vals);
}

static void
client_set_clone_command (GnomeClient *client)
{
  GList  *list;
  gint    argc;
  gchar **ptr;
  gint    i = 0;

  SmPropValue *vals;

  if (!GNOME_CLIENT_CONNECTED (client))
    return;

  ptr=client->clone_command ? client->clone_command : client->restart_command;

  if (!ptr)
    return;

  for (argc = 0; *ptr ; ptr++) argc++;

  /* Add space for static arguments and config prefix. */
  argc += g_list_length (client->static_args) + 2;

  vals = g_new (SmPropValue, argc);

  ptr=client->clone_command ? client->clone_command : client->restart_command;

  vals[i].length = strlen (*ptr);
  vals[i++].value  = *ptr++;

  if (client->config_prefix)
    {
      vals[i].length = strlen (sm_config_prefix_arg_name);
      vals[i++].value = (char *) sm_config_prefix_arg_name;
      vals[i].length = strlen (client->config_prefix);
      vals[i++].value = client->config_prefix;
    }

  for (list = client->static_args; list; list= g_list_next (list))
    {
      vals[i].length= strlen ((gchar *)list->data);
      vals[i++].value = (gchar *)list->data;
    }

  while (*ptr)
    {
      vals[i].length = strlen (*ptr);
      vals[i++].value  = *ptr++;
    }

  client_set_value (client, SmCloneCommand, SmLISTofARRAY8, i, vals);

  g_free (vals);
}

static void
client_set_restart_command (GnomeClient *client)
{
  GList  *list;
  gint    argc;
  gchar **ptr;
  gint    i = 0;

#ifdef HAVE_GTK_MULTIHEAD
  gint disp;
  gchar *tmp;
#endif

  SmPropValue *vals;

  if (!GNOME_CLIENT_CONNECTED (client) || (client->restart_command == NULL))
    return;

  for (ptr = client->restart_command, argc = 0; *ptr ; ptr++) argc++;

  /* Add space for static arguments, config prefix and client id. */
  argc += g_list_length (client->static_args) + 4;

#ifdef HAVE_GTK_MULTIHEAD
  /* add two more for --screen */
  argc += 2;
#endif

  vals = g_new (SmPropValue, argc);

  ptr = client->restart_command;

  vals[i].length = strlen (*ptr);
  vals[i++].value  = *ptr++;

  if (client->config_prefix)
    {
      vals[i].length = strlen (sm_config_prefix_arg_name);
      vals[i++].value = (char *) sm_config_prefix_arg_name;
      vals[i].length = strlen (client->config_prefix);
      vals[i++].value = client->config_prefix;
    }
  vals[i].length = strlen (sm_client_id_arg_name);
  vals[i++].value = (char *) sm_client_id_arg_name;
  vals[i].length = strlen (client->client_id);
  vals[i++].value = client->client_id;

#ifdef HAVE_GTK_MULTIHEAD
  /* set the screen number */
  disp = gdk_screen_get_number (gdk_screen_get_default ());
  tmp = g_strdup_printf ("%d", disp);
  vals[i].length = strlen (sm_screen);
  vals[i++].value = (char *)sm_screen;
  vals[i].length = strlen (tmp);
  vals[i++].value = tmp;
#endif

  for (list = client->static_args; list; list = g_list_next (list))
    {
      vals[i].length= strlen ((gchar *)list->data);
      vals[i++].value = (gchar *)list->data;
    }

  while (*ptr)
    {
      vals[i].length = strlen (*ptr);
      vals[i++].value  = *ptr++;
    }

  client_set_value (client, SmRestartCommand, SmLISTofARRAY8, i, vals);

  g_free (vals);

#ifdef HAVE_GTK_MULTIHEAD
  g_free (tmp);
#endif
}

static void
client_unset (GnomeClient *client, gchar *name)
{
  g_return_if_fail (name  != NULL);

  if (GNOME_CLIENT_CONNECTED (client))
    SmcDeleteProperties ((SmcConn) client->smc_conn, 1, &name);
}

#endif /* HAVE_LIBSM */

static void
client_set_state (GnomeClient *client, GnomeClientState state)
{
  client->state= state;
}


/*****************************************************************************/
/* Callback functions  */

/* The following functions are callback functions (and helper
   functions).  They are registered in 'gnome_client_connect',
   'gnome_interaction_key_return' and
   'gnome_client_request_interaction_internal',
   'client_save_yourself_callback'.  */

#ifdef HAVE_LIBSM


static void
client_save_phase_2_callback (SmcConn smc_conn, SmPointer client_data);

static void
client_save_yourself_possibly_done (GnomeClient *client)
{
  if (client->interaction_keys)
    return;

  if ((client->state == GNOME_CLIENT_SAVING_PHASE_1) &&
      client->save_phase_2_requested)
    {
      Status status;

      status= SmcRequestSaveYourselfPhase2 ((SmcConn) client->smc_conn,
					    client_save_phase_2_callback,
					    (SmPointer) client);

      if (status)
	client_set_state (client, GNOME_CLIENT_WAITING_FOR_PHASE_2);
    }

  if ((client->state == GNOME_CLIENT_SAVING_PHASE_1) ||
      (client->state == GNOME_CLIENT_SAVING_PHASE_2))
    {
      SmcSaveYourselfDone ((SmcConn) client->smc_conn,
			   client->save_successfull);

      if (client->shutdown)
	client_set_state (client, GNOME_CLIENT_FROZEN);
      else
	client_set_state (client, GNOME_CLIENT_IDLE);
    }
}


static void
client_save_phase_2_callback (SmcConn smc_conn, SmPointer client_data)
{
  GnomeClient *client= (GnomeClient*) client_data;
  gboolean ret;

  client_set_state (client, GNOME_CLIENT_SAVING_PHASE_2);

  g_signal_emit (client, client_signals[SAVE_YOURSELF], 0,
		 2,
		 client->save_style,
		 client->shutdown,
		 client->interact_style,
		 client->fast,
		 &ret);

  client_save_yourself_possibly_done (client);
}

static gint
end_wait (gpointer data)
{
  gboolean *waiting = (gboolean*)data;
  *waiting = FALSE;
  return 0;
}

static void
client_save_yourself_callback (SmcConn   smc_conn,
			       SmPointer client_data,
			       int       save_style,
			       gboolean  shutdown,
			       int       interact_style,
			       gboolean  fast)
{
  GnomeClient *client= (GnomeClient*) client_data;
  gchar *name, *prefix;
  int fd, len;
  gboolean ret;

  if (!client_grab_widget)
    {
      GDK_THREADS_ENTER();
      client_grab_widget = gtk_widget_new (GTK_TYPE_INVISIBLE, NULL);
      GDK_THREADS_LEAVE();
    }

  /* The first SaveYourself after registering for the first time
   * is a special case (SM specs 7.2).
   *
   * This SaveYourself seems to be included in the protocol to
   * ask the client to specify its initial SmProperties since
   * there is little point saving a copy of the initial state.
   *
   * A bug in xsm means that it does not send us a SaveComplete
   * in response to this initial SaveYourself. Therefore, we
   * must not set a grab because it would never be released.
   * Indeed, even telling the app that this SaveYourself has
   * arrived is hazardous as the app may take its own steps
   * to freeze its WM state while waiting for the SaveComplete.
   *
   * Fortunately, we have already set the SmProperties during
   * gnome_client_connect so there is little lost in simply
   * returning immediately.
   *
   * Apps which really want to save their initial states can
   * do so safely using gnome_client_save_yourself_request. */

  if (client->state == GNOME_CLIENT_REGISTERING)
    {
      client_set_state (client, GNOME_CLIENT_IDLE);

      /* Double check that this is a section 7.2 SaveYourself: */

      if (save_style == SmSaveLocal &&
	  interact_style == SmInteractStyleNone &&
	  !shutdown && !fast)
	{
	  /* The protocol requires this even if xsm ignores it. */
	  SmcSaveYourselfDone ((SmcConn) client->smc_conn, TRUE);
	  return;
	}
    }

  switch (save_style)
    {
    case SmSaveGlobal:
      client->save_style= GNOME_SAVE_GLOBAL;
      break;

    case SmSaveLocal:
      client->save_style= GNOME_SAVE_LOCAL;
      break;

    case SmSaveBoth:
    default:
      client->save_style= GNOME_SAVE_BOTH;
      break;
    }
  client->shutdown= shutdown;
  switch (interact_style)
    {
    case SmInteractStyleErrors:
      client->interact_style= GNOME_INTERACT_ERRORS;
      break;

    case SmInteractStyleAny:
      client->interact_style= GNOME_INTERACT_ANY;
      break;

    case SmInteractStyleNone:
    default:
      client->interact_style= GNOME_INTERACT_NONE;
      break;
    }
  client->fast = fast;

  client->save_phase_2_requested= FALSE;
  client->save_successfull      = TRUE;
  client->save_yourself_emitted = FALSE;

  client_set_state (client, GNOME_CLIENT_SAVING_PHASE_1);

  GDK_THREADS_ENTER();

  if (gdk_pointer_is_grabbed())
    {
      gboolean waiting = TRUE;
      gint id = g_timeout_add (4000, end_wait, &waiting);

      while (gdk_pointer_is_grabbed() && waiting)
	gtk_main_iteration();
      g_source_remove (id);
    }

  /* Check that we did not receive a shutdown cancelled while waiting
   * for the grab to be released. The grab should prevent it but ... */
  if (client->state != GNOME_CLIENT_SAVING_PHASE_1)
    {
      GDK_THREADS_LEAVE();
      return;
    }

  gdk_pointer_ungrab (GDK_CURRENT_TIME);
  gdk_keyboard_ungrab (GDK_CURRENT_TIME);
  gtk_grab_add (client_grab_widget);

  GDK_THREADS_LEAVE();

  name = g_strdup (gnome_client_get_global_config_prefix(client));
  name[strlen (name) - 1] = '\0';

  prefix = g_strconcat (name, "-XXXXX", "/", NULL);
  g_free (name);
  len = strlen (prefix);

  name = gnome_config_get_real_path (prefix);
  g_free (prefix);

  name [strlen (name) - 1] = 'X';
  fd = mkstemp (name);

  if (fd != -1)
    {
      unlink (name);
      close (fd);
      g_free (client->config_prefix);
      client->config_prefix = g_strconcat (name+strlen(name) - len, "/", NULL);

      if (client == master_client)
	{
	  /* The config prefix has been changed, so it cannot be used
             anymore to restore a saved session.  */
	  master_client_restored= FALSE;
	}
    }
  g_free (name);

  client_set_clone_command (client);
  client_set_restart_command (client);

  g_signal_emit (client, client_signals[SAVE_YOURSELF], 0,
		 1,
		 client->save_style,
		 shutdown,
		 client->interact_style,
		 fast,
		 &ret);

#ifdef BREAK_KDE_SESSION_MANAGER
  /* <jsh> The KDE session manager actually cares about the `success'
     field of the SaveYourselfDone message (unlike gnome-session, which
     totally ignores it. Hence the code below has the effect of making
     KDE unable to shutdown when any GNOME apps are running that haven't
     connected to the "save_yourself" signal. */

  if (!client->save_yourself_emitted)
    client->save_successfull= FALSE;
#endif

  client_save_yourself_possibly_done (client);
}


static void
client_die_callback (SmcConn smc_conn, SmPointer client_data)
{
  GnomeClient *client= (GnomeClient*) client_data;

  if (client_grab_widget)
    {
      GDK_THREADS_ENTER();
      gtk_grab_remove (client_grab_widget);
      GDK_THREADS_LEAVE();
    }

  g_signal_emit (client, client_signals[DIE], 0);
}


static void
client_save_complete_callback (SmcConn smc_conn, SmPointer client_data)
{
  GnomeClient *client = (GnomeClient*) client_data;

  if (client_grab_widget)
    {
      GDK_THREADS_ENTER();
      gtk_grab_remove (client_grab_widget);
      GDK_THREADS_LEAVE();
    }

  g_signal_emit (client, client_signals[SAVE_COMPLETE], 0);
}


static void
client_shutdown_cancelled_callback (SmcConn smc_conn, SmPointer client_data)
{
  GnomeClient *client= (GnomeClient*) client_data;

  if (client_grab_widget)
    {
      GDK_THREADS_ENTER();
      gtk_grab_remove (client_grab_widget);
      GDK_THREADS_LEAVE();
    }

  g_signal_emit (client, client_signals[SHUTDOWN_CANCELLED], 0);
}


static void
client_interact_callback (SmcConn smc_conn, SmPointer client_data)
{
  GnomeClient *client= (GnomeClient *) client_data;

  if (client->interaction_keys)
    {
      GSList         *tmp= client->interaction_keys;
      InteractionKey *key= (InteractionKey *) tmp->data;

      client->interaction_keys= g_slist_remove (tmp, tmp->data);

      interaction_key_use (key);
    }
  else
    {
      /* This branch should never be executed.  But if it is executed,
         we just finish interacting.  */
      SmcInteractDone ((SmcConn) client->smc_conn, FALSE);
    }
}


#endif /* HAVE_LIBSM */


/*****************************************************************************/
/* Managing the master client */

#if 0
/* The following environment variables will be set on the master
   client, if they are defined the programs environment.  The array
   must end with a NULL entry.
   For now we have no entries.  You might think that saving DISPLAY,
   or HOME, or something like that would be right.  It isn't.  We
   definitely want to inherit these values from the user's (possibly
   changing) environment.  */
static char* master_environment[]=
{
  NULL
};
#endif

/********* gnome_client module */

/* Forward declaration for our module functions.  */
static void client_parse_func (poptContext ctx,
			       enum poptCallbackReason reason,
			       const struct poptOption *opt,
			       const char *arg, void *data);

static gboolean gnome_client_goption_sm_config_prefix (const gchar *option_name,
						       const gchar *value,
						       gpointer data,
						       GError **error);

static gboolean gnome_client_goption_sm_disable (const gchar *option_name,
						 const gchar *value,
						 gpointer data,
						 GError **error);

static void gnome_client_pre_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info);
static void gnome_client_post_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info);

enum { ARG_SM_CLIENT_ID=1, ARG_SM_CONFIG_PREFIX, ARG_SM_DISABLE };

typedef struct {
	guint sm_connect_id;
} GnomeProgramClass_gnome_client;

typedef struct {
        gboolean sm_connect;
} GnomeProgramPrivate_gnome_client;

static gchar *gnome_client_goption_sm_client_id = NULL;

static GQuark quark_gnome_program_private_gnome_client = 0;
static GQuark quark_gnome_program_class_gnome_client = 0;

static void
gnome_client_module_get_property (GObject *object, guint param_id, GValue *value,
			   GParamSpec *pspec)
{
        GnomeProgramClass_gnome_client *cdata;
        GnomeProgramPrivate_gnome_client *priv;
        GnomeProgram *program;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GNOME_IS_PROGRAM (object));

        program = GNOME_PROGRAM (object);

        cdata = g_type_get_qdata (G_OBJECT_TYPE (program),
				  quark_gnome_program_class_gnome_client);
        priv = g_object_get_qdata (G_OBJECT (program),
				   quark_gnome_program_private_gnome_client);

	if (param_id == cdata->sm_connect_id)
		g_value_set_boolean (value, priv->sm_connect);
	else
        	G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
}

static void
gnome_client_module_set_property (GObject *object, guint param_id,
			   const GValue *value, GParamSpec *pspec)
{
        GnomeProgramClass_gnome_client *cdata;
        GnomeProgramPrivate_gnome_client *priv;
        GnomeProgram *program;

        g_return_if_fail (object != NULL);
        g_return_if_fail (GNOME_IS_PROGRAM (object));

        program = GNOME_PROGRAM (object);

        cdata = g_type_get_qdata (G_OBJECT_TYPE (program),
				  quark_gnome_program_class_gnome_client);
        priv = g_object_get_qdata (G_OBJECT (program),
				   quark_gnome_program_private_gnome_client);

	if (param_id == cdata->sm_connect_id) {
		priv->sm_connect = g_value_get_boolean (value);
	} else {
		G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, pspec);
	}
}

static void
gnome_client_module_init_pass (const GnomeModuleInfo *mod_info)
{
    if (!quark_gnome_program_private_gnome_client)
	quark_gnome_program_private_gnome_client = g_quark_from_static_string
	    ("gnome-program-private:gnome_client");

    if (!quark_gnome_program_class_gnome_client)
	quark_gnome_program_class_gnome_client = g_quark_from_static_string
	    ("gnome-program-class:gnome_client");
}

static void
gnome_client_module_class_init (GnomeProgramClass *klass, const GnomeModuleInfo *mod_info)
{
        GnomeProgramClass_gnome_client *cdata = g_new0 (GnomeProgramClass_gnome_client, 1);

        g_type_set_qdata (G_OBJECT_CLASS_TYPE (klass), quark_gnome_program_class_gnome_client, cdata);

        cdata->sm_connect_id = gnome_program_install_property (
                klass,
                gnome_client_module_get_property,
                gnome_client_module_set_property,
                g_param_spec_boolean (GNOME_CLIENT_PARAM_SM_CONNECT, NULL, NULL,
                                      sm_connect_default,
				      (G_PARAM_READABLE | G_PARAM_WRITABLE |
				       G_PARAM_CONSTRUCT)));
}

static void
gnome_client_module_instance_init (GnomeProgram *program, GnomeModuleInfo *mod_info)
{
    GnomeProgramPrivate_gnome_client *priv = g_new0 (GnomeProgramPrivate_gnome_client, 1);

    g_object_set_qdata_full (G_OBJECT (program), quark_gnome_program_private_gnome_client, priv, g_free);
}

static GOptionGroup *
gnome_client_module_get_goption_group (void)
{
	const GOptionEntry session_goptions[] = {
		{ "sm-client-id", '\0', 0, G_OPTION_ARG_STRING,
		  &gnome_client_goption_sm_client_id,
		  N_("Specify session management ID"), N_("ID")},
		{ "sm-config-prefix", '\0', 0, G_OPTION_ARG_CALLBACK,
		  gnome_client_goption_sm_config_prefix,
		  N_("Specify prefix of saved configuration"), N_("PREFIX")},
		{ "sm-disable", '\0', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK,
		  gnome_client_goption_sm_disable,
		  N_("Disable connection to session manager"), NULL},
		{ NULL }
	};
	GOptionGroup *option_group;

	_gnome_ui_gettext_init (TRUE);

	option_group = g_option_group_new ("gnome-session",
					   N_("Session management:"),
					   N_("Show session management options"),
					   NULL, NULL);
	g_option_group_set_translation_domain (option_group, GETTEXT_PACKAGE);
	g_option_group_add_entries(option_group, session_goptions);

	return option_group;
}

const GnomeModuleInfo *
gnome_client_module_info_get (void)
{
	/* Command-line arguments understood by this module.  */
	static const struct poptOption options[] = {
		{ NULL, '\0', POPT_ARG_INTL_DOMAIN, GETTEXT_PACKAGE, 0, NULL, NULL },

		{ NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_PRE | POPT_CBFLAG_POST,
		  client_parse_func, 0, NULL, NULL },

		{ "sm-client-id", '\0', POPT_ARG_STRING, NULL, ARG_SM_CLIENT_ID,
		  N_("Specify session management ID"), N_("ID") },

		{ "sm-config-prefix", '\0', POPT_ARG_STRING, NULL, ARG_SM_CONFIG_PREFIX,
		  N_("Specify prefix of saved configuration"), N_("PREFIX") },

		{ "sm-disable", '\0', POPT_ARG_NONE, NULL, ARG_SM_DISABLE,
		  N_("Disable connection to session manager"), NULL },

		{ NULL, '\0', 0, NULL, 0 }
	};

	static GnomeModuleInfo module_info = {
		"gnome-client", VERSION, N_("Session management"),
		NULL, gnome_client_module_instance_init,
		gnome_client_pre_args_parse, gnome_client_post_args_parse,
		(struct poptOption *)options,
		gnome_client_module_init_pass, gnome_client_module_class_init,
		NULL, NULL
	};

	module_info.get_goption_group_func = gnome_client_module_get_goption_group;

	if (module_info.requirements == NULL) {
		static GnomeModuleRequirement req[3];

		_gnome_ui_gettext_init (FALSE);

		req[0].required_version = "1.3.7";
		req[0].module_info = gnome_gtk_module_info_get ();

		req[1].required_version = "1.102.0";
		req[1].module_info = libgnome_module_info_get ();

		req[2].required_version = NULL;
		req[2].module_info = NULL;

		module_info.requirements = req;
	}

	return &module_info;
}

/* Parse command-line arguments we recognize.  */
static void
client_parse_func (poptContext ctx,
		   enum poptCallbackReason reason,
		   const struct poptOption *opt,
		   const char *arg, void *data)
{
  switch (reason) {
  case POPT_CALLBACK_REASON_OPTION:
    switch (opt->val) {
    case ARG_SM_CLIENT_ID:
      gnome_client_set_id (master_client, arg);
      break;
    case ARG_SM_DISABLE:
      g_object_set (G_OBJECT (gnome_program_get()),
		    GNOME_CLIENT_PARAM_SM_CONNECT, FALSE, NULL);
      break;
    case ARG_SM_CONFIG_PREFIX:
      g_free(master_client->config_prefix);
      master_client->config_prefix= g_strdup (arg);
      master_client_restored = TRUE;
      break;
    }
    break;
  default:
    break;
  }
}

static gboolean
gnome_client_goption_sm_disable (const gchar *option_name,
				 const gchar *value,
				 gpointer data,
				 GError **error)
{
	g_object_set (G_OBJECT (gnome_program_get()),
		      GNOME_CLIENT_PARAM_SM_CONNECT, FALSE, NULL);

	return TRUE;
}

static gboolean
gnome_client_goption_sm_config_prefix (const gchar *option_name,
				       const gchar *value,
				       gpointer data,
				       GError **error)
{
	g_free (master_client->config_prefix);
	master_client->config_prefix = g_strdup (value);
	master_client_restored = TRUE;
 
	return TRUE;
}

static void
gnome_client_pre_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info)
{
#if 0
  int i;
#endif
  char *cwd;

  /* Make sure the Gtk+ type system is initialized.  */
  g_type_init ();
  gnome_type_init ();

  /* Create the master client.  */
  master_client = gnome_client_new_without_connection ();
  /* Connect the master client's default signals.  */
  g_signal_connect (master_client, "connect",
		    G_CALLBACK (master_client_connect), NULL);
  g_signal_connect (master_client, "disconnect",
		    G_CALLBACK (master_client_disconnect), NULL);

  /* Initialise ICE */
  gnome_ice_init ();

#if 0
  /* Set the master client's environment.  */
  for (i= 0; master_environment[i]; i++)
    {
      const char *value= g_getenv (master_environment[i]);

      if (value)
	gnome_client_set_environment (master_client,
				      master_environment[i],
				      value);
    }
#endif

  cwd = g_get_current_dir();
  if (cwd != NULL)
    {
      gnome_client_set_current_directory (master_client, cwd);
      g_free (cwd);
    }

  /* needed on non-glibc systems: */
  gnome_client_set_program (master_client, g_get_prgname ());
  /* Argument parsing is starting.  We set the restart and the
     clone command to a default value, so other functions can use
     the master client while parsing the command line.  */
  gnome_client_set_restart_command (master_client, 1,
				    &master_client->program);
}

static void
gnome_client_post_args_parse(GnomeProgram *app, GnomeModuleInfo *mod_info)
{
  gboolean do_connect = TRUE;

  g_object_get (G_OBJECT (app), GNOME_CLIENT_PARAM_SM_CONNECT, &do_connect, NULL);

  if (gnome_client_goption_sm_client_id != NULL)
    {
      gnome_client_set_id (master_client, gnome_client_goption_sm_client_id);
      g_free (gnome_client_goption_sm_client_id);
      gnome_client_goption_sm_client_id = NULL;
    }
  else
    {
      const gchar *desktop_autostart_id;

      desktop_autostart_id = g_getenv ("DESKTOP_AUTOSTART_ID");

      if (desktop_autostart_id != NULL)
        gnome_client_set_id (master_client, desktop_autostart_id);
    }

  /* Unset DESKTOP_AUTOSTART_ID in order to avoid child processes to
   * use the same client id. */
  g_unsetenv ("DESKTOP_AUTOSTART_ID");

  /* We're done, so we can connect to the session manager now.  */
  if (do_connect)
    gnome_client_connect (master_client);
}

#ifndef GNOME_DISABLE_DEPRECATED_SOURCE
/**
 * gnome_client_disable_master_connection
 *
 * Description: Don't connect the master client to the session manager. Usually
 * invoked by users when they pass the --sm-disable argument to a Gnome application.
 **/

void
gnome_client_disable_master_connection (void)
{
	if (gnome_program_get () == NULL) {
		sm_connect_default = FALSE;
	} else {
		g_object_set (G_OBJECT (gnome_program_get()),
			      GNOME_CLIENT_PARAM_SM_CONNECT, FALSE, NULL);
	}
}
#endif /* GNOME_DISABLE_DEPRECATED_SOURCE */

static void
master_client_connect (GnomeClient *client,
		       gboolean     restarted,
		       gpointer     client_data)
{
  gdk_set_sm_client_id (gnome_client_get_id (client));
}

static void
master_client_disconnect (GnomeClient *client,
			  gpointer client_data)
{
  if(client_grab_widget && gtk_grab_get_current() == client_grab_widget)
    gtk_grab_remove(client_grab_widget);

  gdk_set_sm_client_id (NULL);
}

/**
 * gnome_master_client
 *
 * Description:
 * Get the master session management client.  This master client gets a client
 * id, that may be specified by the '--sm-client-id' command line option.  A
 * master client will be generated by gnome_program_init().  If possible the
 * master client will contact the session manager after command-line parsing is
 * finished (unless gnome_client_disable_master_connection() was called). The
 * master client will also set the SM_CLIENT_ID property on the client leader
 * window of your application.
 *
 * Additionally, the master client gets some static arguments set automatically
 * (see gnome_client_add_static_arg() for static arguments):
 * gnome_program_init() passes all the command line options which are
 * recognised by gtk as static arguments to the master client.
 *
 * Returns:  Pointer to the master client
 **/

GnomeClient*
gnome_master_client (void)
{
  return master_client;
}


/*****************************************************************************/
/* GTK-class managing functions */

GNOME_CLASS_BOILERPLATE (GnomeClient, gnome_client,
			 GtkObject, GTK_TYPE_OBJECT)

static void
gnome_client_class_init (GnomeClientClass *klass)
{
  GtkObjectClass *object_class = (GtkObjectClass*) klass;
  GObjectClass *gobject_class = (GObjectClass*) klass;

  client_signals[SAVE_YOURSELF] =
    g_signal_new ("save_yourself",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GnomeClientClass, save_yourself),
		  NULL, NULL,
		  _gnome_marshal_BOOLEAN__INT_ENUM_BOOLEAN_ENUM_BOOLEAN,
		  G_TYPE_BOOLEAN, 5,
		  G_TYPE_INT,
		  GNOME_TYPE_SAVE_STYLE,
		  G_TYPE_BOOLEAN,
		  GNOME_TYPE_INTERACT_STYLE,
		  G_TYPE_BOOLEAN);
  client_signals[DIE] =
    g_signal_new ("die",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GnomeClientClass, die),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  client_signals[SAVE_COMPLETE] =
    g_signal_new ("save_complete",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GnomeClientClass, save_complete),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  client_signals[SHUTDOWN_CANCELLED] =
    g_signal_new ("shutdown_cancelled",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GnomeClientClass, shutdown_cancelled),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);
  client_signals[CONNECT] =
    g_signal_new ("connect",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GnomeClientClass, connect),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__BOOLEAN,
		  G_TYPE_NONE, 1,
		  G_TYPE_BOOLEAN);
  client_signals[DISCONNECT] =
    g_signal_new ("disconnect",
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (GnomeClientClass, disconnect),
		  NULL, NULL,
		  g_cclosure_marshal_VOID__VOID,
		  G_TYPE_NONE, 0);

  object_class->destroy = gnome_real_client_destroy;
  gobject_class->finalize = gnome_real_client_finalize;

  klass->save_yourself      = NULL;
  klass->die                = gnome_client_disconnect;
  klass->save_complete      = gnome_real_client_save_complete;
  klass->shutdown_cancelled = gnome_real_client_shutdown_cancelled;
  klass->connect            = gnome_real_client_connect;
  klass->disconnect         = gnome_real_client_disconnect;
}

static void
gnome_client_instance_init (GnomeClient *client)
{
  client->smc_conn          = NULL;
  client->client_id         = NULL;
  client->previous_id       = NULL;
  client->input_id          = 0;
  client->config_prefix       = NULL;
  client->global_config_prefix= NULL;

  client->static_args       = NULL;

  /* Preset some default values.  */
  client->clone_command     = NULL;
  client->current_directory = NULL;
  client->discard_command   = NULL;
  client->environment       = g_hash_table_new (g_str_hash, g_str_equal);

  client->process_id        = getpid ();

  client->program           = NULL;
  client->resign_command    = NULL;
  client->restart_command   = NULL;

  client->restart_style     = -1;

  client->shutdown_command  = NULL;

  client->user_id= g_strdup (g_get_user_name ());

  client->state                       = GNOME_CLIENT_DISCONNECTED;
  client->interaction_keys            = NULL;
}

static gboolean
environment_entry_remove (gchar *key, gchar *value, gpointer data)
{
  g_free (key);
  g_free (value);

  return TRUE;
}

static void
gnome_real_client_destroy (GtkObject *object)
{
  GnomeClient *client;

  /* remember, destroy can be run multiple times! */

  g_return_if_fail (object != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (object));

  client = GNOME_CLIENT (object);

  if (GNOME_CLIENT_CONNECTED (client))
	  gnome_client_disconnect (client);

  GNOME_CALL_PARENT (GTK_OBJECT_CLASS, destroy, (object));
}

static void
gnome_real_client_finalize (GObject *object)
{
  GnomeClient *client;

  g_return_if_fail (object != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (object));

  client = GNOME_CLIENT (object);

  g_free (client->client_id);
  client->client_id = NULL;
  g_free (client->previous_id);
  client->previous_id = NULL;
  g_free (client->config_prefix);
  client->config_prefix = NULL;
  g_free (client->global_config_prefix);
  client->global_config_prefix = NULL;

  if (client->static_args != NULL) {
	  g_list_foreach (client->static_args, (GFunc)g_free, NULL);
	  g_list_free    (client->static_args);
	  client->static_args = NULL;
  }

  g_strfreev (client->clone_command);
  client->clone_command = NULL;
  g_free     (client->current_directory);
  client->current_directory = NULL;
  g_strfreev (client->discard_command);
  client->discard_command = NULL;

  if(client->environment != NULL) {
	  g_hash_table_foreach_remove (client->environment,
				       (GHRFunc)environment_entry_remove, NULL);
	  g_hash_table_destroy        (client->environment);
	  client->environment = NULL;
  }

  g_free     (client->program);
  client->program = NULL;
  g_strfreev (client->resign_command);
  client->resign_command = NULL;
  g_strfreev (client->restart_command);
  client->restart_command = NULL;
  g_strfreev (client->shutdown_command);
  client->shutdown_command = NULL;
  g_free     (client->user_id);
  client->user_id = NULL;

  GNOME_CALL_PARENT (G_OBJECT_CLASS, finalize, (object));
}

/*****************************************************************************/
/* 'gnome_client' public member functions */


/**
 * gnome_client_new
 *
 * Description: Allocates memory for a new GNOME session management client
 * object. After allocating, the client tries to connect to a session manager.
 * You probably want to use gnome_master_client() instead.
 *
 * Returns: Pointer to a newly allocated GNOME session management client object.
 **/

GnomeClient *
gnome_client_new (void)
{
  GnomeClient *client;

  client= gnome_client_new_without_connection ();

  gnome_client_connect (client);

  return client;
}


/**
 * gnome_client_new_without_connection
 *
 * Description: Allocates memory for a new GNOME session management client
 * object. You probably want to use gnome_master_client() instead.
 *
 * Returns: Pointer to a newly allocated GNOME session management client object.
 **/

GnomeClient *
gnome_client_new_without_connection (void)
{
  GnomeClient *client;

  client = g_object_new (GNOME_TYPE_CLIENT, NULL);

  /* Preset the CloneCommand, RestartCommand and Program properties.
     FIXME: having a default would be cool, but it is probably hard to
     get this to interact correctly with the distributed command-line
     parsing.  */
  client->clone_command   = NULL;
  client->restart_command = NULL;

  /* Non-glibc systems do not get this set on the master_client until
     client_parse_func but this is not a problem.
     The SM specs require explictly require that this is the value: */
  client->program = g_strdup (g_get_prgname ());

  return client;
}


/**
 * gnome_client_flush
 * @client: A #GnomeClient instance.
 *
 * Description:
 * This will force the underlying connection to the session manager to be
 * flushed. This is useful if you have some pending changes that you want to
 * make sure get committed.
 **/

void
gnome_client_flush (GnomeClient *client)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

#ifdef HAVE_LIBSM
  if (GNOME_CLIENT_CONNECTED (client)) {
    IceConn conn = SmcGetIceConnection ((SmcConn) client->smc_conn);
    IceFlush (conn);
  }
#endif
}

/*****************************************************************************/

#define ERROR_STRING_LENGTH 256


/**
 * gnome_client_connect
 * @client: A #GnomeClient instance.
 *
 * Description: Causes the client to connect to the session manager.
 * Usually happens automatically; no need to call this function.
 **/

void
gnome_client_connect (GnomeClient *client)
{
#ifdef HAVE_LIBSM
  SmcCallbacks      callbacks;
  gchar            *client_id;
#endif /* HAVE_LIBSM */

  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

#ifdef HAVE_LIBSM
  if (GNOME_CLIENT_CONNECTED (client))
    return;

  callbacks.save_yourself.callback      = client_save_yourself_callback;
  callbacks.die.callback                = client_die_callback;
  callbacks.save_complete.callback      = client_save_complete_callback;
  callbacks.shutdown_cancelled.callback = client_shutdown_cancelled_callback;

  callbacks.save_yourself.client_data =
    callbacks.die.client_data =
    callbacks.save_complete.client_data =
    callbacks.shutdown_cancelled.client_data = (SmPointer) client;

  if (g_getenv ("SESSION_MANAGER"))
    {
      gchar error_string_ret[ERROR_STRING_LENGTH] = "";

      client->smc_conn= (gpointer)
	SmcOpenConnection (NULL, client,
			   SmProtoMajor, SmProtoMinor,
			   SmcSaveYourselfProcMask | SmcDieProcMask |
			   SmcSaveCompleteProcMask |
			   SmcShutdownCancelledProcMask,
			   &callbacks,
			   client->client_id, &client_id,
			   ERROR_STRING_LENGTH, error_string_ret);

      if (error_string_ret[0])
	g_warning ("While connecting to session manager:\n%s.",
		   error_string_ret);
    }

  if (GNOME_CLIENT_CONNECTED (client))
    {
      gint    restarted= FALSE;

      g_free (client->previous_id);
      client->previous_id= client->client_id;
      client->client_id= client_id;

      restarted= (client->previous_id &&
		  !strcmp (client->previous_id, client_id));

      client_set_state (client, (restarted ?
				 GNOME_CLIENT_IDLE :
				 GNOME_CLIENT_REGISTERING));

      /* Let all the world know, that we have a connection to a
         session manager.  */
      g_signal_emit (client, client_signals[CONNECT], 0, restarted);
    }
#endif /* HAVE_LIBSM */
}



/**
 * gnome_client_disconnect
 * @client: A #GnomeClient instance.
 *
 * Description: Disconnect the client from the session manager.
 **/

void
gnome_client_disconnect (GnomeClient *client)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  if (GNOME_CLIENT_CONNECTED (client))
    {
      gnome_client_flush (client);
      g_signal_emit (client, client_signals[DISCONNECT], 0);
    }
}

/*****************************************************************************/

/**
 * gnome_client_get_flags:
 * @client: Pointer to GNOME session client object.
 *
 * Description: Determine the client's status with the session manager.,
 *
 * Returns: Various #GnomeClientFlag flags which have been or'd together.
 **/

GnomeClientFlags
gnome_client_get_flags (GnomeClient *client)
{
  GnomeClientFlags flags= 0;

  g_return_val_if_fail (client != NULL, 0);
  g_return_val_if_fail (GNOME_IS_CLIENT (client), 0);

  /* To not break binary compability with existing code, the
     GnomeClient struct has not been changed.  So the flags have to be
     calculated every time.  */

  if (GNOME_CLIENT_CONNECTED (client))
    {
      flags |= GNOME_CLIENT_IS_CONNECTED;

      if (client->previous_id &&
	  !strcmp (client->previous_id, client->client_id))
	flags |= GNOME_CLIENT_RESTARTED;

      if (master_client_restored && (client == master_client))
	flags |= GNOME_CLIENT_RESTORED;
    }

  return flags;
}


/*****************************************************************************/

/**
 * gnome_client_set_clone_command
 * @client: Pointer to GNOME session client object.
 * @argc: Number of strings in the @argv vector.
 * @argv: Argument strings, suitable for use with execv().
 *
 * Description: Set a command the session manager can use to create a new
 * instance of the application.
 **/

void
gnome_client_set_clone_command (GnomeClient *client,
				gint argc, gchar *argv[])
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  /* Whenever clone_command == NULL then restart_command is used instead */
  g_strfreev (client->clone_command);
  client->clone_command = array_init_from_arg (argc, argv);

#ifdef HAVE_LIBSM
  client_set_clone_command (client);
#endif /* HAVE_LIBSM */
}

/**
 * gnome_client_set_current_directory
 * @client: Pointer to GNOME session client object.
 * @dir: Directory path.
 *
 * Description: Set the directory to be in when running shutdown, discard,
 * restart, etc. commands.
 **/

void
gnome_client_set_current_directory (GnomeClient *client,
				    const gchar *dir)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  g_free (client->current_directory);

  if (dir)
    {
      client->current_directory= g_strdup (dir);
#ifdef HAVE_LIBSM
      client_set_string (client, SmCurrentDirectory,
				  client->current_directory);
#endif /* HAVE_LIBSM */
    }
  else
    {
      client->current_directory= NULL;
#ifdef HAVE_LIBSM
      client_unset (client, SmCurrentDirectory);
#endif /* HAVE_LIBSM */
    }
}


/* See doc/reference/tmpl/gnome-client.sgml for documentation. */

void
gnome_client_set_discard_command (GnomeClient *client,
				  gint argc, gchar *argv[])
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  if (argv == NULL)
    {
      g_return_if_fail (argc == 0);

      g_strfreev (client->discard_command);
      client->discard_command= NULL;
#ifdef HAVE_LIBSM
      client_unset (client, SmDiscardCommand);
#endif /* HAVE_LIBSM */
    }
  else
    {
      g_strfreev (client->discard_command);
      client->discard_command = array_init_from_arg (argc, argv);

#ifdef HAVE_LIBSM
      client_set_array (client, SmDiscardCommand,
				  client->discard_command);
#endif /* HAVE_LIBSM */
    }
}



/**
 * gnome_client_set_environment
 * @client: Pointer to GNOME session client object.
 * @name: Name of the environment variable
 * @value: Value of the environment variable
 *
 * Description: Set an environment variable to be placed in the
 * client's environment prior to running restart, shutdown, discard, etc. commands.
 **/

void
gnome_client_set_environment (GnomeClient *client,
			      const gchar *name,
			      const gchar *value)
{
  gchar *old_name;
  gchar *old_value;

  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  g_return_if_fail (name != NULL);

  if (g_hash_table_lookup_extended (client->environment,
				    name,
				    (gpointer *) &old_name,
				    (gpointer *) &old_value))
    {
      if (value)
	{
	  g_hash_table_insert (client->environment, old_name,
			       g_strdup (value));
	  g_free (old_value);
	}
      else
	{
	  g_hash_table_remove (client->environment, name);
	  g_free (old_name);
	  g_free (old_value);
	}
    }
  else if (value)
    {
      g_hash_table_insert (client->environment,
			   g_strdup (name),
			   g_strdup (value));
    }

#ifdef HAVE_LIBSM
  client_set_ghash (client, SmEnvironment, client->environment);
#endif /* HAVE_LIBSM */
}



/**
 * gnome_client_set_process_id
 * @client: Pointer to GNOME session client object.
 * @pid: PID to set as the client's PID.
 *
 * Description: The client should tell the session manager the result of
 * getpid(). However, GNOME does this automatically; so you do not need this
 * function.
 **/

void
gnome_client_set_process_id (GnomeClient *client, pid_t pid)
{
  gchar str_pid[32];

  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  client->process_id= pid;

  g_snprintf (str_pid, sizeof(str_pid), "%d", client->process_id);

#ifdef HAVE_LIBSM
  client_set_string (client, SmProcessID, str_pid);
#endif /* HAVE_LIBSM */
}



/**
 * gnome_client_set_program
 * @client: Pointer to GNOME session client object.
 * @program: Name of the program.
 *
 * Description: Used to tell the session manager the name of your program. Set
 * automatically; this function isn't needed.
 **/

void
gnome_client_set_program (GnomeClient *client,
			  const gchar *program)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  /* The Program property is required, so you must not unset it.  */
  g_return_if_fail (program != NULL);

  g_free (client->program);

  client->program= g_strdup (program);

  client_unset_config_prefix (client);

#ifdef HAVE_LIBSM
  client_set_string (client, SmProgram, client->program);
#endif /* HAVE_LIBSM */
}



/**
 * gnome_client_set_resign_command
 * @client: Pointer to GNOME session client object.
 * @argc: Number of strings in @argv.
 * @argv: execv()-style command to undo the effects of the client.
 *
 * Description: Some clients can be "undone," removing their effects and
 * deleting any saved state. For example, xmodmap could register a resign
 * command to undo the keymap changes it saved.
 *
 * Used by clients that use the #GNOME_RESTART_ANYWAY restart style to to undo
 * their effects (these clients usually perform initialisation functions and
 * leave effects behind after they die). The resign command combines the
 * effects of a shutdown command and a discard command. It is executed when the
 * user decides that the client should cease to be restarted.
 **/

void
gnome_client_set_resign_command (GnomeClient *client,
				 gint argc, gchar *argv[])
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  if (argv == NULL)
    {
      g_return_if_fail (argc == 0);

      g_strfreev (client->resign_command);
      client->resign_command= NULL;

#ifdef HAVE_LIBSM
      client_unset (client, SmResignCommand);
#endif /* HAVE_LIBSM */
    }
  else
    {
      g_strfreev (client->resign_command);
      client->resign_command = array_init_from_arg (argc, argv);

#ifdef HAVE_LIBSM
      client_set_array (client, SmResignCommand, client->resign_command);
#endif /* HAVE_LIBSM */
    }
}



/**
 * gnome_client_set_restart_command
 * @client: Pointer to GNOME session client object.
 * @argc: Number of strings in argv.
 * @argv: Argument vector to an execv() to restart the client.
 *
 * Description: When clients crash or the user logs out and back in, they are
 * restarted. This command should perform the restart.  Executing the restart
 * command on the local host should reproduce the state of the client at the
 * time of the session save as closely as possible. Saving config info under
 * the gnome_client_get_config_prefix() is generally useful.
 **/

void
gnome_client_set_restart_command (GnomeClient *client,
				  gint argc, gchar *argv[])
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  /* The RestartCommand property is required, so you must not unset
     it.  */
  g_return_if_fail (argc != 0);
  g_return_if_fail (argv != NULL);

  g_strfreev (client->restart_command);
  client->restart_command = array_init_from_arg (argc, argv);

#ifdef HAVE_LIBSM
  client_set_restart_command (client);
#endif /* HAVE_LIBSM */
}

/**
 * gnome_client_set_priority
 * @client: Pointer to GNOME session client object.
 * @priority: Position of client in session start up ordering.
 *
 * Description:
 *
 * The gnome-session manager restarts clients in order of their
 * priorities in a similar way to the start up ordering in SysV.
 * This function allows the app to suggest a position in this
 * ordering. The value should be between 0 and 99. A default
 * value of 50 is assigned to apps that do not provide a value.
 * The user may assign a different priority.
 **/

void
gnome_client_set_priority (GnomeClient *client, guint priority)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

#ifdef HAVE_LIBSM
  if (priority > 99)
    priority = 99;

  client_set_gchar (client, "_GSM_Priority", (gchar) priority);
#endif
}

/**
 * gnome_client_set_restart_style
 * @client: Pointer to GNOME session client object.
 * @style: When to restart the client.
 *
 * Tells the session manager how the client should be restarted in future
 * session. The options are given by the #GnomeRestartStyle enum.
 **/

void
gnome_client_set_restart_style (GnomeClient *client,
				GnomeRestartStyle style)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  switch (style)
    {
    case GNOME_RESTART_IF_RUNNING:
#ifdef HAVE_LIBSM
      client_set_gchar (client, SmRestartStyleHint, SmRestartIfRunning);
      break;
#endif /* HAVE_LIBSM */

    case GNOME_RESTART_ANYWAY:
#ifdef HAVE_LIBSM
      client_set_gchar (client, SmRestartStyleHint, SmRestartAnyway);
      break;
#endif /* HAVE_LIBSM */

    case GNOME_RESTART_IMMEDIATELY:
#ifdef HAVE_LIBSM
      client_set_gchar (client, SmRestartStyleHint, SmRestartImmediately);
      break;
#endif /* HAVE_LIBSM */

    case GNOME_RESTART_NEVER:
#ifdef HAVE_LIBSM
      client_set_gchar (client, SmRestartStyleHint, SmRestartNever);
#endif /* HAVE_LIBSM */
      break;

    default:
      g_assert_not_reached ();
      return;
    }

  client->restart_style= style;
}



/**
 * gnome_client_set_shutdown_command
 * @client: Pointer to GNOME session client object.
 * @argc: Number of strings in argv.
 * @argv: Command to shutdown the client if the client isn't running.
 *
 * Description: GNOME_RESTART_ANYWAY clients can set this command to run
 * when the user logs out but the client is no longer running.
 *
 * Used by clients that use the GNOME_RESTART_ANYWAY restart style to
 * to undo their effects (these clients usually perform initialisation
 * functions and leave effects behind after they die).  The shutdown
 * command simply undoes the effects of the client. It is executed
 * during a normal logout.
 **/

void
gnome_client_set_shutdown_command (GnomeClient *client,
				   gint argc, gchar *argv[])
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  if (argv == NULL)
    {
      g_return_if_fail (argc == 0);

      g_strfreev (client->shutdown_command);
      client->shutdown_command = NULL;
#ifdef HAVE_LIBSM
      client_unset (client, SmShutdownCommand);
#endif /* HAVE_LIBSM */
    }
  else
    {
      g_strfreev (client->shutdown_command);
      client->shutdown_command = array_init_from_arg (argc, argv);

#ifdef HAVE_LIBSM
      client_set_array (client, SmShutdownCommand,
				  client->shutdown_command);
#endif /* HAVE_LIBSM */
    }
}



/**
 * gnome_client_set_user_id
 * @client: Pointer to GNOME session client object.
 * @id: Username.
 *
 * Description: Tell the session manager the user's login name. GNOME
 * does this automatically; no need to call the function.
 **/

void
gnome_client_set_user_id (GnomeClient *client,
			  const gchar       *id)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  /* The UserID property is required, so you must not unset it.  */
  g_return_if_fail (id != NULL);

  g_free (client->user_id);

  client->user_id= g_strdup (id);
#ifdef HAVE_LIBSM
  client_set_string (client, SmUserID, client->user_id);
#endif /* HAVE_LIBSM */
}



/**
 * gnome_client_add_static_arg
 * @client: Pointer to GNOME session client object.
 * @...: %NULL-terminated list of arguments to add to the restart command.
 *
 * Description: You can add arguments to your restart command's argv with this
 * function. This function provides an alternative way of adding new arguments
 * to the restart command. The arguments are placed before the arguments
 * specified by gnome_client_set_restart_command() and after the arguments
 * recognised by GTK+ that are specified by the user on the original command
 * line.
 **/

void
gnome_client_add_static_arg (GnomeClient *client, ...)
{
  va_list  args;
  gchar   *str;

  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  va_start (args, client);
  str= va_arg (args, gchar*);
  while (str)
    {
      client->static_args= g_list_append (client->static_args, g_strdup(str));
      str= va_arg (args, gchar*);
    }
  va_end (args);
}

/*****************************************************************************/


/**
 * gnome_client_set_id
 * @client: A #GnomeClient instance.
 * @id: Session management ID.
 *
 * Description: Set the client's session management ID; must be done
 * before connecting to the session manager. There is usually no reason to call
 * this function.
 **/

void
gnome_client_set_id (GnomeClient *client, const gchar *id)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  g_return_if_fail (!GNOME_CLIENT_CONNECTED (client));

  g_return_if_fail (id != NULL);

  g_free (client->client_id);
  client->client_id= g_strdup (id);
}



/**
 * gnome_client_get_id
 * @client: A #GnomeClient instance.
 *
 * Description: Returns session management ID
 *
 * Returns:  Session management ID for this client; %NULL if not connected to a
 * session manager.
 **/

const gchar *
gnome_client_get_id (GnomeClient *client)
{
  g_return_val_if_fail (client != NULL, NULL);
  g_return_val_if_fail (GNOME_IS_CLIENT (client), NULL);

  return client->client_id;
}

/**
 * gnome_client_get_previous_id
 * @client: A #GnomeClient instance.
 *
 * Description: Get the session management ID from the previous session.
 *
 * Returns: Pointer to the session management ID the client had in the last
 * session, or %NULL if it was not in a previous session.
 **/

const gchar *
gnome_client_get_previous_id (GnomeClient *client)
{
  g_return_val_if_fail (client != NULL, NULL);
  g_return_val_if_fail (GNOME_IS_CLIENT (client), NULL);

  return client->previous_id;
}

/**
 * gnome_client_get_desktop_id:
 * @client: A #GnomeClient instance.
 *
 * Get the client ID of the desktop's current instance, i.e.  if
 * you consider the desktop as a whole as a session managed app, this
 * returns its session ID using a GNOME extension to session
 * management. May return %NULL for apps not running under a recent
 * version of gnome-session; apps should handle that case.
 *
 * Returns: Session ID of GNOME desktop instance, or %NULL if none.
 **/
const gchar*
gnome_client_get_desktop_id (GnomeClient *client)
{
  g_return_val_if_fail (GNOME_IS_CLIENT (client), NULL);

  return g_getenv ("GNOME_DESKTOP_SESSION_ID");
}

/**
 * gnome_client_get_config_prefix
 * @client: Pointer to GNOME session client object.
 *
 * Description: Get the config prefix for a client. This config prefix
 * provides a suitable place to store any details about the state of
 * the client which can not be described using the app's command line
 * arguments (as set in the restart command). You may push the
 * returned value using gnome_config_push_prefix() and read or write
 * any values you require.
 *
 * Returns: Config prefix. The returned string belongs to libgnomeui library
 * and should NOT be freed by the caller.
 **/

const gchar *
gnome_client_get_config_prefix (GnomeClient *client)
{
  g_return_val_if_fail (client == NULL || GNOME_IS_CLIENT (client), NULL);

  if (!client)
      client = master_client;

  if (!client)
      return gnome_client_get_global_config_prefix (client);

  if (!client->config_prefix)
    client->config_prefix = (char *)gnome_client_get_global_config_prefix (client);

  return client->config_prefix;
}

static gchar* config_prefix = NULL;

/**
 * gnome_client_set_global_config_prefix
 * @client: Pointer to GNOME session client object.
 * @prefix: Prefix for saving the global configuration.
 *
 * Description:  Set the value used for the global config prefix. The config
 * prefixes returned by gnome_client_get_config_prefix() are formed by
 * extending this prefix with an unique identifier.
 *
 * The global config prefix defaults to a name based on the name of
 * the executable. This function allows you to set it to a different
 * value. It should be called BEFORE retrieving the config prefix for
 * the first time. Later calls will be ignored.
 *
 * For example, setting a global config prefix of "/app.d/session/"
 * would ensure that all your session save files or directories would
 * be gathered together into the app.d directory.
 **/

void
gnome_client_set_global_config_prefix (GnomeClient *client, const gchar* prefix)
{
  if (client == NULL)
    {
      config_prefix= g_strdup (prefix);
      return;
    }

  g_return_if_fail (GNOME_IS_CLIENT (client));

  client->global_config_prefix = g_strdup (prefix);
}

/**
 * gnome_client_get_global_config_prefix
 * @client: Pointer to GNOME session client object.
 *
 * Description: Get the config prefix that will be returned by
 * gnome_client_get_config_prefix() for clients which have NOT been restarted
 * or cloned (i.e. for clients started by the user without `--sm-' options).
 * This config prefix may be used to write the user's preferred config for
 * these "new" clients.
 *
 * You could also use this prefix as a place to store and retrieve config
 * details that you wish to apply to ALL instances of the app. However, this
 * practice limits the users freedom to configure each instance in a different
 * way so it should be used with caution.
 *
 * Returns:  The config prefix as a newly allocated string.
 **/

const gchar *
gnome_client_get_global_config_prefix (GnomeClient *client)
{
  if (client == NULL)
    {
      if (!config_prefix) {
	char *prgname, *name;
	prgname = g_get_prgname ();

	g_assert (prgname != NULL);

	name= strrchr (prgname, '/');
	name= name ? (name+1) : prgname;

	config_prefix= g_strconcat ("/", name, "/", NULL);
      }

      return config_prefix;
    }

  g_return_val_if_fail (GNOME_IS_CLIENT (client), NULL);

  if (!client->global_config_prefix)
    {
      char *name;
      name= strrchr (client->program, '/');

      name= name ? (name+1) : client->program;

      client->global_config_prefix= g_strconcat ("/", name, "/", NULL);
    }

  return client->global_config_prefix;
}

static void
client_unset_config_prefix (GnomeClient *client)
{
  g_free (client->config_prefix);
  client->config_prefix= NULL;
  g_free (client->global_config_prefix);
  client->global_config_prefix= NULL;
}


/*****************************************************************************/

static void
gnome_real_client_save_complete (GnomeClient *client)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  client_set_state (client, GNOME_CLIENT_IDLE);
}

/* The following function is executed, after receiving a SHUTDOWN
 * CANCELLED message from the session manager.  It is executed before
 * the function connected to the 'shutdown_cancelled' signal are
 * called */

static void
gnome_real_client_shutdown_cancelled (GnomeClient *client)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

#ifdef HAVE_LIBSM
  if ((client->state == GNOME_CLIENT_SAVING_PHASE_1) ||
      (client->state == GNOME_CLIENT_WAITING_FOR_PHASE_2) ||
      (client->state == GNOME_CLIENT_SAVING_PHASE_2))
    SmcSaveYourselfDone ((SmcConn) client->smc_conn, False);

  client_set_state (client, GNOME_CLIENT_IDLE);

  /* Free all interation keys but the one in use.  */
  while (client->interaction_keys)
    {
      GSList *tmp= client->interaction_keys;

      interaction_key_destroy_if_possible ((InteractionKey *) tmp->data);

      client->interaction_keys= g_slist_remove (tmp, tmp->data);
    }
#endif /* HAVE_LIBSM */
}

static void
gnome_real_client_connect (GnomeClient *client,
			   gboolean     restarted)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

#ifdef HAVE_LIBSM
  /* We now set all non empty properties.  */
  client_set_string (client, SmCurrentDirectory, client->current_directory);
  client_set_array (client, SmDiscardCommand, client->discard_command);
  client_set_ghash (client, SmEnvironment, client->environment);
  {
    gchar str_pid[32];

    g_snprintf (str_pid, sizeof(str_pid), "%d", client->process_id);
    client_set_string (client, SmProcessID, str_pid);
  }
  client_set_string (client, SmProgram, client->program);
  client_set_array (client, SmResignCommand, client->resign_command);

  client_set_clone_command (client);
  client_set_restart_command (client);

  switch (client->restart_style)
    {
    case GNOME_RESTART_IF_RUNNING:
      client_set_gchar (client, SmRestartStyleHint, SmRestartIfRunning);
      break;

    case GNOME_RESTART_ANYWAY:
      client_set_gchar (client, SmRestartStyleHint, SmRestartAnyway);
      break;

    case GNOME_RESTART_IMMEDIATELY:
      client_set_gchar (client, SmRestartStyleHint, SmRestartImmediately);
      break;

    case GNOME_RESTART_NEVER:
      client_set_gchar (client, SmRestartStyleHint, SmRestartNever);
      break;

    default:
      break;
    }

  client_set_array (client, SmShutdownCommand, client->shutdown_command);
  client_set_string (client, SmUserID, client->user_id);
#endif /* HAVE_LIBSM */
}


static void
gnome_real_client_disconnect (GnomeClient *client)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

#ifdef HAVE_LIBSM
  if (GNOME_CLIENT_CONNECTED (client))
    {
      SmcCloseConnection ((SmcConn) client->smc_conn, 0, NULL);
      client->smc_conn = NULL;
    }

  client_set_state (client, GNOME_CLIENT_DISCONNECTED);

  /* Free all interation keys but the one in use.  */
  while (client->interaction_keys)
    {
      GSList *tmp= client->interaction_keys;

      interaction_key_destroy_if_possible ((InteractionKey *) tmp->data);

      client->interaction_keys= g_slist_remove (tmp, tmp->data);
    }

#endif /* HAVE_LIBSM */
}

/*****************************************************************************/

static void
gnome_client_request_interaction_internal (GnomeClient           *client,
					   GnomeDialogType        dialog_type,
					   gboolean               interp,
					   GnomeInteractFunction  function,
					   gpointer               data,
					   GDestroyNotify       destroy)
{
#ifdef HAVE_LIBSM
  Status          status;
  InteractionKey *key;
  int             _dialog_type;
#endif

  /* Convert GnomeDialogType into XSM library values */
  switch (dialog_type)
    {
    case GNOME_DIALOG_ERROR:
#ifdef HAVE_LIBSM
      _dialog_type= SmDialogError;
#endif
      break;

    case GNOME_DIALOG_NORMAL:
#ifdef HAVE_LIBSM
      _dialog_type= SmDialogError;
#endif
      break;

    default:
      g_assert_not_reached ();
      return;
    }

#ifdef HAVE_LIBSM

  key= interaction_key_new (client, dialog_type, interp,
			    function, data, destroy);

  g_return_if_fail (key);

  status= SmcInteractRequest ((SmcConn) client->smc_conn, _dialog_type,
			      client_interact_callback, (SmPointer) client);

  if (status)
    {
        client->interaction_keys = g_slist_append (client->interaction_keys,
						   key);
    }
  else
    {
      interaction_key_destroy (key);
    }
#endif /* HAVE_LIBSM */
}

static void
gnome_client_save_dialog_show (GnomeClient *client, gint key,
			       GnomeDialogType type, gpointer data)
{
  GtkDialog* dialog = GTK_DIALOG (data);
  gboolean shutdown_cancelled;

  if (client->shutdown)
	  gtk_dialog_add_button (dialog, _("Cancel Logout"), GTK_RESPONSE_CANCEL);
  gtk_widget_show_all (GTK_WIDGET (dialog));
  /* These are SYSTEM modal dialogs so map them above everything else */
#if 0
  /* FIXME: hmmm, _NET doesn't include a way to do this, I'm not sure
   * if window type == DOCK is a correct way and the old win_hints
   * layer stuff is no more */
  gnome_win_hints_set_layer (GTK_WIDGET (dialog), WIN_LAYER_ABOVE_DOCK);
#endif
  shutdown_cancelled = (gtk_dialog_run (dialog) == GTK_RESPONSE_CANCEL);
  gnome_interaction_key_return (key, shutdown_cancelled);
}

/**
 * gnome_client_save_any_dialog
 * @client: Pointer to #GnomeClient object.
 * @dialog: Pointer to GNOME dialog widget (a #GtkDialog widget).
 *
 * Description:
 * May be called during a "save_youself" handler to request that a
 * (modal) dialog is presented to the user. The session manager decides
 * when the dialog is shown, but it will not be shown it unless the
 * session manager is sending an interaction style of #GNOME_INTERACT_ANY.
 * A "Cancel Logout" button will be added during a shutdown.
 **/
void
gnome_client_save_any_dialog (GnomeClient *client, GtkDialog *dialog)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  g_return_if_fail (GTK_IS_DIALOG (dialog));

  if (client->interact_style == GNOME_INTERACT_ANY)
      gnome_client_request_interaction (client,
					GNOME_DIALOG_NORMAL,
					gnome_client_save_dialog_show,
					(gpointer) dialog);
}

/**
 * gnome_client_save_error_dialog
 * @client: Pointer to #GnomeClient object.
 * @dialog: Pointer to GNOME dialog widget (a #GtkDialog widget).
 *
 * Description:
 * May be called during a "save_youself" handler when an error has occurred
 * during the save. The session manager decides when the dialog is shown, but
 * it will not be shown it unless the session manager is sending an interaction
 * style of #GNOME_INTERACT_ANY. A "Cancel Logout" button will be added
 * during a shutdown.
 **/

void
gnome_client_save_error_dialog (GnomeClient *client, GtkDialog *dialog)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  g_return_if_fail (GTK_IS_DIALOG (dialog));

  if (client->interact_style != GNOME_INTERACT_NONE)
    gnome_client_request_interaction (client,
				      GNOME_DIALOG_ERROR,
				      gnome_client_save_dialog_show,
				      (gpointer) dialog);
}

/**
 * gnome_client_request_interaction
 * @client: A #GnomeClient object.
 * @dialog_type: The type of dialog to create.
 * @function: Callback to invoke to perform the interaction.
 * @data: Callback data.
 *
 * Description: Use the following functions, if you want to interact with the
 * user during a "save_yourself" handler without being restricted to using the
 * dialog based commands gnome_client_save_any_dialog() or
 * gnome_client_save_error_dialog(). Note, however, that overriding the session
 * manager specified preference in this way (by using arbitrary dialog boxes)
 * is not very nice.
 *
 * If and when the session manager decides that it's the app's turn to interact
 * then 'func' will be called with the specified arguments and a unique
 * 'GnomeInteractionKey'. The session manager will block other
 * clients from interacting until this key is returned with
 * gnome_interaction_key_return().
 **/

void
gnome_client_request_interaction (GnomeClient *client,
				  GnomeDialogType dialog_type,
				  GnomeInteractFunction function,
				  gpointer data)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  g_return_if_fail ((client->state == GNOME_CLIENT_SAVING_PHASE_1) ||
		    (client->state == GNOME_CLIENT_SAVING_PHASE_2));

  g_return_if_fail ((client->interact_style != GNOME_INTERACT_NONE) &&
		    ((client->interact_style == GNOME_INTERACT_ANY) ||
		     (dialog_type == GNOME_DIALOG_ERROR)));

  gnome_client_request_interaction_internal (client, dialog_type,
					     FALSE, function, data, NULL);
}


/**
 * gnome_client_request_interaction_interp
 * @client: Pointer to GNOME session client object.
 * @dialog_type: Type of dialog to show.
 * @function: Callback to perform the interaction (a #GnomeInteractFunction).
 * @data: Callback data.
 * @destroy: Function to destroy callback data.
 *
 * Description: Similar to gnome_client_request_interaction(), but used when
 * you need to destroy the callback data after the interaction.
 **/

void
gnome_client_request_interaction_interp (GnomeClient *client,
					 GnomeDialogType dialog_type,
					 GtkCallbackMarshal function,
					 gpointer data,
					 GDestroyNotify destroy)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  g_return_if_fail ((client->state == GNOME_CLIENT_SAVING_PHASE_1) ||
		    (client->state == GNOME_CLIENT_SAVING_PHASE_2));

  g_return_if_fail ((client->interact_style != GNOME_INTERACT_NONE) &&
		    ((client->interact_style == GNOME_INTERACT_ANY) ||
		     (dialog_type == GNOME_DIALOG_ERROR)));

  gnome_client_request_interaction_internal (client, dialog_type,
					     TRUE,
					     (GnomeInteractFunction)function,
					     data, destroy);
}


/*****************************************************************************/


/**
 * gnome_client_request_phase_2
 * @client: A #GnomeClient object.
 *
 * Description:  Request the session managaer to emit the "save_yourself"
 * signal for a second time after all the clients in the session have ceased
 * interacting with the user and entered an idle state. This might be useful if
 * your app manages other apps and requires that they are in an idle state
 * before saving its final data.
 **/

void
gnome_client_request_phase_2 (GnomeClient *client)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  /* Check if we are in save phase one */
  g_return_if_fail (client->state == GNOME_CLIENT_SAVING_PHASE_1);

  client->save_phase_2_requested= TRUE;
}


/**
 * gnome_client_request_save
 * @client: Pointer to GNOME session client object.
 * @save_style: Save style to request.
 * @shutdown: Whether to log out of the session.
 * @interact_style: Whether to allow user interaction.
 * @fast: Minimize activity to save as soon as possible.
 * @global: Request that all other apps in the session also save their state.
 *
 * Description: Request the session manager to save the session in
 * some way. The arguments correspond with the arguments passed to the
 * "save_yourself" signal handler.
 *
 * The save_style indicates whether the save should affect data
 * accessible to other users (#GNOME_SAVE_GLOBAL) or only the state
 * visible to the current user (#GNOME_SAVE_LOCAL) or both. Setting
 * shutdown to %TRUE will initiate a logout. The interact_style
 * specifies which kinds of interaction will be available. Setting
 * fast to %TRUE will limit the save to setting the session manager
 * properties plus any essential data.  Setting the value of global to
 * %TRUE will request that all the other apps in the session do a save
 * as well. A global save is mandatory when doing a shutdown.
 *
 **/

void
gnome_client_request_save (GnomeClient	       *client,
			   GnomeSaveStyle	save_style,
			   gboolean		shutdown,
			   GnomeInteractStyle	interact_style,
			   gboolean		fast,
			   gboolean		global)
{
#ifdef HAVE_LIBSM
  int _save_style;
  int _interact_style;
#endif

  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  switch (save_style)
    {
    case GNOME_SAVE_GLOBAL:
#ifdef HAVE_LIBSM
      _save_style= SmSaveGlobal;
      break;
#endif

    case GNOME_SAVE_LOCAL:
#ifdef HAVE_LIBSM
      _save_style= SmSaveLocal;
      break;
#endif

    case GNOME_SAVE_BOTH:
#ifdef HAVE_LIBSM
      _save_style= SmSaveBoth;
#endif
      break;

    default:
      g_assert_not_reached ();
      return;
    }

  switch (interact_style)
    {
    case GNOME_INTERACT_NONE:
#ifdef HAVE_LIBSM
      _interact_style= SmInteractStyleNone;
      break;
#endif

    case GNOME_INTERACT_ERRORS:
#ifdef HAVE_LIBSM
      _interact_style= SmInteractStyleErrors;
      break;
#endif

    case GNOME_INTERACT_ANY:
#ifdef HAVE_LIBSM
      _interact_style= SmInteractStyleAny;
#endif
      break;

    default:
      g_assert_not_reached ();
      return;
    }

  if (GNOME_CLIENT_CONNECTED (client))
    {
#ifdef HAVE_LIBSM
      if (shutdown)
	{
	  gnome_triggers_do("Session shutdown", NULL,
			    "gnome", "logout", NULL);
	}
      SmcRequestSaveYourself ((SmcConn) client->smc_conn, _save_style,
			      shutdown, _interact_style,
			      fast, global);
#endif
    }
  else
    {
      gboolean ret;
      g_signal_emit (client, client_signals[SAVE_YOURSELF], 0,
		     1, save_style, shutdown, interact_style, fast, &ret);
      if (shutdown)
	g_signal_emit (client, client_signals[DIE], 0);
    }
}

/*****************************************************************************/
/* 'InteractionKey' stuff */


/**
 * gnome_interaction_key_return
 * @key: Key passed to interaction callback
 * @cancel_shutdown: If %TRUE, cancel the shutdown
 *
 * Description: Used in interaction callback to tell the session manager
 * you are done interacting.
 **/

void
gnome_interaction_key_return (gint     tag,
			      gboolean cancel_shutdown)
{
#ifdef HAVE_LIBSM
  InteractionKey *key;
  GnomeClient    *client;

  key= interaction_key_find_by_tag (tag);
  g_return_if_fail (key);

  client= key->client;

  interaction_key_destroy (key);

  /* The case that 'client != NULL' should only occur, if the
     connection to the session manager was closed, while we where
     interacting or we received a SHUTDOWN_CANCELLED while
     interacting.  */
  if (client == NULL)
    return;

  client->interaction_keys= g_slist_remove (client->interaction_keys, key);

  if (cancel_shutdown && !client->shutdown)
    cancel_shutdown= FALSE;

  SmcInteractDone ((SmcConn) client->smc_conn, cancel_shutdown);

  client_save_yourself_possibly_done (client);
#endif /* HAVE_LIBSM */
}

/*****************************************************************************/
/* array helping functions - these function should be replaced by g_lists */

static gchar **
array_init_from_arg (gint argc, gchar *argv[])
{
  gchar **array;
  int i;

  if (argv == NULL)
    {
      g_return_val_if_fail (argc == 0, NULL);

      return NULL;
    }
  else
    {
      /* Now initialize the array.  */
      array = g_new (gchar *, argc + 1);

      for(i = 0; i < argc; i++)
	array[i] = g_strdup(argv[i]);

      array[i] = NULL;
    }

  return array;
}
