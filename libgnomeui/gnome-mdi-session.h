/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * gnome-mdi-session.h - session managament functions
 * written by Martin Baulig <martin@home-of-linux.org>
 */

#ifndef __GNOME_MDI_SESSION_H__
#define __GNOME_MDI_SESSION_H__

#include <string.h>

#include "libgnomeui/gnome-mdi.h"

BEGIN_GNOME_DECLS

/* This function should parse the config string and return a newly
 * created GnomeMDIChild. */
typedef GnomeMDIChild *(*GnomeMDIChildCreator) (const gchar *);

/* gnome_mdi_restore_state(): call this with the GnomeMDI object, the
 * config section and the function used to recreate the GnomeMDIChilds
 * from their config strings. */
gboolean	gnome_mdi_restore_state	(GnomeMDI *, const char *,
					 GnomeMDIChildCreator);

/* gnome_mdi_save_state (): call this with the GnomeMDI object as the
 * first and the config section as the second argument. */
void		gnome_mdi_save_state	(GnomeMDI *, const gchar *);

END_GNOME_DECLS

#endif
