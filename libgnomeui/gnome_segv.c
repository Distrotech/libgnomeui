/* -*- Mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
#include <config.h>

/* needed for sigaction and friends under 'gcc -ansi -pedantic' on 
 * GNU/Linux */
#ifndef _POSIX_SOURCE
#  define _POSIX_SOURCE 1
#endif
#include <sys/types.h>
#include <sys/wait.h>

#include <stdlib.h>
#include <unistd.h>
#include <gnome.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>

int retval = 1;
pid_t bug_buddy_pid = -1;

gchar *bug_buddy_path;
gchar *appname;

gint bug_buddy_cb(GtkWidget *button, gpointer data)
{
  gchar *envv[3] = { NULL };
  gchar *argv[2] = { NULL };

  argv[0] = bug_buddy_path;
  envv[0] = g_strdup_printf("BB_APPNAME=%s", appname);
  envv[1] = g_strdup_printf("BB_PID=%d", getppid());

  bug_buddy_pid = gnome_execute_async_with_env (NULL, 
                                                1, argv,
                                                2, envv);
  return FALSE;
}

int main(int argc, char *argv[])
{
  GtkWidget *mainwin, *urlbtn;
  gchar* msg;
  struct sigaction sa;
  poptContext ctx;
  const char **args;

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
  
  bug_buddy_path = gnome_is_program_in_path ("bug-buddy");
  if (bug_buddy_path != NULL)
    {
      gnome_dialog_append_button(GNOME_DIALOG(mainwin),
                                 _("Submit a bug report"));
      gnome_dialog_button_connect(GNOME_DIALOG(mainwin), 1,
                                  GTK_SIGNAL_FUNC(bug_buddy_cb),
                                  NULL);
    }
  /* Please download http://www.gnome.org/application_crashed-shtml.txt,
   * translate the plain text, and send the file to webmaster@gnome.org. */
  urlbtn = gnome_href_new(_("http://www.gnome.org/application_crashed.shtml"),
                          _("Please visit the GNOME Application Crash page for more information"));
  gtk_widget_show(urlbtn);
  gtk_container_add(GTK_CONTAINER(GNOME_DIALOG(mainwin)->vbox), urlbtn);

  g_free(msg);

  gnome_dialog_run(GNOME_DIALOG(mainwin));

  if (bug_buddy_pid != -1)
    waitpid(bug_buddy_pid, NULL, 0);

  return 0;
}
