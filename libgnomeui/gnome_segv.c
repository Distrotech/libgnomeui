#include <config.h>
#include <unistd.h>
#include <gnome.h>
#include <signal.h>
#include <string.h>

int retval = 1;

void button_click(GtkWidget *button, int choice)
{
  retval = choice;
  gtk_main_quit();
}

gboolean delete_event(GtkWidget *awin)
{
  retval = 1;
  gtk_main_quit();
  return TRUE; /* Piece of crap Gtk. Return values from signal handlers
		  work shoddily */
}

int main(int argc, char *argv[])
{
  GtkWidget *mainwin, *btn, *lbl;
  GString *mystr;
  int firstarg;
  struct sigaction sa;

  /* We do this twice to make sure we don't start running ourselves... :) */
  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sigaction(SIGSEGV, &sa, NULL);


  bindtextdomain (PACKAGE, GNOMELOCALEDIR);
  textdomain (PACKAGE);
  gnome_init("gnome_segv", NULL, 1, argv, 0, &firstarg);

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = SIG_IGN;
  sigaction(SIGSEGV, &sa, NULL);

  mainwin = gtk_dialog_new();

  btn = gnome_stock_button(GNOME_STOCK_BUTTON_YES);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(mainwin)->action_area),
		    btn);
  gtk_signal_connect(GTK_OBJECT(btn), "clicked",
		     GTK_SIGNAL_FUNC(button_click), (gpointer)0);

  btn = gnome_stock_button(GNOME_STOCK_BUTTON_NO);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(mainwin)->action_area),
		    btn);
  gtk_signal_connect(GTK_OBJECT(btn), "clicked",
		     GTK_SIGNAL_FUNC(button_click), (gpointer)1);

  btn = gtk_button_new_with_label(_("No, and ignore\nfuture SIGSEGV's"));
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(mainwin)->action_area),
		    btn);
  gtk_signal_connect(GTK_OBJECT(btn), "clicked",
		     GTK_SIGNAL_FUNC(button_click), (gpointer)99);

  gtk_box_set_homogeneous(GTK_BOX(GTK_DIALOG(mainwin)->action_area),
			  TRUE);

  mystr = g_string_new(NULL);
  g_string_sprintf(mystr, _("Application \"%s\" has a bug.\nSIGSEGV received at PC %#lx in PID %d.\n\n\nDo you want to exit this program?"),
		   argv[firstarg], atol(argv[firstarg + 1]), getppid());
  lbl = gtk_label_new(mystr->str);
  gtk_container_add(GTK_CONTAINER(GTK_DIALOG(mainwin)->vbox),
		    lbl);

  gtk_widget_show_all(mainwin);

  gtk_main();

  return retval;
}
