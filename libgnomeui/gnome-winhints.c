#include <config.h>
#include <X11/Xlib.h>
#include <X11/Xmd.h>
#include <X11/Xatom.h>
#include <gdk/gdkx.h>
#include <gdk/gdkprivate.h>
#include "gnome-winhints.h"

/* these are the X atoms for the hints we use */
Atom _XA_WIN_WORKSPACE;
Atom _XA_WIN_WORKSPACE_COUNT;
Atom _XA_WIN_WORKSPACE_NAMES;
Atom _XA_WIN_STATE;
Atom _XA_WIN_HINTS;
Atom _XA_WIN_PROTOCOLS;
Atom _XA_WIN_LAYER;

void
gnome_win_hints_init()
{
    /* Get the atoms we are working with, creating them if necessary.
     */
    _XA_WIN_STATE = XInternAtom(GDK_DISPLAY(), XA_WIN_STATE, False);
    _XA_WIN_WORKSPACE = XInternAtom(GDK_DISPLAY(), XA_WIN_WORKSPACE, False);
    _XA_WIN_WORKSPACE_COUNT = XInternAtom(GDK_DISPLAY(), XA_WIN_WORKSPACE_COUNT, False);
    _XA_WIN_WORKSPACE_NAMES = XInternAtom(GDK_DISPLAY(), XA_WIN_WORKSPACE_NAMES, False);
    _XA_WIN_LAYER = XInternAtom(GDK_DISPLAY(), XA_WIN_LAYER, False);
    _XA_WIN_PROTOCOLS = XInternAtom(GDK_DISPLAY(), XA_WIN_PROTOCOLS, False);
    _XA_WIN_HINTS = XInternAtom(GDK_DISPLAY(), XA_WIN_HINTS, False);
    /* FIXME: There is another hint (XA_WIN_ICONS) that is not yet implemented.
     * But it needs to go in here.
     */

    return;
}

gboolean
gnome_win_hints_set_layer(GtkWidget *window, gulong layer)
{
    XEvent xev;
    GdkWindowPrivate *priv;

    priv = (GdkWindowPrivate*)(GTK_WIDGET(window)->window);
    
    xev.type = ClientMessage;
    xev.xclient.type = ClientMessage;
    xev.xclient.window = priv->xwindow;
    xev.xclient.message_type = _XA_WIN_LAYER;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = layer;
    xev.xclient.data.l[1] = gdk_time_get();

    XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
               SubstructureNotifyMask, (XEvent*) &xev);

    return TRUE;
}

gulong
gnome_win_hints_get_layer(GtkWidget *window)
{
    gulong mylayer;
    Atom r_type;
    int r_format;
    GdkWindowPrivate *priv;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;
    CARD32 layer;

    priv = (GdkWindowPrivate*)(window->window);
    if (XGetWindowProperty(GDK_DISPLAY(), priv->xwindow, _XA_WIN_LAYER, 0, 1,
                           False, XA_CARDINAL, &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1){
            layer = ((CARD32 *)prop)[0];
            mylayer = (gulong)layer;
        }
        XFree(prop);
        return mylayer;
    }
    return WinLayerInvalid;
}

gboolean
gnome_win_hints_set_state(GtkWidget *window,  gint32 mask, gint32 state)
{
    GdkWindowPrivate *privwin;
    XEvent xev;

    privwin = (GdkWindowPrivate*)(GTK_WIDGET(window)->window);
    xev.type = ClientMessage;
    xev.xclient.type = ClientMessage;
    xev.xclient.window = privwin->xwindow;
    xev.xclient.message_type = _XA_WIN_STATE;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = mask;
    xev.xclient.data.l[1] = state;
    xev.xclient.data.l[2] = gdk_time_get();

    XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False, SubstructureNotifyMask, (XEvent*) &xev);
                                   
    return TRUE;
}

gboolean
gnome_win_hints_set_skip(GtkWidget *window,  GnomeWinHintsSkip skip)
{
    GdkWindowPrivate *privwin;
    XEvent xev;

    privwin = (GdkWindowPrivate*)(GTK_WIDGET(window)->window);
    xev.type = ClientMessage;
    xev.xclient.type = ClientMessage;
    xev.xclient.window = privwin->xwindow;
    xev.xclient.message_type = _XA_WIN_HINTS;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = WinHintsSkipFocus|WinHintsSkipWindowMenu|WinHintsSkipTaskBar;
    xev.xclient.data.l[1] = ((skip.skipFocus   ? (1<<0) : (0<<0) ) |
                             (skip.skipWinMenu ? (1<<1) : (0<<1) ) |
                             (skip.skipWinList ? (1<<2) : (0<<2) ));
    xev.xclient.data.l[2] = gdk_time_get();

    XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False, SubstructureNotifyMask, (XEvent*) &xev);
    
    return TRUE;
}


