/* gnome-less.c: Copyright (C) 1998 Free Software Foundation
 * Written by: Havoc Pennington
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

#include "gnome-less.h"

#include <config.h>

#include "libgnome/gnome-util.h"

#include "libgnome/gnome-i18nP.h"

#include "libgnomeui/gnome-uidefs.h"

#include <unistd.h>
#include <time.h>
#include <string.h> /* memset */
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


static void gnome_less_class_init (GnomeLessClass *klass);
static void gnome_less_init       (GnomeLess      *messagebox);

static void gnome_less_destroy (GtkObject *gl);

static gint text_clicked_cb(GtkText * text, GdkEventButton * e, 
			    GnomeLess * gl);

static GtkWindowClass *parent_class;

guint
gnome_less_get_type ()
{
  static guint gl_type = 0;

  if (!gl_type)
    {
      GtkTypeInfo gl_info =
      {
	"GnomeLess",
	sizeof (GnomeLess),
	sizeof (GnomeLessClass),
	(GtkClassInitFunc) gnome_less_class_init,
	(GtkObjectInitFunc) gnome_less_init,
	(GtkArgSetFunc) NULL,
	(GtkArgGetFunc) NULL,
      };

      gl_type = gtk_type_unique (gtk_vbox_get_type (), &gl_info);
    }

  return gl_type;
}

static void
gnome_less_class_init (GnomeLessClass *klass)
{
  GtkObjectClass *object_class;

  object_class = (GtkObjectClass*) klass;

  parent_class = gtk_type_class (gtk_vbox_get_type ());

  object_class->destroy = gnome_less_destroy;
}

static void
gnome_less_init (GnomeLess *gl)
{
  GtkWidget * vscroll;
  GtkWidget * hbox; /* maybe just inherit from hbox? hard to say yet. */
  GtkWidget * mi;

  gl->text = GTK_TEXT(gtk_text_new(NULL, NULL));

  gl->font = NULL;

  gl->popup = GTK_MENU(gtk_menu_new());
  mi = gtk_menu_item_new_with_label("Doesn't do anything yet");
  gtk_menu_append(gl->popup, mi);
  gtk_widget_show(mi);

  gtk_widget_set_events(GTK_WIDGET(gl->text), GDK_BUTTON_PRESS_MASK);
  gtk_signal_connect(GTK_OBJECT(gl->text), "button_press_event",
		     GTK_SIGNAL_FUNC(text_clicked_cb), gl);

  hbox = gtk_hbox_new(FALSE, 0);
  vscroll = gtk_vscrollbar_new(gl->text->vadj);
  
  gtk_box_pack_start(GTK_BOX(hbox), GTK_WIDGET(gl->text),
		     TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), vscroll, FALSE, FALSE, 0);

  /* Since horizontal scroll doesn't work, use this hack. */
  gtk_widget_set_usize(GTK_WIDGET(gl->text), 300, -1); 

  gtk_widget_show_all(hbox);

  gtk_container_add(GTK_CONTAINER(gl), hbox);
}

/**
 * gnome_less_new
 *
 * Creates a new GnomeLess widget.
 * 
 * Returns:
 * &GtkWidget pointer to a new GNOME less widget
 **/

GtkWidget* gnome_less_new (void)
{
  GnomeLess * gl;
  
  gl = gtk_type_new(gnome_less_get_type());

  return GTK_WIDGET (gl);
}

static void gnome_less_destroy (GtkObject *gl)
{
  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));

  if (GNOME_LESS(gl)->font) gdk_font_unref(GNOME_LESS(gl)->font);

  if (GTK_OBJECT_CLASS(parent_class)->destroy)
    (* (GTK_OBJECT_CLASS(parent_class)->destroy))(gl);
}

/**
 * gnome_less_show_file
 * @gl: Pointer to GnomeLess widget
 * @path: Pathname of file to be displayed
 *
 * Displays a file in a GnomeLess widget. Replaces any text already being displayed
 * in the widget.
 *
 * Returns:
 * %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/

gboolean gnome_less_show_file(GnomeLess * gl, const gchar * path)
{
  FILE * f;

  g_return_val_if_fail(gl != NULL, FALSE);
  g_return_val_if_fail(GNOME_IS_LESS(gl), FALSE);
  g_return_val_if_fail(path != NULL, FALSE);

  if ( ! g_file_exists(path) ) {
    /* Leave errno from the stat in g_file_exists */
    return FALSE;
  }

  f = fopen(path, "rt");

  if ( f == NULL ) {
    /* Leave errno */
    return FALSE;
  }

  if ( ! gnome_less_show_filestream(gl, f) ) {
    /* Starting to feel like exceptions would be nice. */
    int save_errno = errno;
    fclose(f); /* nothing to do if it fails */
    errno = save_errno;  /* Want to report the root cause of the error */
    return FALSE;
  }

  if (fclose(f) != 0) {
    return FALSE;
  }
  else return TRUE;
}

