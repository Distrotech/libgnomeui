
/* gnome-client.c - GNOME session management client support
 *
 * Copyright (C) 1998 Carsten Schaar
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

#include <config.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <pwd.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>

#include "libgnome/libgnomeP.h"
#include "gnome-client.h"
#include "gnome-ice.h"
#include "gnome-winhints.h"
#include <gtk/gtk.h>
#include <gdk/gdkprivate.h>
#include <X11/Xatom.h>

extern char *program_invocation_name;
extern char *program_invocation_short_name;

#ifdef HAVE_LIBSM
#include <X11/SM/SMlib.h>
#endif /* HAVE_LIBSM */

static GtkWidget *client_grab_widget = NULL;

enum {
  SAVE_YOURSELF,
  DIE,
  SAVE_COMPLETE,
  SHUTDOWN_CANCELLED,
  CONNECT,
  DISCONNECT,
  LAST_SIGNAL
};

typedef gint (*GnomeClientSignal1) (GnomeClient        *client,
				    gint                phase,
				    GnomeSaveStyle      save_style,
				    gint                shutdown,
				    GnomeInteractStyle  interact_style,
				    gint                fast,
				    gpointer            client_data);
typedef void (*GnomeClientSignal2) (GnomeClient        *client,
				    gint                restarted,
				    gpointer            client_data);

static void gnome_client_marshal_signal_1 (GtkObject     *object,
					   GtkSignalFunc  func,
					   gpointer       func_data,
					   GtkArg        *args);
static void gnome_client_marshal_signal_2 (GtkObject     *object,
					   GtkSignalFunc  func,
					   gpointer       func_data,
					   GtkArg        *args);

static void gnome_client_class_init              (GnomeClientClass *klass);
static void gnome_client_object_init             (GnomeClient      *client);

static void gnome_real_client_destroy            (GtkObject        *object);
static void gnome_real_client_save_complete      (GnomeClient      *client);
static void gnome_real_client_shutdown_cancelled (GnomeClient      *client);
static void gnome_real_client_connect            (GnomeClient      *client,
						  gint              restarted);
static void gnome_real_client_disconnect         (GnomeClient      *client);

static void   client_unset_config_prefix    (GnomeClient *client);

static gchar** array_init_from_arg           (gint argc, 
					      gchar *argv[]);
static gchar** array_copy                    (gchar **source);


static GtkObjectClass *parent_class = NULL;
static gint client_signals[LAST_SIGNAL] = { 0 };

static const char *sm_client_id_arg_name = "--sm-client-id";
static const char *sm_config_prefix_arg_name = "--sm-config-prefix";
static const char *sm_client_id_prop="SM_CLIENT_ID";


/* The master client.  */
static GnomeClient *master_client= NULL;

static gboolean gnome_client_auto_connect_master = TRUE;
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
  GtkDestroyNotify       destroy;
};


/* List of all existing interaction keys.  */
static GList *interact_functions = NULL;


static InteractionKey *
interaction_key_new (GnomeClient           *client,
		     GnomeDialogType        dialog_type,
		     gboolean               interp,
		     GnomeInteractFunction  function,
		     gpointer               data,
		     GtkDestroyNotify       destroy)
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
      args[3].type          = GTK_TYPE_ENUM;
      args[3].d.pointer_data= &key->dialog_type;

      ((GtkCallbackMarshal)key->function) (NULL, key->data, 3, args);
    }
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
  
  argc = g_hash_table_size (table);

  if (argc == 0) 
    return;
  
  /* Now initialize the 'vals' array.  */
  vals = g_new (SmPropValue, argc);
  tmp = vals;
  
  g_hash_table_foreach (table, (GHFunc) client_set_ghash0, &tmp);
  
  client_set_value (client, name, SmLISTofARRAY8, argc, vals);

  g_free (vals);
  
};


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
  gint    i;

  SmPropValue *vals;
  
  if (!GNOME_CLIENT_CONNECTED (client) || (client->clone_command == NULL))
    return;

  for (ptr = client->clone_command, argc = 0; *ptr ; ptr++) argc++;

  /* Add space for static arguments and config prefix. */
  argc += g_list_length (client->static_args) + 2;

  vals = g_new (SmPropValue, argc);

  ptr = client->clone_command;

  vals[0].length = strlen (*ptr);
  vals[0].value  = *ptr++;

  vals[1].length = strlen (sm_config_prefix_arg_name);
  vals[1].value = (char *) sm_config_prefix_arg_name;
  vals[2].length = strlen (gnome_client_get_config_prefix (client));
  vals[2].value = client->config_prefix;

  for (list = client->static_args, i = 3; list; i++, list= g_list_next (list))
    {
      vals[i].length= strlen ((gchar *)list->data);
      vals[i].value = (gchar *)list->data;
    }

  for (; *ptr ; i++)
    {
      vals[i].length = strlen (*ptr);
      vals[i].value  = *ptr++;
    }

  client_set_value (client, SmCloneCommand, SmLISTofARRAY8, argc, vals);
  
  g_free (vals);
}		   

