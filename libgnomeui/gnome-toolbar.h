/* GNOME GUI Library
 * Copyright (C) 1997 The Free Software Foundation
 *
 * Author: Aldy Hernandez (aldy@uaa.edu)
 *
 * GNOME toolbar support definitions
 */

#ifndef __GNOME_TOOLBAR_H__
#define __GNOME_TOOLBAR_H__

#include "libgnome/gnome-defs.h"

BEGIN_GNOME_DECLS

typedef struct _GnomeToolbar		GnomeToolbar;
typedef struct _GnomeToolbarItem	GnomeToolbarItem;

typedef void (*GnomeToolbarFunc) (void *data);

enum GnomeToolbarStyle
{
	GNOME_TB_NOTHING    = 0,
	GNOME_TB_TEXT       = 2,
	GNOME_TB_ICONS      = 4,
	GNOME_TB_VERTICAL   = 8,
	GNOME_TB_HORIZONTAL = 16,
	GNOME_TB_AXIS       = 24
};

enum GnomePackMethod
{
	GNOME_TB_CONTAINER_ADD,
	GNOME_TB_PACK_START,
	GNOME_TB_PACK_END
};

enum GnomeToolbarDefaultItem
{
	GNOME_TOOLBAR_OPEN,
	GNOME_TOOLBAR_CLOSE,
	GNOME_TOOLBAR_RELOAD,
	GNOME_TOOLBAR_PRINT,
	GNOME_TOOLBAR_EXIT
};

struct _GnomeToolbar
{
	/* Actual toolbar widget */
	GtkWidget *toolbar;
	
	/* Parent box in which to put ``toolbar''.  We need a separate box
	 * in which to put the toolbar in so we can destroy and recreate the
	 * toolbar at will.
	 */
	GtkWidget *box;
	
	enum GnomeToolbarStyle style;
	
	int nitems;
	
	GnomeToolbarItem *items;
};

struct _GnomeToolbarItem
{
	char *label;
	char *pixmap_filename;
	void *data;
	GnomeToolbarFunc func;
};

GnomeToolbar*	gnome_create_toolbar	  (void                         *parent,
					   enum GnomePackMethod         packmethod);
void		gnome_destroy_toolbar	  (GnomeToolbar                 *toolbar);
void		gnome_toolbar_add 	  (GnomeToolbar                 *toolbar,
					   char                         *label,
					   char                         *pixmap_filename,
					   GnomeToolbarFunc             func,
					   void                         *data);
void		gnome_toolbar_add_default (GnomeToolbar                 *toolbar,
					   enum GnomeToolbarDefaultItem tb_default,
					   GnomeToolbarFunc             func,
					   void                         *data);
void		gnome_toolbar_set_style   (GnomeToolbar                 *toolbar,
					   enum GnomeToolbarStyle       style);

END_GNOME_DECLS

#endif /* __GNOME_TOOLBAR_H__ */


