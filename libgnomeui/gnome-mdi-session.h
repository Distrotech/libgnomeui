/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * gnome-mdi-session.h - session managament functions
 * written by Martin Baulig <martin@home-of-linux.org>
 */

#ifndef __GNOME_MDI_SESSION_H__
#define __GNOME_MDI_SESSION_H__

#ifndef GNOME_DISABLE_DEPRECATED

#include <string.h>

#include "libgnomeui/gnome-mdi.h"

G_BEGIN_DECLS

/* This function should parse the config string and return a newly
 * created GnomeMDIChild. */
typedef GnomeMDIChild *(*GnomeMDIChildCreator) (const gchar *);

/* gnome_mdi_restore_state(): call this with the GnomeMDI object, the
 * config section name and the function used to recreate the GnomeMDIChildren
 * from their config strings. */
gboolean	gnome_mdi_restore_state	(GnomeMDI *mdi, const gchar *section,
					 GnomeMDIChildCreator create_child_func);

/* gnome_mdi_save_state (): call this with the GnomeMDI object as the
 * first and the config section name as the second argument. */
void		gnome_mdi_save_state	(GnomeMDI *mdi, const gchar *section);

G_END_DECLS

#endif /* GNOME_DISABLE_DEPRECATED */

#endif
