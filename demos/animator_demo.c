#include <config.h>
#include <gnome.h>

#define ANIMFILE "mailcheck/email.png"
/* #define ANIMFILE "gnome-ppp-animation-large.png" */

static void quit_cb (GtkWidget * widget, void *data);
static void toggle_start_stop_cb (GtkWidget * widget, void *data);

static void
quit_cb (GtkWidget * widget, void *data)
{
  gtk_main_quit ();
}

static void
toggle_start_stop_cb (GtkWidget * widget, gpointer data)
{
  GtkWidget *tmp;
  GnomeAnimatorStatus status;

  tmp = (GtkWidget *) data;

  status = gnome_animator_get_status (GNOME_ANIMATOR (tmp));
  if (status == GNOME_ANIMATOR_STATUS_STOPPED)
    gnome_animator_start (GNOME_ANIMATOR (tmp));
  else
    gnome_animator_stop (GNOME_ANIMATOR (tmp));
}

int
main (int argc, char *argv[])
{
  GtkWidget *window;
  GtkWidget *button;
  GtkWidget *animator;
  GdkPixbuf *pixbuf;
  char *s;

  gnome_program_init ("gnome-animator", VERSION, &libgnomeui_module_info,
		      argc, argv, NULL);

  window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_realize (window);
  gtk_signal_connect (GTK_OBJECT (window), "delete_event",
		      GTK_SIGNAL_FUNC (quit_cb), NULL);

  s = gnome_program_locate_file (gnome_program_get (),
				 GNOME_FILE_DOMAIN_PIXMAP,
				 ANIMFILE, TRUE, NULL);
  pixbuf = gdk_pixbuf_new_from_file (s, NULL);
  g_free (s);
  animator = gnome_animator_new_with_size (48, 48);
  gnome_animator_append_frames_from_pixbuf (GNOME_ANIMATOR (animator),
					    pixbuf, 0, 0, 150, 48);
  gdk_pixbuf_unref (pixbuf);
/*  gnome_animator_set_range (GNOME_ANIMATOR (animator), 1, 21); */
  gnome_animator_set_loop_type (GNOME_ANIMATOR (animator),
				GNOME_ANIMATOR_LOOP_RESTART);
  gnome_animator_set_playback_speed (GNOME_ANIMATOR (animator), 5.0);

  button = gtk_button_new ();
  gtk_container_add (GTK_CONTAINER (button), animator);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      GTK_SIGNAL_FUNC (toggle_start_stop_cb), animator);

  gtk_container_add (GTK_CONTAINER (window), button);
  gtk_widget_show_all (window);

  gnome_animator_start (GNOME_ANIMATOR (animator));

  gtk_main ();

  return 0;
}
