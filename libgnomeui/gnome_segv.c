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
#ifndef _POSIX_SOURCE
#  define _POSIX_SOURCE 1
#endif
#include <sys/types.h>
#include <sys/wait.h>

#include <errno.h>

#include <stdlib.h>
#include <unistd.h>
#include <libgnome.h>
#include <libgnomeui.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

/* Must be before all other gnome includes!! */
#include "gnome-i18nP.h"

enum {
  RESPONSE_NULL,
  RESPONSE_BUG_BUDDY,
  RESPONSE_DEBUG
};

int main(int argc, char *argv[])
{
  GtkWidget *mainwin, *urlbtn;
  gchar* msg;
  struct sigaction sa;
  poptContext ctx;
  const char **args;
  char *urlstr;
  const char *app_version = NULL;
  int res;
  gchar *appname;
  gchar *bug_buddy_path = NULL;
  const char *debugger = NULL;
  gchar *debugger_path = NULL;
  
  int bb_sm_disable = 0;

  /* We do this twice to make sure we don't start running ourselves... :) */
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sigaction(SIGSEGV, &sa, NULL);


  bindtextdomain (PACKAGE, GNOMEUILOCALEDIR);
  textdomain (PACKAGE);

  /* in case gnome-session is segfaulting :-) */
  gnome_client_disable_master_connection();
  
  gnome_init_with_popt_table("gnome_segv", VERSION, argc, argv, NULL, 0, &ctx);

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sigaction(SIGSEGV, &sa, NULL);

  args = poptGetArgs(ctx);
  if (args && args[0] && args[1])
    {
      char *base = g_path_get_basename (args[0]);
      if (strcmp(base, "gnome-session") == 0)
        {
          msg = g_strdup_printf(_("The GNOME Session Manager (process %d) has crashed\n"
                                  "due to a fatal error (%s).\n"
                                  "When you close this dialog, all applications will close "
                                  "and your session will exit.\n"
                                  "Please save all your files before closing this dialog."),
                                getppid(), g_strsignal(atoi(args[1])));
          bb_sm_disable = 1;
        }
      else
        {
          msg = g_strdup_printf(_("Application \"%s\" (process %d) has crashed\ndue to a fatal error.\n(%s)"),
                                args[0], getppid(), g_strsignal(atoi(args[1])));
        }
      g_free(base);
      if(args[2])
	app_version = args[2];
    }
  else
    {
      fprintf(stderr, _("Usage: gnome_segv appname signum\n"));
      return 1;
    }
  appname = g_strdup(args[0]);

  mainwin = gtk_message_dialog_new (NULL, GTK_DIALOG_MODAL,
                                    GTK_MESSAGE_ERROR,
                                    GTK_BUTTONS_CLOSE,
                                    msg);


  
  bug_buddy_path = gnome_is_program_in_path ("bug-buddy");
  if (bug_buddy_path != NULL)
    {
      gtk_dialog_add_button(GTK_DIALOG(mainwin),
                            _("Submit a bug report"),
                            RESPONSE_BUG_BUDDY);
    }

  debugger = g_getenv("GNOME_DEBUGGER");
  if (debugger && strlen(debugger)>0)
  {
    debugger_path = gnome_is_program_in_path (debugger);
    if (debugger_path != NULL)
      {
        gtk_dialog_add_button(GTK_DIALOG(mainwin),
                              _("Debug"),
                              RESPONSE_DEBUG);
      }
  }
  
  /* Please download http://www.gnome.org/application_crashed-shtml.txt,
   * translate the plain text, and send the file to webmaster@gnome.org. */
  urlstr = g_strdup_printf("%s?app=%s%s%s&libsver=%s", 
		_("http://www.gnome.org/application_crashed.shtml"),
		args[0],
		app_version?"&version=":"",
		app_version?app_version:"",
		VERSION);
  urlbtn = gnome_href_new(urlstr,
                          _("Please visit the GNOME Application Crash page for more information"));
  gtk_widget_show(urlbtn);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(mainwin)->vbox), urlbtn);

  g_free(msg);

  res = gtk_dialog_run(GTK_DIALOG(mainwin));

  if (res == RESPONSE_BUG_BUDDY && (bug_buddy_path != NULL))
    {
      gchar *exec_str;
      int retval;

      g_assert(bug_buddy_path);
      exec_str = g_strdup_printf("%s --appname=\"%s\" --pid=%d "
                                 "--package-ver=\"%s\" %s", 
                                 bug_buddy_path, appname, getppid(), 
                                 app_version, bb_sm_disable 
                                 ? "--sm-disable" : "");

      retval = system(exec_str);
      g_free(exec_str);
      if (retval == -1 || retval == 127)
        {
          g_warning("Couldn't run bug-buddy: %s", g_strerror(errno));
        }
    }
  else if (res == RESPONSE_DEBUG || (res == RESPONSE_BUG_BUDDY && (bug_buddy_path == NULL)))
    {
      gchar *exec_str;
      int retval;

      g_assert (debugger_path);
      exec_str = g_strdup_printf("%s --appname=\"%s\" --pid=%d "
                                 "--package-ver=\"%s\" %s", 
                                 debugger_path, appname, getppid(), 
                                 app_version, bb_sm_disable 
                                 ? "--sm-disable" : "");

      retval = system(exec_str);
      g_free(exec_str);
      if (retval == -1 || retval == 127)
        {
          g_warning("Couldn't run debugger: %s", g_strerror(errno));
        }
    }

  return 0;
}