static void
client_set_restart_command (GnomeClient *client)
{
  GList  *list;
  gint    argc;
  gchar **ptr;
  gint    i;

  SmPropValue *vals;
  
  if (!GNOME_CLIENT_CONNECTED (client) || (client->restart_command == NULL))
    return;

  for (ptr = client->restart_command, argc = 0; *ptr ; ptr++) argc++;

  /* Add space for static arguments, config prefix and client id. */
  argc += g_list_length (client->static_args) + 4;

  vals = g_new (SmPropValue, argc);

  ptr = client->restart_command;

  vals[0].length = strlen (*ptr);
  vals[0].value  = *ptr++;

  vals[1].length = strlen (sm_config_prefix_arg_name);
  vals[1].value = (char *) sm_config_prefix_arg_name;
  vals[2].length = strlen (gnome_client_get_config_prefix (client));
  vals[2].value = client->config_prefix;

  vals[3].length = strlen (sm_client_id_arg_name);
  vals[3].value = (char *) sm_client_id_arg_name;
  vals[4].length = strlen (client->client_id);
  vals[4].value = client->client_id;

  for (list = client->static_args, i = 5; list; i++, list = g_list_next (list))
    {
      vals[i].length= strlen ((gchar *)list->data);
      vals[i].value = (gchar *)list->data;
    }

  for (; *ptr ; i++)
    {
      vals[i].length = strlen (*ptr);
      vals[i].value  = *ptr++;
    }

  client_set_value (client, SmRestartCommand, SmLISTofARRAY8, argc, vals);
  
  g_free (vals);
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

  client_set_state (client, GNOME_CLIENT_SAVING_PHASE_2);
 
  gtk_signal_emit (GTK_OBJECT (client), client_signals[SAVE_YOURSELF],
		   2,
		   client->save_style,
		   client->shutdown,
		   client->interact_style,
		   client->fast);

  client_save_yourself_possibly_done (client);
}


static void
client_save_yourself_callback (SmcConn   smc_conn,
			       SmPointer client_data,
			       int       save_style,
			       Bool      shutdown,
			       int       interact_style,
			       Bool      fast)
{
  GnomeClient *client= (GnomeClient*) client_data;
  gchar *name, *prefix;
  int fd;

  if (!client_grab_widget)
    client_grab_widget = gtk_widget_new (gtk_widget_get_type(), NULL);

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

  while (gdk_pointer_is_grabbed())
    gtk_main_iteration();

  /* Check that we did not receive a shutdown cancelled while waiting 
   * for the grab to be released. The grab should prevent it but ... */
  if (client->state != GNOME_CLIENT_SAVING_PHASE_1)
    return;

  gdk_pointer_ungrab (GDK_CURRENT_TIME);
  gdk_keyboard_ungrab (GDK_CURRENT_TIME);
  gtk_grab_add (client_grab_widget);

  name = client->program;

  if ((prefix = strrchr (name, '/')))
    name = prefix + 1;

  prefix = g_strconcat ("/", name, "-XXXXX", "/", NULL);
  name = gnome_config_get_real_path (prefix);
  
  free (prefix);

  name [strlen (name) - 1] = 'X';
  fd = mkstemp (name);

  if (fd != -1)
    {
      unlink (name);
      close (fd);
      client->config_prefix = g_strconcat (strrchr (name, '/'), "/", NULL);

      if (client == master_client)
	{
	  /* The config prefix has been changed, so it cannot be used
             anymore to restore a saved session.  */
	  master_client_restored= FALSE;
	}
    }
  free (name);

  client_set_clone_command (client);
  client_set_restart_command (client);

  gtk_signal_emit (GTK_OBJECT (client), 
		   client_signals[SAVE_YOURSELF],
		   1, 
		   client->save_style, 
		   shutdown, 
		   client->interact_style, 
		   fast);

  if (!client->save_yourself_emitted)
    client->save_successfull= FALSE;

  client_save_yourself_possibly_done (client);
}


