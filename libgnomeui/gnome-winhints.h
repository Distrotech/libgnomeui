/* gnome-winhints.h: Copyright (C) 1998 Free Software Foundation
 * Convenience functions for working with XA_WIN_* hints.
 *
 * Written by: M.Watson <redline@pdq.net>
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
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#ifndef GNOME_WINHINTS_H
#define GNOME_WINHINTS_H

#include <gnome.h>
#include <gtk/gtk.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>

BEGIN_GNOME_DECLS

/* The hints we recognize */
#define XA_WIN_PROTOCOLS       "WIN_PROTOCOLS"
#define XA_WIN_ICONS           "WIN_ICONS"
#define XA_WIN_WORKSPACE       "WIN_WORKSPACE"
#define XA_WIN_WORKSPACE_COUNT "WIN_WORKSPACE_COUNT"
#define XA_WIN_WORKSPACE_NAMES "_WIN_WORKSPACE_NAMES"    
#define XA_WIN_LAYER           "WIN_LAYER"
#define XA_WIN_STATE           "WIN_STATE"
#define XA_WIN_HINTS           "WIN_HINTS"
#define XA_WIN_WORKAREA        "WIN_WORKAREA"

/* flags for the window layer */
#define WinLayerCount          16
#define WinLayerInvalid        0xFFFFFFFFUL
#define WinLayerDesktop        0UL
#define WinLayerBelow          2UL
#define WinLayerNormal         4UL
#define WinLayerOnTop          6UL
#define WinLayerDock           8UL
#define WinLayerAboveDock      10UL

/* flags for the window state */
#define WinStateAllWorkspaces  (1 << 0)   /* appears on all workspaces */
#define WinStateMinimized      (1 << 1)
#define WinStateMaximizedVert  (1 << 2)   /* maximized vertically */
#define WinStateMaximizedHoriz (1 << 3)   /* maximized horizontally */
#define WinStateHidden         (1 << 4)   /* not on taskbar if any, but still accessible */
#define WinStateRollup         (1 << 5)   /* only titlebar visible */
#define WinStateHidWorkspace   (1 << 6)   /* not on current workspace */
#define WinStateHidTransient   (1 << 7)   /* hidden due to invisible owner window */
#define WinStateDockHorizontal (1 << 8)

/* flags for skip hint */
#define WinHintsSkipFocus      (1 << 0)
#define WinHintsSkipWindowMenu (1 << 1)
#define WinHintsSkipTaskBar    (1 << 2)
#define WinHintsGroupTransient (1 << 3)

    
    
/* these are the X atoms for the hints we use */
extern Atom _XA_WIN_WORKSPACE;
extern Atom _XA_WIN_WORKSPACE_COUNT;
extern Atom _XA_WIN_WORKSPACE_NAMES;
extern Atom _XA_WIN_STATE;
extern Atom _XA_WIN_HINTS;
extern Atom _XA_WIN_PROTOCOLS;
extern Atom _XA_WIN_LAYER;

typedef struct _GnomeWinHintsSkip {
    gboolean skipFocus;
    gboolean skipWinMenu;
    gboolean skipWinList;
} GnomeWinHintsSkip;

/* This must be called before any gnome_win_hints_* calls */
void
gnome_win_hints_init();

/* Set the current layer for window */
gboolean
gnome_win_hints_set_layer(GtkWidget *window, gulong layer);

/* Get the current layer for window */
gulong
gnome_win_hints_get_layer(GtkWidget *window);

/* Return a GList of workspace names (char*) */
GList*
gnome_win_hints_get_workspace_list(GtkWidget *window);

/* Return the current number of defined workspaces */
gint
gnome_win_hints_get_workspace_count();

/* Return the number of the currently active  workspace */
gint
gnome_win_hints_get_current_workspace();

/* Set the currently active workspace */
gboolean
gnome_win_hints_set_current_workspace(gint workspace);

/* Return the number of the workspace that window is on*/
gint
gnome_win_hints_get_workspace(GtkWidget *window);

/* set the workspace of window */
gboolean
gnome_win_hints_set_workspace(GtkWidget *window, gint workspace);

/* This is not yet implemented. It seems kind of messy. */
/*
GList*
gnome_win_hints_get_hints(void);
*/

/* Set the skip properties of window. (task[bar,list], focus, window menu) */
gboolean
gnome_win_hints_set_skip(GtkWidget *window, GnomeWinHintsSkip skip);

/* Return the skip properties for window */
GnomeWinHintsSkip
gnome_win_hints_get_skip(GtkWidget *window);

/* Set the state of window */
gboolean
gnome_win_hints_set_state(GtkWidget *window,  gint32 mask, gint32 state);

/* Return the state info of window */
gint32
gnome_win_hints_get_state(GtkWidget *window);


END_GNOME_DECLS

#endif

    