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

#include <libgnome.h>
#include <libgnomeui/gnome-ui-init.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

static char *display = NULL, *languages = NULL;
static int ior_fd = -1;

struct poptOption options[] = {
  {"display", '\0', POPT_ARG_STRING, &display, 0},
  {"ior-fd", '\0', POPT_ARG_INT, &ior_fd, 0},
  {"languages", '\0', POPT_ARG_STRING, &languages, 0},
  {NULL}
};

typedef struct {
  FILE *pipe_fh;
  GMainLoop *ml;
  int pipe_fd, pipe_tag;
} proginfo;

static gboolean handle_commands(GIOChannel *ioc, GIOCondition cond, gpointer data);
static gboolean relay_output   (GIOChannel *ioc, GIOCondition cond, gpointer data);

int main(int argc, char **argv)
{
  proginfo myprog;

  gnome_program_init("gnome-remote-bootstrap", VERSION, &libgnomeui_module_info,
		     argc, argv, GNOME_PARAM_POPT_TABLE, options, NULL);

  if(!display
     || (ior_fd < 0))
    {
      g_printerr("This is program is used internally to bootstrap the network connections for the desktop.\n");
      return 1;
    }

  putenv(g_strconcat("DISPLAY", display));
  if(strcmp(languages, "C"))
    putenv(g_strconcat("LANG", languages));

  memset(&myprog, 0, sizeof(myprog));
  myprog.pipe_fd = -1;

  myprog.ml = g_main_new(FALSE);

  {
    GIOChannel *glib_bites = g_io_channel_unix_new(0); /* stdin */

    g_io_add_watch(glib_bites, G_IO_IN|G_IO_ERR|G_IO_HUP|G_IO_NVAL, handle_commands, &myprog);
    g_io_channel_unref(glib_bites);
  }
  
  fprintf(stdout, "GNOME BOOTSTRAP READY\n");
  fflush(stdout);

  g_main_run(myprog.ml);

  return 0;
}

static gboolean
handle_commands(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
  char aline[4096];
  proginfo *pi = (proginfo *) data;

  if(!(cond & G_IO_IN))
    {
      g_main_quit(pi->ml);
      return FALSE;
    }

  if(!fgets(aline, sizeof(aline), stdin))
    {
      g_main_quit(pi->ml);
      return FALSE;
    }

  g_strstrip(aline);

  if(!strcmp(aline, "DONE"))
    {
      g_main_quit(pi->ml);
      return FALSE;
    }
  else if(!strncmp(aline, "RUN ", strlen("RUN ")))
    {
      int pipes[2];
      int childpid;
      char **pieces;
      int i;

      if(pipe(pipes))
	goto err;

      dup2(pipes[1], ior_fd);
      pieces = g_strsplit(aline + strlen("RUN "), " ", -1);
      for(i = 0; pieces[i]; i++) /**/;

      childpid = gnome_execute_async_fds(NULL, i, pieces, FALSE);

      if(childpid < 0)
	{
	  close(pipes[1]);
	  close(pipes[0]);
	  close(ior_fd);
	  goto err;
	}
      close(pipes[1]);
      pi->pipe_fd = pipes[0];
      pi->pipe_fh = fdopen(pi->pipe_fd, "r");
      {
	GIOChannel *glib_bites = g_io_channel_unix_new(pi->pipe_fd);

	pi->pipe_tag = g_io_add_watch(glib_bites, G_IO_IN|G_IO_ERR|G_IO_HUP|G_IO_NVAL, relay_output, pi);
	g_io_channel_unref(glib_bites);
      }

      return TRUE;
    err:
      fprintf(stdout, "ERROR\n");
      fflush(stdout);
      return TRUE;
    }

  return TRUE;
}

static gboolean
relay_output(GIOChannel *ioc, GIOCondition cond, gpointer data)
{
  char aline[4096];
  proginfo *pi = (proginfo *) data;

  if(!(cond & G_IO_IN))
    goto err;

  if(!fgets(aline, sizeof(aline), pi->pipe_fh))
    goto err;

  fprintf(stdout, "OUTPUT %s%s", aline,
	  (aline[strlen(aline) - 1] == '\n')?"":"\n");
  fflush(stdout);

  return TRUE;

 err:
  g_source_remove(pi->pipe_tag);
  fclose(pi->pipe_fh);
  return FALSE;
}