static void
client_die_callback (SmcConn smc_conn, SmPointer client_data)
{
  GnomeClient *client= (GnomeClient*) client_data;

  gtk_grab_remove (client_grab_widget);

  gtk_signal_emit (GTK_OBJECT (client), client_signals[DIE]);
}


static void
client_save_complete_callback (SmcConn smc_conn, SmPointer client_data)
{
  GnomeClient *client = (GnomeClient*) client_data;

  gtk_grab_remove (client_grab_widget);

  gtk_signal_emit (GTK_OBJECT (client), client_signals[SAVE_COMPLETE]);
}


static void
client_shutdown_cancelled_callback (SmcConn smc_conn, SmPointer client_data)
{
  GnomeClient *client= (GnomeClient*) client_data;

  gtk_grab_remove (client_grab_widget);

  gtk_signal_emit (GTK_OBJECT (client), client_signals[SHUTDOWN_CANCELLED]);
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

/* Forward declaration for our parsing function.  */
static void client_parse_func (poptContext ctx,
			       enum poptCallbackReason reason,
			       const struct poptOption *opt,
			       const char *arg, void *data);


/* Command-line arguments understood by this module.  */
static const struct poptOption options[] = {
  {NULL, '\0', POPT_ARG_INTL_DOMAIN, PACKAGE, 0, NULL, NULL},
  {NULL, '\0', POPT_ARG_CALLBACK | POPT_CBFLAG_PRE | POPT_CBFLAG_POST, 
   client_parse_func, 0, NULL, NULL},
  {"sm-client-id", '\0', POPT_ARG_STRING, NULL, -1, 
   N_("Specify session management ID"), N_("ID")},
  {"sm-config-prefix", '\0', POPT_ARG_STRING, NULL, -2, 
   N_("Specify prefix of saved configuration"), N_("PREFIX")},
  {"sm-disable", '\0', POPT_ARG_NONE, NULL, -3, 
   N_("Disable connection to session manager"), NULL},
  {NULL, '\0', 0, NULL, 0}
};


/* Parse command-line arguments we recognize.  */
static void
client_parse_func (poptContext ctx,
		   enum poptCallbackReason reason,
		   const struct poptOption *opt,
		   const char *arg, void *data)
{
  int key = opt ? opt->val : 0;

  /* 'gnome_client_init' must have been called, before call
     'client_parse_func'.  */
  g_return_if_fail (master_client);

  if (reason == POPT_CALLBACK_REASON_PRE) 
    {
      /* needed on non-glibc systems: */
      gnome_client_set_program (master_client, program_invocation_name);
      /* Argument parsing is starting.  We set the restart and the
	 clone command to a default value, so other functions can use
	 the master client while parsing the command line.  */
      gnome_client_set_restart_command (master_client, 1, 
					&master_client->program);
      gnome_client_set_clone_command (master_client, 0, NULL);      
    }
  else if (reason == POPT_CALLBACK_REASON_POST)
    {
      /* We're done, so we can connect to the session manager now.  */
      if (gnome_client_auto_connect_master)
	gnome_client_connect (master_client);
    }
  else if (key == -1)
    {
      /* Option: --sm-client-id  */
      gnome_client_set_id (master_client, arg);
    }
  else if (key == -2)
    {
      /* Option: --sm-config-prefix  */
      master_client->config_prefix= g_strdup (arg);
      master_client_restored = TRUE;    
    }
  else if (key == -3)
    {
      /* Option: --sm-disable  */
      gnome_client_disable_master_connection ();
    }
}


/**
 * gnome_client_disable_master_connection
 *
 * Description:
 **/

void         
gnome_client_disable_master_connection (void)
{
  gnome_client_auto_connect_master= FALSE;
}

/* Called at exit to ensure the ice connection is closed cleanly
 * This avoids io errors on the session manager side */
static void  
master_client_clean_up (void)
{
  if (GNOME_CLIENT_CONNECTED (master_client))
      gnome_client_disconnect (master_client);
}

static void
master_client_connect (GnomeClient *client,
		       gint         restarted,
		       gpointer     client_data)
{
  gdk_set_sm_client_id (gnome_client_get_id (client));

  g_atexit (master_client_clean_up);
}

static void
master_client_disconnect (GnomeClient *client,
			  gpointer client_data)
{
  gdk_set_sm_client_id (NULL);
}


/**
 * gnome_client_init
 *
 * Description:
 **/

void
gnome_client_init (void)
{
  gint i;
  gchar *buffer= NULL;

  /* This function must not be called more than once.  */
  g_return_if_fail (master_client == NULL);
  
  /* Make sure the Gtk+ type system is initialized.  */
  gtk_type_init ();
  gtk_signal_init ();

  /* Create the master client.  */
  master_client= gnome_client_new_without_connection ();
  g_assert (master_client);

  /* Initialise ICE */
  gnome_ice_init ();

  /* Connect the master client's default signals.  */
  gtk_signal_connect (GTK_OBJECT (master_client), "connect",
		      GTK_SIGNAL_FUNC (master_client_connect), NULL);
  gtk_signal_connect (GTK_OBJECT (master_client), "disconnect",
		      GTK_SIGNAL_FUNC (master_client_disconnect), NULL);

  /* Register commandline options.  */
  gnomelib_register_popt_table (options, N_("Session management options"));

  /* Set the master client's environment.  */
  for (i= 0; master_environment[i]; i++)
    {
      char *value= getenv (master_environment[i]);
	      
      if (value)
	gnome_client_set_environment (master_client,
				      master_environment[i],
				      value);
    }

  /* Set the current directory.  */
  i= 512;
  while (buffer == NULL)
    {
      buffer= (gchar *) g_malloc (i);
	      
      if (getcwd (buffer, i) == NULL)
	{
	  g_free (buffer);
	  i *= 2;
	  buffer= NULL;
	  /* ERANGE means that we needed more space in the
	     buffer.  Other errors mean don't bother setting
	     the directory information.  */
	  if (errno != ERANGE)
	    break;
	}
    }
  if (buffer != NULL)
    {
      gnome_client_set_current_directory (master_client, buffer);
      g_free (buffer);
    }
}


/**
 * gnome_master_client
 *
 * Description:
 *
 * Returns:  Pointer to
 **/

GnomeClient*
gnome_master_client (void)
{
  return master_client;
}


/**
 * gnome_cloned_client
 *
 * Description:
 *
 * Returns:  Pointer to
 **/

GnomeClient*
gnome_cloned_client (void)
{
  g_warning ("'gnome_cloned_client' is not working as expected anymore.\n");
  return NULL;
}

/*****************************************************************************/
/* GTK-class managing functions */

GtkType
gnome_client_get_type (void)
{
  static GtkType client_type = 0;
  
  if (!client_type)
    {
      GtkTypeInfo client_info =
      {
	"GnomeClient",
	sizeof (GnomeClient),
	sizeof (GnomeClientClass),
	(GtkClassInitFunc) gnome_client_class_init,
	(GtkObjectInitFunc) gnome_client_object_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL
      };

      client_type = gtk_type_unique (gtk_object_get_type (), &client_info);
    }
  
  return client_type;
}

static void
gnome_client_class_init (GnomeClientClass *klass)
{
  GtkObjectClass *object_class = (GtkObjectClass*) klass;
  
  parent_class = gtk_type_class (gtk_object_get_type ());
  
  
  client_signals[SAVE_YOURSELF] =
    gtk_signal_new ("save_yourself",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeClientClass, save_yourself),
		    gnome_client_marshal_signal_1,
		    GTK_TYPE_BOOL, 5,
		    GTK_TYPE_INT,
		    GTK_TYPE_ENUM,
		    GTK_TYPE_BOOL,
		    GTK_TYPE_ENUM,
		    GTK_TYPE_BOOL);
  client_signals[DIE] =
    gtk_signal_new ("die",
		    GTK_RUN_LAST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeClientClass, die),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);
  client_signals[SAVE_COMPLETE] =
    gtk_signal_new ("save_complete",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeClientClass, save_complete),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);
  client_signals[SHUTDOWN_CANCELLED] =
    gtk_signal_new ("shutdown_cancelled",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeClientClass, shutdown_cancelled),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);
  client_signals[CONNECT] =
    gtk_signal_new ("connect",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeClientClass, connect),
		    gnome_client_marshal_signal_2,
		    GTK_TYPE_NONE, 1,
		    GTK_TYPE_BOOL);
  client_signals[DISCONNECT] =
    gtk_signal_new ("disconnect",
		    GTK_RUN_FIRST,
		    object_class->type,
		    GTK_SIGNAL_OFFSET (GnomeClientClass, disconnect),
		    gtk_signal_default_marshaller,
		    GTK_TYPE_NONE, 0);
  
  gtk_object_class_add_signals (object_class, client_signals, LAST_SIGNAL);
  
  object_class->destroy = gnome_real_client_destroy;
  
  klass->save_yourself      = NULL;
  klass->die                = gnome_client_disconnect;
  klass->save_complete      = gnome_real_client_save_complete;
  klass->shutdown_cancelled = gnome_real_client_shutdown_cancelled;
  klass->connect            = gnome_real_client_connect;
  klass->disconnect         = gnome_real_client_disconnect;
}

