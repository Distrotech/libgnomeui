/* -*- Mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
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
#include <gnome.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

int retval = 1;

int main(int argc, char *argv[])
{
  GtkWidget *mainwin, *urlbtn;
  gchar* msg;
  struct sigaction sa;
  poptContext ctx;
  const char **args;
  int res;
  gchar *appname;
  gchar *bug_buddy_path = NULL;

  /* We do this twice to make sure we don't start running ourselves... :) */
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sigaction(SIGSEGV, &sa, NULL);


  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
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
      if (strcmp(args[0], "gnome-session") == 0)
        {
          msg = g_strdup_printf(_("The GNOME Session Manager (process %d) has crashed\ndue to a fatal error (%s).\nWhen you close this dialog, all applications will close and your session will exit.\nPlease save all your files before closing this dialog."),
                                getppid(), g_strsignal(atoi(args[1])));
        }
      else
        {
          msg = g_strdup_printf(_("Application \"%s\" (process %d) has crashed\ndue to a fatal error.\n(%s)"),
                                args[0], getppid(), g_strsignal(atoi(args[1])));
        }
    }
  else
    {
      fprintf(stderr, _("Usage: gnome_segv appname signum\n"));
      return 1;
    }
  appname = g_strdup(args[0]);
  poptFreeContext(ctx);

  mainwin = gnome_message_box_new(msg,
                                  GNOME_MESSAGE_BOX_ERROR,
                                  GNOME_STOCK_BUTTON_CLOSE,
                                  NULL);
  
  /* so we don't try to debug bug-buddy */
  if (getenv("BB_APPNAME") == NULL)
    {
      bug_buddy_path = gnome_is_program_in_path ("bug-buddy");
      if (bug_buddy_path != NULL)
        gnome_dialog_append_button(GNOME_DIALOG(mainwin),
                                   _("Submit a bug report"));
    }
  /* Please download http://www.gnome.org/application_crashed-shtml.txt,
   * translate the plain text, and send the file to webmaster@gnome.org. */
  urlbtn = gnome_href_new(_("http://www.gnome.org/application_crashed.shtml"),
                          _("Please visit the GNOME Application Crash page for more information"));
  gtk_widget_show(urlbtn);
  gtk_container_add(GTK_CONTAINER(GNOME_DIALOG(mainwin)->vbox), urlbtn);

  g_free(msg);

  res = gnome_dialog_run(GNOME_DIALOG(mainwin));

  if (res == 1)
    {
      gchar *env_str;
      env_str = g_strdup_printf("BB_APPNAME=%s", appname);
      putenv(env_str);
      
      env_str = g_strdup_printf("BB_PID=%d", getppid());
      putenv(env_str);

      g_assert (bug_buddy_path);
      system(bug_buddy_path);
    }

  return 0;
}