/**
 * gnome_less_show_command
 * @gl: Pointer to GnomeLess widget
 * @command_line: Command to be executed
 *
 * Runs the shell command specified in @command_line, and places the output of that command
 * in the GnomeLess widget specified by @gl. Replaces any text already being displayed in the
 * widget.
 *
 * Returns:
 * %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/

gboolean gnome_less_show_command(GnomeLess * gl,
				 const gchar * command_line)
{
  FILE * p;

  g_return_val_if_fail(gl != NULL, FALSE);
  g_return_val_if_fail(GNOME_IS_LESS(gl), FALSE);
  g_return_val_if_fail(command_line != NULL, FALSE);
  
  p = popen(command_line, "r");

  if ( p == NULL ) {
    return FALSE;
  }

  if ( ! gnome_less_show_filestream(gl, p) ) {
    int save_errno = errno;
    pclose(p); /* nothing to do if it fails */
    errno = save_errno; /* Report the root cause of the error */
    return FALSE;
  }

  if ( pclose(p) == -1 ) {
    return FALSE;
  }
  else return TRUE;
}

#define GLESS_BUFSIZE 1024

/**
 * gnome_less_show_filestream
 * @gl: Pointer to GnomeLess widget
 * @f: Filestream to be displayed in the widget
 *
 * Reads all of the text from filestream @f, and places it in the GnomeLess widget @gl. Replaces any text
 * already being displayed.
 *
 * Returns:
 * %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/

gboolean gnome_less_show_filestream(GnomeLess * gl, FILE * f)
{
  gchar buffer [GLESS_BUFSIZE];
  gchar * s;

  g_return_val_if_fail(gl != NULL, FALSE);
  g_return_val_if_fail(GNOME_IS_LESS(gl), FALSE);
  g_return_val_if_fail(f != NULL, FALSE);

  gtk_text_freeze(gl->text);
  
  gnome_less_clear(gl);
  
  errno = 0; /* Reset it to detect errors */
  while (TRUE) {
    s = fgets(buffer, GLESS_BUFSIZE, f);

    if ( s == NULL ) break;
    
    gtk_text_insert(gl->text, gl->font, NULL, NULL, buffer, strlen(s));
  }

  gtk_text_thaw(gl->text);

  if ( errno != 0 ) {
    /* We quit on an error, not EOF */
    return FALSE;
  }
  else {
    return TRUE;
  }
}

/**
 * gnome_less_show_fd
 * @gl: Pointer to GnomeLess widget
 * @file_descriptor: Filestream to be displayed in the widget
 *
 * Reads all of the text from file descriptor @file_descriptor, and places it in the GnomeLess widget @gl.
 * Replaces any text already being displayed.
 *
 * Returns:
 * %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/

gboolean gnome_less_show_fd         (GnomeLess * gl, int file_descriptor)
{
  FILE * f;

  g_return_val_if_fail(gl != NULL, FALSE);

  f = fdopen(file_descriptor, "r");

  if ( f && gnome_less_show_filestream(gl, f) ) {
    return TRUE;
  }
  else return FALSE;
}

static void gnome_less_append_string(GnomeLess * gl, const gchar * s)
{
  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));
  g_return_if_fail(s != NULL);

  gtk_text_set_point(gl->text, gtk_text_get_length(gl->text));
  gtk_text_insert(gl->text, gl->font, NULL, NULL, s, strlen(s)); 
}

/**
 * gnome_less_show_string
 * @gl: Pointer to GnomeLess widget
 * @s: String to be displayed
 *
 * Displays a string in the GnomeLess widget @gl. Replaces any text
 * already being displayed.
 *
 **/
 
void gnome_less_show_string(GnomeLess * gl, const gchar * s)
{
  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));
  g_return_if_fail(s != NULL);

  gtk_text_freeze(gl->text);
  gnome_less_clear(gl);
  gnome_less_append_string(gl, s);
  gtk_text_thaw(gl->text);
}

/**
 * gnome_less_clear
 * @gl: Pointer to GnomeLess widget
 *
 * Clears all text from GnomeLess widget @gl.
 **/

void gnome_less_clear (GnomeLess * gl)
{
  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));

  gtk_editable_delete_text(GTK_EDITABLE(gl->text), 0,
			   gtk_text_get_length(GTK_TEXT(gl->text)));
}

/**
 * gnome_less_set_font
 * @gl: Pointer to GnomeLess widget
 * @font: Pointer to GdkFont
 *
 * Sets the font of the text to be displayed in the GnomeLess widget @gl to @font.
 *
 * Note: This will not affect text already being displayed.
 * If you use this function after adding text to the widget, you must show it again
 * by using #gnome_less_reshow or one of the gnome_less_show commands.
 **/

void gnome_less_set_font(GnomeLess * gl, GdkFont * font)
{
  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));

  /* font is allowed to be NULL */

  if (gl->font) gdk_font_unref(gl->font);
  gl->font = font;
  if (gl->font) {
    gdk_font_ref(gl->font);
  }
}