static void
gnome_client_object_init (GnomeClient *client)
{
  struct passwd *pwd;

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
  
  g_return_if_fail (object != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (object));
  
  client = GNOME_CLIENT (object);
  
  if (GNOME_CLIENT_CONNECTED (client))
    gnome_client_disconnect (client);

  g_free (client->client_id);
  g_free (client->previous_id);
  g_free (client->config_prefix);
  g_free (client->global_config_prefix);

  g_list_foreach (client->static_args, (GFunc)g_free, NULL);
  g_list_free    (client->static_args);

  g_strfreev (client->clone_command);
  g_free     (client->current_directory);
  g_strfreev (client->discard_command);

  g_hash_table_foreach_remove (client->environment, 
			       (GHRFunc)environment_entry_remove, NULL);
  g_hash_table_destroy        (client->environment);

  g_free     (client->program);
  g_strfreev (client->resign_command);
  g_strfreev (client->restart_command);
  g_strfreev (client->shutdown_command);
  g_free     (client->user_id);

  if (GTK_OBJECT_CLASS (parent_class)->destroy)
    (* GTK_OBJECT_CLASS (parent_class)->destroy) (object);
}

/*****************************************************************************/
/* 'gnome_client' public member functions */


/**
 * gnome_client_new
 *
 * Description: Allocates memory for a new GNOME session management
 * client object.  After allocating, the client tries to connect to a
 * session manager.
 *
 * Returns: Pointer to a newly allocated GNOME session management
 * client object.
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
 * Description: Allocates memory for a new GNOME session management
 * client object.
 *
 * Returns: Pointer to a newly allocated GNOME session management
 * client object.
 **/

