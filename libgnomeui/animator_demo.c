/* Simple demonstration for the `GnomeAnimator' widget.  For now, it
   requires the `gnome-default.png', `mailcheck/tux-anim.xpm' and
   `mailcheck/email.xpm' icons to be installed in the GNOME pixmap
   directory (these should be in the `gnome-core' module).  */

#include <config.h>
#include <gnome.h>

static GtkWidget *the_app;
static GtkWidget *the_button;
static GtkWidget *the_animator;
static GtkWidget *the_second_animator;
static GtkWidget *the_third_animator;
static GtkWidget *the_box;

static void quit_cb (GtkWidget *widget, void *data);
static void toggle_start_stop_cb (GtkWidget *widget, void *data);

static void
quit_cb (GtkWidget *widget, void *data)
{
  gtk_main_quit ();
}

static void
toggle_start_stop_cb (GtkWidget *widget, void *data)
{
  GnomeAnimatorStatus status;

  status = gnome_animator_get_status (GNOME_ANIMATOR (the_animator));
  if (status == GNOME_ANIMATOR_STATUS_STOPPED)
    gnome_animator_start (GNOME_ANIMATOR (the_animator));
  else
    gnome_animator_stop (GNOME_ANIMATOR (the_animator));
}

int
main (int argc, char *argv[])
{
  GtkWidget *tmp;
  char *s;

  gnome_init ("gnome-animator", VERSION, argc, argv);

  the_app = gnome_app_new ("gnome-animator", "Test!");
  gtk_widget_realize (the_app);

  gtk_signal_connect (GTK_OBJECT (the_app), "delete_event",
                      GTK_SIGNAL_FUNC (quit_cb), NULL);

  the_box = gtk_vbox_new (TRUE, 0);

  the_animator = gnome_animator_new_with_size (100, 100);

  /* The following code loads the default GNOME foot and creates a
     simple animation by changing its size frame-by-frame.  */
  
  /* Warning: This is slow, and should be avoided in real programs
     with long animations.  Load the file once with Imlib and use
     `gnome_animator_append_from_imlib_at_size()' instead.  */
  s = gnome_pixmap_file ("gnome-default.png");
  gnome_animator_append_frame_from_file_at_size (GNOME_ANIMATOR (the_animator),
                                                 s, 36, 36, 200, 24, 24);
  gnome_animator_append_frame_from_file_at_size (GNOME_ANIMATOR (the_animator),
                                                 s, 30, 30, 50, 36, 36);
  gnome_animator_append_frame_from_file_at_size (GNOME_ANIMATOR (the_animator),
                                                 s, 28, 28, 50, 40, 40);
  gnome_animator_append_frame_from_file_at_size (GNOME_ANIMATOR (the_animator),
                                                 s, 24, 24, 50, 48, 48);
  gnome_animator_append_frame_from_file_at_size (GNOME_ANIMATOR (the_animator),
                                                 s, 23, 23, 50, 51, 51);
  gnome_animator_append_frame_from_file_at_size (GNOME_ANIMATOR (the_animator),
                                                 s, 17, 17, 50, 62, 62);
  gnome_animator_append_frame_from_file_at_size (GNOME_ANIMATOR (the_animator),
                                                 s, 9, 9, 50, 78, 78);
  gnome_animator_append_frame_from_file_at_size (GNOME_ANIMATOR (the_animator),
                                                 s, 3, 3, 50, 90, 90);
  gnome_animator_append_frame_from_file_at_size (GNOME_ANIMATOR (the_animator),
                                                 s, -2, -2, 50, 100, 100);
  gnome_animator_append_frame_from_file_at_size (GNOME_ANIMATOR (the_animator),
                                                 s, -12, -12, 50, 120, 120);
  gnome_animator_append_frame_from_file_at_size (GNOME_ANIMATOR (the_animator),
                                                 s, -27, -27, 50, 150, 150);
  gnome_animator_append_frame_from_file_at_size (GNOME_ANIMATOR (the_animator),
                                                 s, -52, -52, 50, 200, 200);
  gnome_animator_append_frame_from_file_at_size (GNOME_ANIMATOR (the_animator),
                                                 s, -102, -102, 100, 300, 300);
  g_free (s);

  /* This puts the foot animation into a GtkButton, to demonstrate how
     the animator can behave (almost) like a shaped windows without
     actually being one.  If we used a real shaped window, the
     animation would be a lot less smooth.  */
  /* Also notice that the window size is never larger than the
     specified size: the foot is always clipped to fit into this size,
     even if the button becomes larger.  */
  the_button = gtk_button_new ();
  gtk_container_set_border_width (GTK_CONTAINER (the_button), 4);
  {
    tmp = gtk_hbox_new (TRUE, 0);
    gtk_container_add (GTK_CONTAINER (the_button), tmp);
    gtk_box_pack_start (GTK_BOX (tmp), the_animator, FALSE, TRUE, 2);
    gtk_widget_show (tmp);
  }

  /* Clicking on the foot starts/stops its animation.  */
  gtk_box_pack_start (GTK_BOX (the_box), the_button, FALSE, TRUE, 0);
  gtk_signal_connect (GTK_OBJECT (the_button), "clicked",
                      GTK_SIGNAL_FUNC (toggle_start_stop_cb), NULL);

  the_second_animator = gnome_animator_new_with_size (100, 100);

#if 0
  /* This is the mandatory "jumping Tux" animation.  The animation is
     extracted from a large image, made up by tiling the frames
     horizontally.  The frames are quite small, so we get a chance to
     show how easy it is to magnify them with
     `gnome_animator_append_frames_from_file_at_size()'.  */
  s = gnome_pixmap_file ("mailcheck/tux-anim.png");
  gnome_animator_append_frames_from_file_at_size (GNOME_ANIMATOR
                                                    (the_second_animator),
                                                  s,
                                                  0, 0, 300, 48, 100, 100);
  g_free (s);

  gtk_box_pack_start (GTK_BOX (the_box), the_second_animator, FALSE, TRUE, 0);

#endif

  the_third_animator = gnome_animator_new_with_size (48, 48);

  /* ...And this is another animation, similiar to the Tux one, but
     without magnification and no shape.  */
  s = gnome_pixmap_file ("mailcheck/email.png");
  gnome_animator_append_frames_from_file (GNOME_ANIMATOR (the_third_animator),
                                          s, 0, 0, 150, 48);
  g_free (s);

  gtk_box_pack_start (GTK_BOX (the_box), the_third_animator, FALSE, FALSE, 0);

  gnome_app_set_contents (GNOME_APP (the_app), the_box);

  gtk_widget_show (the_animator);

  gtk_widget_show (the_button);
  
  /*  gtk_widget_show (the_second_animator); */

  gtk_widget_show (the_third_animator);

  gtk_widget_show (the_app);

  gnome_animator_set_loop_type (GNOME_ANIMATOR (the_animator),
                                GNOME_ANIMATOR_LOOP_PING_PONG);
  gnome_animator_start (GNOME_ANIMATOR (the_animator));

  gnome_animator_set_loop_type (GNOME_ANIMATOR (the_second_animator),
                                GNOME_ANIMATOR_LOOP_RESTART);
  /* This makes the animation twice as fast than the specified
     interval values.  This feature is useful in case one wants to
     increase the speed of the animation according to some custom
     parameter, without having to re-create a new animation.  */
  gnome_animator_set_playback_speed (GNOME_ANIMATOR (the_second_animator),
                                     2.0);
  gnome_animator_start (GNOME_ANIMATOR (the_second_animator));

  gnome_animator_set_loop_type (GNOME_ANIMATOR (the_third_animator),
                                GNOME_ANIMATOR_LOOP_PING_PONG);
  gnome_animator_set_playback_speed (GNOME_ANIMATOR (the_third_animator),
                                     1.0);
  gnome_animator_start (GNOME_ANIMATOR (the_third_animator));

  gtk_main ();

  return 0;
}
