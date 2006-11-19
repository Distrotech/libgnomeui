/* -*- Mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
/*
 * Copyright (C) 1999, 2000 Red Hat, Inc.
 * All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with the Gnome Library; see the file COPYING.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

#include <config.h>

/* needed for sigaction and friends under 'gcc -ansi -pedantic' on 
 * GNU/Linux */
#ifndef _GNU_SOURCE
#  define _GNU_SOURCE 1
#endif
#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>

#include <stdlib.h>
#include <unistd.h>
#include <libgnome/libgnome.h>
#include <libgnomeui/libgnomeui.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

/* Must be before all other gnome includes!! */
#include "gnome-i18nP.h"

int main(int argc, char *argv[])
{
  struct sigaction sa;
  poptContext ctx;
  const char **args;
  const char *app_version = NULL;
  gchar *appname;
  gchar *bug_buddy_path = NULL;
  
  int bb_sm_disable = 0;

  /* We do this twice to make sure we don't start running ourselves... :) */
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sigaction(SIGSEGV, &sa, NULL);


  bindtextdomain (GETTEXT_PACKAGE, GNOMEUILOCALEDIR);
  textdomain (GETTEXT_PACKAGE);

  /* in case gnome-session is segfaulting :-) */
  gnome_client_disable_master_connection();
  
  gnome_init_with_popt_table("gnome_segv2", VERSION, argc, argv, NULL, 0, &ctx);

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sigaction(SIGSEGV, &sa, NULL);

  args = poptGetArgs(ctx);
  if (!args || !args[0] || !args[1])
    {
      gsize bytes_read;
      gsize bytes_written;
      gchar* localestr=g_locale_from_utf8(_("Usage: gnome_segv2 appname signum\n"),-1,&bytes_read,&bytes_written,NULL);
      fprintf(stderr, localestr);
      g_free(localestr);
      return 1;
    }
  appname = g_strdup(args[0]);
  if (g_getenv("GNOME_HACKER")) 
    {
      gchar *exec_str;
      gchar *title;
      int retval;

      title = g_strdup_printf (_("Debugging %s"), appname);
      exec_str = g_strdup_printf("gnome-terminal "
                                 "--title=\"%s\" "
                                 "--command=\"gdb %s %d\"",
                                 title, appname, getppid());
      g_free (title);

      retval = system(exec_str);
      g_free(exec_str);
      if (retval == -1 || retval == 127)
        {
          g_warning("Couldn't run debugger: %s", g_strerror(errno));
        }
      return 0;
    }

  /* Give up if bug-buddy itself has aborted. */
  if (strcmp (appname, "bug-buddy") == 0)
    return 0;

  bug_buddy_path = g_find_program_in_path ("bug-buddy");
  if (bug_buddy_path != NULL)
    {
      gchar *exec_str;
      int retval;

      g_assert(bug_buddy_path);
      exec_str = g_strdup_printf("%s --appname=\"%s\" --pid=%d "
                                 "--package-ver=\"%s\" %s", 
                                 bug_buddy_path, appname, getppid(), 
                                 app_version, bb_sm_disable 
                                 ? "--sm-disable" : "");
#ifdef HAVE_SETENV
      setenv ("BUG_BUDDY_GNOME_VERSION", "2.0 (" VERSION ")", 1);
#else
      putenv ("BUG_BUDDY_GNOME_VERSION=2.0 (" VERSION ")");
#endif
      retval = system(exec_str);
      g_free(exec_str);
      if (retval == -1 || retval == 127)
        {
          g_warning("Couldn't run bug-buddy: %s", g_strerror(errno));
        }
    }
  else
    {
      GtkWidget *dialog;
      dialog = gtk_message_dialog_new_with_markup (NULL, GTK_DIALOG_MODAL, 
		                                   GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
						   _("<b>The application %s has crashed.</b>\n\n"
						    "Please install <b>bug-buddy</b>, the\n"
						    "GNOME crash report tool, which enables you\n"
						    "to send the developers detailed information\n"
						    "about the error.\n"), appname);
      gtk_dialog_run (GTK_DIALOG (dialog));
    }
      
  
  g_free (appname);

  return 0;
}