GnomeClient *
gnome_client_new_without_connection (void)
{
  GnomeClient *client;

  client= gtk_type_new (gnome_client_get_type ());

  /* Preset the CloneCommand, RestartCommand and Program properties.
     FIXME: having a default would be cool, but it is probably hard to
     get this to interact correctly with the distributed command-line
     parsing.  */
  client->clone_command   = NULL;
  client->restart_command = NULL;

  /* Non-glibc systems do not get this set on the master_client until 
     client_parse_func but this is not a problem.
     The SM specs require explictly require that this is the value: */
  client->program = g_strdup (program_invocation_name);

  return client;
}


/**
 * gnome_client_flush
 * @client: Pointer to GNOME session client object.
 *
 * Description:
 * This will force the underlying connection to the session manager to
 * be flushed.  This is useful if you have some pending changes that
 * you want to make sure get committed.
 **/

void
gnome_client_flush (GnomeClient *client)
{
#ifdef HAVE_LIBSM
  IceConn conn;
#endif

  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_CLIENT_CONNECTED (client));

#ifdef HAVE_LIBSM
  conn = SmcGetIceConnection ((SmcConn) client->smc_conn);
  IceFlush (conn);
#endif
}

/*****************************************************************************/

#define ERROR_STRING_LENGTH 256