gboolean
gnome_win_hints_set_workspace(GtkWidget *window, gint workspace)
{
    XEvent xev;
    GdkWindowPrivate *priv;
    
    priv = (GdkWindowPrivate*)(GTK_WIDGET(window)->window);

    xev.type = ClientMessage;
    xev.xclient.type = ClientMessage;
    xev.xclient.window = priv->xwindow;
    xev.xclient.message_type = _XA_WIN_WORKSPACE;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = workspace;
    xev.xclient.data.l[1] = gdk_time_get();

    XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
               SubstructureNotifyMask, (XEvent*) &xev);
    
    return TRUE;
}

gboolean
gnome_win_hints_set_current_workspace(gint workspace)
{
    XEvent xev;

    xev.type = ClientMessage;
    xev.xclient.type = ClientMessage;
    xev.xclient.window = GDK_ROOT_WINDOW();
    xev.xclient.message_type = _XA_WIN_WORKSPACE;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = workspace;
    xev.xclient.data.l[1] = gdk_time_get();

    XSendEvent(GDK_DISPLAY(), GDK_ROOT_WINDOW(), False,
               SubstructureNotifyMask, (XEvent*) &xev);
    
    return TRUE;
}

gint
gnome_win_hints_get_current_workspace()
{
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;
    CARD32 ws = 0;

    if (XGetWindowProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(),
                           _XA_WIN_WORKSPACE, 0, 1, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1)
        {
            CARD32 n = *(CARD32 *)prop;

            if (n < gnome_win_hints_get_workspace_count())
                ws = (gint)n;
        }
        XFree(prop);
    }       
    return ws;
}

gint
gnome_win_hints_get_workspace(GtkWidget *window)
{
    Atom r_type;
    GdkWindowPrivate *priv;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;
    CARD32 ws = 0;

    priv = (GdkWindowPrivate*)window->window;
    if (XGetWindowProperty(GDK_DISPLAY(), priv->xwindow,
                           _XA_WIN_WORKSPACE, 0, 1, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1)
        {
            CARD32 n = *(CARD32 *)prop;

            if (n < gnome_win_hints_get_workspace_count())
                ws = (gint)n;
        }
        XFree(prop);
    }       
    return ws;
}

GList*
gnome_win_hints_get_workspace_list(GtkWidget *window)
{
    GList *tmp_list;
    XTextProperty tp;
    char **list;
    int count, i;
    char tmpbuf[1024];

    XGetTextProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(),
                     &tp, _XA_WIN_WORKSPACE_NAMES);

    XTextPropertyToStringList(&tp, &list, &count);

    if(tp.value==NULL) return NULL; /* current wm does not support this! */
    
    tmp_list = NULL;
    for(i=0; i<count; i++)
    {
        sprintf(tmpbuf, "%d", i);
        tmp_list = g_list_append(tmp_list, g_strdup(tmpbuf));
    }
    g_free(tmpbuf);
    
    return tmp_list;
}

    
gint
gnome_win_hints_get_workspace_count(GtkWidget *window)
{
    gint wscount;
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

    if (XGetWindowProperty(GDK_DISPLAY(), GDK_ROOT_WINDOW(),
                           _XA_WIN_WORKSPACE_COUNT, 0, 1, False, XA_CARDINAL,
                           &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 1)
        {
            CARD32 n = *(CARD32 *)prop;
            wscount = (gint)n;
        }
        XFree(prop);
    }       
    return wscount;
}

gint32
gnome_win_hints_get_state(GtkWidget *window)
{
    CARD32 state[2] = { 0, 0 };
    GdkWindowPrivate *priv;
    Atom r_type;
    int r_format;
    unsigned long count;
    unsigned long bytes_remain;
    unsigned char *prop;

    priv = (GdkWindowPrivate*)(window->window);
    if (XGetWindowProperty(GDK_DISPLAY(), priv->xwindow, _XA_WIN_STATE,
                           0, 2, False, XA_CARDINAL, &r_type, &r_format,
                           &count, &bytes_remain, &prop) == Success && prop)
    {
        if (r_type == XA_CARDINAL && r_format == 32 && count == 2)
        {
            state[0] = ((CARD32 *)prop)[0];
            state[1] = ((CARD32 *)prop)[1];
        }
        XFree(prop);
        return (gint32)state[1];
    }
    return NULL;
}