/**
 * gnome_less_fixed_font
 * @gl: Pointer to GnomeLess widget
 *
 * This function is obsolete. Please use #gnome_less_set_fixed_font instead.
 **/

void gnome_less_fixed_font(GnomeLess * gl)
{
  g_warning("Please use gnome_less_set_fixed_font instead. Sorry!\n");
  gnome_less_set_fixed_font(gl, TRUE);
}

/**
 * gnome_less_set_fixed_font
 * @gl: Pointer to GNOME Less widget
 * @fixed: Whether or not to use a fixed font
 *
 * Specifies whether or not new text should be displayed using a fixed font. Pass TRUE
 * in @fixed to use a fixed font, or FALSE to revert to the default GtkText font.
 *
 * Note: This will not affect text already being displayed.
 * If you use this function after adding text to the widget, you must show it again
 * by using #gnome_less_reshow or one of the gnome_less_show commands.
 **/
  
void gnome_less_set_fixed_font  (GnomeLess * gl, gboolean fixed)
{
  GdkFont * font;
  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));

  if ( fixed == FALSE ) {
    font = NULL; /* Note that a NULL string to gtk_text_insert_text 
		    uses the default font. */
  }
  else {
    /* FIXME maybe try loading a nicer font first? */
    /* Is this a standard X font in every respect? */
    /* I'm told that "fixed" is the standard X font, but
       "fixed" doesn't appear to be fixed-width; courier does though. */
    font = gdk_fontset_load(
	"-*-courier-medium-r-normal-*-12-*-*-*-*-*-*-*,"
	"-*-*-medium-r-normal-*-12-*-*-*-*-*-*-*,*");
    
    if ( font == NULL ) {
      g_warning("GnomeLess: Couldn't load fixed font\n");
      return;
    }
  }

  gnome_less_set_font(gl, font);  
}

/**
 * gnome_less_write_fd 
 * @gl: Pointer to GnomeLess widget
 * @fd: File descriptor
 *
 * Writes the text displayed in the GnomeLess widget @gl to file descriptor @fd.
 *
 * Returns: %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/

gboolean gnome_less_write_fd (GnomeLess * gl, int fd)
{
  gchar * contents;
  gint len;
  gint bytes_written;

  g_return_val_if_fail(gl != NULL, FALSE);
  g_return_val_if_fail(fd >= 0, FALSE);

  contents = 
    gtk_editable_get_chars(GTK_EDITABLE(gl->text), 0,
			   gtk_text_get_length(GTK_TEXT(gl->text)));
  len = strlen(contents);

  if ( contents && (len > 0)  ) {
    bytes_written = write (fd, contents, len);
    g_free(contents); 
    
    if ( bytes_written != len ) return FALSE;
    else return TRUE;
  }
  else return TRUE; /* Nothing to write. */
}

/**
 * gnome_less_write_file
 * @gl: Pointer to GnomeLess widget
 * @path: Path of file to be written
 *
 * Writes the text displayed in the GnomeLess widget @gl to the file specified by @path.
 *
 * Returns: %TRUE if successful, %FALSE if not. Error stored in %errno.
 **/

gboolean gnome_less_write_file   (GnomeLess * gl, const gchar * path)
{
  int fd;

  g_return_val_if_fail(gl != NULL, FALSE);
  g_return_val_if_fail(path != NULL, FALSE);
  
  fd = open( path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR );

  if ( fd == -1 ) return FALSE; /* Leaving errno set */

  if ( ! gnome_less_write_fd(gl, fd) ) {
    int save_errno = errno;
    close(fd); /* If it fails, nothing to do */
    errno = save_errno; /* Return root cause of problem. */
    return FALSE;
  }

  if ( close(fd) != 0 ) {
    /* Error, leave errno and return */
    return FALSE;
  }
  else return TRUE;
}

/**
 * gnome_less_reshow
 * @gl: Pointer to GnomeLess widget
 *
 * Re-displays all of the text in the GnomeLess widget @gl. If the font has changed since
 * the last show/reshow of text, it will update the current text to the new font.
 **/
 
void gnome_less_reshow          (GnomeLess * gl)
{
  gchar * contents;

  g_return_if_fail(gl != NULL);

  contents = gtk_editable_get_chars(GTK_EDITABLE(gl->text), 0,
				    gtk_text_get_length(GTK_TEXT(gl->text)));

  if ( contents && (strlen(contents) != 0)  ) {
    gnome_less_show_string(gl, contents);
  }

  g_free(contents);
}

static gint text_clicked_cb(GtkText * text, GdkEventButton * e, 
			    GnomeLess * gl)
{
  if (e->button == 1) {
    /* Ignore button 1 */
    return TRUE; 
  }

  /* don't change the selection. */
  gtk_signal_emit_stop_by_name (GTK_OBJECT (text), "button_press_event");

  gtk_menu_popup(gl->popup, NULL, NULL, NULL,
                 NULL, e->button, time(NULL));
  return TRUE; 
}