/**
 * gnome_client_connect
 * @client: Pointer to GNOME session client object.
 *
 * Description:
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

  if (getenv ("SESSION_MANAGER"))
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
      IceConn ice_conn;
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
      gtk_signal_emit (GTK_OBJECT (client), client_signals[CONNECT], 
		       restarted);
    }
#endif /* HAVE_LIBSM */
}



/**
 * gnome_client_disconnect
 * @client: Pointer to GNOME session client object.
 *
 * Description:
 **/

void
gnome_client_disconnect (GnomeClient *client)
{
  g_return_if_fail (client != NULL);

  if (GNOME_CLIENT_CONNECTED (client))
    {
      gnome_client_flush (client);
      gtk_signal_emit (GTK_OBJECT (client), client_signals[DISCONNECT]);
    }
}

/*****************************************************************************/

/**
 * gnome_client_get_flags:
 * @client: Pointer to GNOME session client object.
 * 
 * Description:
 *
 * Returns some flags, that give additional information about this
 * client.  Right now, the following flags are supported:
 * 
 * - GNOME_CLIENT_IS_CONNECTED: The client is connected to a session
 *   manager (It's the same information like using *
 *   GNOME_CLIENT_CONNECTED).
 *  
 * - GNOME_CLIENT_RESTARTED: The client has been restarted, i. e. it
 *   has been running with the same client id before.
 *    
 * - GNOME_CLIENT_RESTORED: This flag is only used for the master
 *   client.  It indicates, that there may be a configuraion file from
 *   which the clients state should be restored (using the
 *   gnome_client_get_config_prefix call).
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
 * @argc:
 * @argv[]:
 *
 * Description:
 **/

void 
gnome_client_set_clone_command (GnomeClient *client, 
				gint argc, gchar *argv[])
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));

  /* Usually the CloneCommand property is required.  If you unset the
     CloneCommand property, we will use the RestartCommand instead.  */
  if (argv == NULL)
    {
      g_return_if_fail (argc == 0);

      g_strfreev (client->clone_command);
      client->clone_command = array_copy (client->restart_command);
    }
  else
    {
      g_strfreev (client->clone_command);
      client->clone_command = array_init_from_arg (argc, argv);
    }

#ifdef HAVE_LIBSM
  client_set_clone_command (client);
#endif /* HAVE_LIBSM */
}

/**
 * gnome_client_set_current_directory
 * @client: Pointer to GNOME session client object.
 * @dir:
 *
 * Description:
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



/**
 * gnome_client_set_discard_command
 * @client: Pointer to GNOME session client object.
 * @argc:
 * @argv[]:
 *
 * Description:
 **/

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
 * @name:
 * @value:
 *
 * Description:
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
 * @pid:
 *
 * Description:
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
 * @program:
 *
 * Description:
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
 * @argc:
 * @argv[]:
 *
 * Description:
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
 * @argc:
 * @argv[]:
 *
 * Description:
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

  client_set_gchar (client, "Priority", (gchar) priority);
#endif
}

/**
 * gnome_client_set_restart_style
 * @client: Pointer to GNOME session client object.
 * @style:
 *
 * Description:
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
 * @argc:
 * @argv[]:
 *
 * Description:
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
 * @id:
 *
 * Description:
 **/

