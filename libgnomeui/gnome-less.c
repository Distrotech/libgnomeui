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

void gnome_less_clear (GnomeLess * gl)
{
  g_return_if_fail(gl != NULL);
  g_return_if_fail(GNOME_IS_LESS(gl));

  gtk_editable_delete_text(GTK_EDITABLE(gl->text), 0,
			   gtk_text_get_length(GTK_TEXT(gl->text)));
}

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

void gnome_less_fixed_font(GnomeLess * gl)
{
  g_warning("Please use gnome_less_set_fixed_font instead. Sorry!\n");
  gnome_less_set_fixed_font(gl, TRUE);
}

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
	"-*-courier-medium-r-normal-*-12-*-*-*-*-*-*-*"
	"-*-*-medium-r-normal-*-12-*-*-*-*-*-*-*,*");
    
    if ( font == NULL ) {
      g_warning("GnomeLess: Couldn't load fixed font\n");
      return;
    }
  }

  gnome_less_set_font(gl, font);  
}

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



