/* GNOME GUI Library
 * Copyright (C) 1997 the Free Software Foundation
 *
 * Special widget for the root window.
 *
 * Author: Federico Mena
 */

#ifndef GNOME_ROOTWIN_H
#define GNOME_ROOTWIN_H


#include <gtk/gtkwindow.h>


BEGIN_GNOME_DECLS


#define GNOME_ROOTWIN(obj)         GTK_CHECK_CAST (obj, gnome_rootwin_get_type (), GnomeRootWin)
#define GNOME_ROOTWIN_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gnome_rootwin_get_type (), GnomeRootWinClass)
#define GNOME_IS_ROOTWIN(obj)      GTK_CHECK_TYPE (obj, gnome_rootwin_get_type ())


typedef struct _GnomeRootWin      GnomeRootWin;
typedef struct _GnomeRootWinClass GnomeRootWinClass;

struct _GnomeRootWin {
	GtkWindow window;
};

struct _GnomeRootWinClass {
	GtkWindowClass parent_class;
};


guint      gnome_rootwin_get_type (void);
GtkWidget *gnome_rootwin_new      (void);


END_GNOME_DECLS

#endif
