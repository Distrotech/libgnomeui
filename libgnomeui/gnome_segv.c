/* -*- Mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; c-basic-offset: 2; -*- */
#include <config.h>

/* needed for sigaction and friends under 'gcc -ansi -pedantic' on 
 * GNU/Linux */
#ifndef _POSIX_SOURCE
#  define _POSIX_SOURCE 1
#endif
#include <sys/types.h>

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
  char **args;

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
      msg = g_strdup_printf(_("Application \"%s\" (process %d) has crashed\ndue to a fatal error.\n(%s)"),
                            args[0], getppid(), g_strsignal(atoi(args[1])));
    }
  else
    {
      fprintf(stderr, _("Usage: gnome_segv appname signum\n"));
      return 1;
    }

  poptFreeContext(ctx);

  mainwin = gnome_message_box_new(msg,
                                  GNOME_MESSAGE_BOX_ERROR,
                                  GNOME_STOCK_BUTTON_CLOSE,
                                  NULL);

  urlbtn = gnome_href_new("http://www.gnome.org/application_crashed.shtml",
                          _("Please visit the GNOME Application Crash page for more information"));
  gtk_widget_show(urlbtn);
  gtk_container_add(GTK_CONTAINER(GNOME_DIALOG(mainwin)->vbox), urlbtn);

  g_free(msg);

  gnome_dialog_run(GNOME_DIALOG(mainwin));

  return 0;
}