void 
gnome_client_set_user_id (      GnomeClient *client,
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
 * @...:
 *
 * Description:
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
 * @client: Pointer to GNOME session client object.
 * @id:
 *
 * Description:
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
 * @client: Pointer to GNOME session client object.
 *
 * Description:
 *
 * Returns:  Pointer to
 **/

gchar *
gnome_client_get_id (GnomeClient *client)
{
  g_return_val_if_fail (client != NULL, NULL);
  g_return_val_if_fail (GNOME_IS_CLIENT (client), NULL);
  
  return client->client_id;
}

/**
 * gnome_client_get_previous_id
 * @client: Pointer to GNOME session client object.
 *
 * Description:
 *
 * Returns:  Pointer to
 **/

gchar *
gnome_client_get_previous_id (GnomeClient *client)
{
  g_return_val_if_fail (client != NULL, NULL);
  g_return_val_if_fail (GNOME_IS_CLIENT (client), NULL);

  return client->previous_id;
}

/**
 * gnome_client_get_config_prefix
 * @client: Pointer to GNOME session client object.
 *
 * Description:
 *
 * Returns:  Pointer to
 **/

gchar *
gnome_client_get_config_prefix (GnomeClient *client)
{
  if (!client || !GNOME_IS_CLIENT (client))
    client = master_client;

  if (!client->config_prefix)
    client->config_prefix = gnome_client_get_global_config_prefix (client);

  return client->config_prefix;
}

static gchar* config_prefix = NULL;

/**
 * gnome_client_set_global_config_prefix
 * @client: Pointer to GNOME session client object.
 * @prefix: Prefix for saving the global configuration
 *
 * Description:
 **/

void
gnome_client_set_global_config_prefix (GnomeClient *client, gchar* prefix)
{
  if (client == NULL)
    {
      config_prefix= g_strdup (prefix);
    }

  g_return_if_fail (GNOME_IS_CLIENT (client));
  
  client->global_config_prefix = g_strdup (prefix);
}

/**
 * gnome_client_get_global_config_prefix
 * @client: Pointer to GNOME session client object.
 *
 * Description:
 *
 * Returns:  Pointer to
 **/

gchar *
gnome_client_get_global_config_prefix (GnomeClient *client)
{
  if (client == NULL)
    {
      if (!config_prefix)
	config_prefix= g_strconcat ("/", program_invocation_short_name, "/",
				       NULL);

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
			   gint         restarted)
{  
  g_return_if_fail (client != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  
  if (client->clone_command == 0)
    client->clone_command = array_copy (client->restart_command);

#ifdef HAVE_LIBSM
  /* We now set all non empty properties.  */
  if (client->clone_command != NULL)
    client_set_clone_command (client);
  else
    client_set_array (client, SmCloneCommand, NULL);

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

  if (client->restart_command != NULL)
    client_set_restart_command (client);
  else
    client_set_array (client, SmRestartCommand, NULL);

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
					   GtkDestroyNotify       destroy) 
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
  GnomeDialog* dialog = GNOME_DIALOG (data);
  gboolean shutdown_cancelled;
  gint cancel_button = g_list_length (dialog->buttons);

  /* These are SYSTEM modal dialogs so map them above everything else */
  gnome_win_hints_set_layer (GTK_WIDGET (dialog), WIN_LAYER_ABOVE_DOCK);

  if (client->shutdown) 
    gnome_dialog_append_button (dialog, _("Cancel Logout"));
  shutdown_cancelled = (cancel_button == gnome_dialog_run_and_close (dialog));
  gnome_interaction_key_return (key, shutdown_cancelled);
}

/**
 * gnome_client_save_any_dialog
 * @client: Pointer to GNOME session client object.
 * @dialog: Pointer to GNOME dialog widget.
 *
 * Description:
 * May be called during a "save_youself" handler to request that a 
 * (modal) dialog is presented to the user. The session manager decides 
 * when the dialog is shown but it will not be shown it unless the 
 * interact_style == GNOME_INTERACT_ANY. A "Cancel Logout" button 
 * will be added during a shutdown.
 **/
void         
gnome_client_save_any_dialog (GnomeClient *client, GnomeDialog *dialog)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  g_return_if_fail (GNOME_IS_DIALOG (dialog));

  if (client->interact_style == GNOME_INTERACT_ANY)
      gnome_client_request_interaction (client, 
					GNOME_DIALOG_NORMAL, 
					gnome_client_save_dialog_show,
					(gpointer) dialog);
}

/**
 * gnome_client_save_error_dialog
 * @client: Pointer to GNOME session client object.
 * @dialog: Pointer to GNOME dialog widget.
 *
 * Description:
 * May be called during a "save_youself" handler when an error has occured 
 * during the save. The session manager decides when the dialog is shown 
 * but it will not be shown when the interact_style == GNOME_INTERACT_NONE.
 * A "Cancel Logout" button will be added during a shutdown.
 **/

void         
gnome_client_save_error_dialog (GnomeClient *client, GnomeDialog *dialog)
{
  g_return_if_fail (client != NULL);
  g_return_if_fail (dialog != NULL);
  g_return_if_fail (GNOME_IS_CLIENT (client));
  g_return_if_fail (GNOME_IS_DIALOG (dialog));

  if (client->interact_style != GNOME_INTERACT_NONE)
    gnome_client_request_interaction (client, 
				      GNOME_DIALOG_ERROR, 
				      gnome_client_save_dialog_show,
				      (gpointer) dialog);
}

/**
 * gnome_client_request_interaction
 * @client: Pointer to GNOME session client object.
 * @dialog_type:
 * @function:
 * @data:
 *
 * Description:
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
 * @dialog_type:
 * @function:
 * @data:
 * @destroy:
 *
 * Description:
 **/

void
gnome_client_request_interaction_interp (GnomeClient *client,
					 GnomeDialogType dialog_type,
					 GtkCallbackMarshal function,
					 gpointer data,
					 GtkDestroyNotify destroy)
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
 * @client: Pointer to GNOME session client object.
 *
 * Description:
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
 * @save_style:
 * @shutdown:
 * @interact_style:
 * @fast:
 * @global:
 *
 * Description:
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
  
#ifdef HAVE_LIBSM
  if (GNOME_CLIENT_CONNECTED (client))
    {
      SmcRequestSaveYourself ((SmcConn) client->smc_conn, _save_style,
			      shutdown, _interact_style,
			      fast, global);            
    }
#endif HAVE_LIBSM
}

/*****************************************************************************/
/* 'InteractionKey' stuff */


/**
 * gnome_interaction_key_return
 * @key:
 * @cancel_shutdown:
 *
 * Description:
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

static void
gnome_client_marshal_signal_1 (GtkObject     *object,
			       GtkSignalFunc  func,
			       gpointer       func_data,
			       GtkArg        *args)
{
  GnomeClient        *client;
  GnomeClientSignal1  rfunc;
  
  client = (GnomeClient*)object;
  rfunc  = (GnomeClientSignal1) func;
  
  client->save_successfull = (* rfunc) (client,
					GTK_VALUE_INT (args[0]),
					GTK_VALUE_ENUM (args[1]),
					GTK_VALUE_BOOL (args[2]),
					GTK_VALUE_ENUM (args[3]),
					GTK_VALUE_BOOL (args[4]),
					func_data);

  client->save_yourself_emitted= TRUE;
}

static void
gnome_client_marshal_signal_2 (GtkObject     *object,
			       GtkSignalFunc  func,
			       gpointer       func_data,
			       GtkArg        *args)
{
  GnomeClientSignal2  rfunc;
  
  rfunc = (GnomeClientSignal2) func;
  
  (* rfunc) ((GnomeClient*)object,
	     GTK_VALUE_BOOL (args[0]),
	     func_data);
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

static gchar **
array_copy (gchar **source)
{
  gchar **array;
  gint    argc;
  int i, n;
  
  if (source == NULL)
    return NULL;

  /* Count number of elements in source array. */
  for (n = 0; source[n]; n++) /* */;
  
  /* Allocate memory for array. */
  array = g_new (gchar *, n + 1);

  /* Copy the elements. */
  for(i = 0; i < n; i++)
    array[i] = g_strdup(source[i]);
  
  array[i] = NULL;
  
  return array;
}
