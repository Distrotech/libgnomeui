/*
 * Copyright (C) 1997, 1998, 1999, 2000 Free Software Foundation
 * All rights reserved.
 *
 * This file is part of the Gnome Library.
 *
 * The Gnome Library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * The Gnome Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with the Gnome Library; see the file COPYING.LIB.  If not,
 * write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */
/*
  @NOTATION@
 */

/* DEPRECATED */
#ifndef __GNOME_DNS_H__
#define __GNOME_DNS_H__

#include <libgnome/gnome-defs.h>

BEGIN_GNOME_DECLS

/*
 *--------------------------------------------------------------
 * gnome_dns_init
 *
 *   Initialize the dns functions for use.
 *
 * Arguments:
 *   server_count specifies the number of servers to fork() at
 *   init, or <= 0 to do dynamic server forking.  If you are
 *   concerned about virtual mem usage, fork your servers as one of the
 *   first things in your program.
 * 
 *   Note that it will still do dynamic forking if you specify > 0. 
 *   when it runs out of servers.. a good init value may be 1 or 2.
 * 
 * Results:
 *   gnome_dns_lookup() will be ready for use.
 *
 * Side effects:
 *   The library is initialized.
 *
 *--------------------------------------------------------------
 */
void gnome_dns_init (gint server_count);

/*
 *--------------------------------------------------------------
 * gnome_dns_lookup
 *
 * looks up an address and returns a tag for use with
 * gnome_dns_abort() if desired.  May not return -1 if
 * hostname was in cache.
 *
 * Arguments:
 * char *hostname - hostname to lookup
 * callback - function to call when dns lookup is complete.
 * callback_data - data to pass to this function.
 *   
 * Results:
 * callback function is called when dns_lookup is complete.
 * returns a tag identifying this lookup or -1 if lookup was
 * in cache.
 * 
 * Side effects:
 * a new dns server may be spawned if all the current servers
 * are in use.
 *
 *--------------------------------------------------------------
 */

guint32 gnome_dns_lookup (const char *hostname,
			   void (* callback) (guint32 ip_addr, void *callback_data),
			   void *callback_data);
	
/*
 *--------------------------------------------------------------
 * gnome_dns_abort
 *
 * aborts a previous call to gnome_dns_lookup().
 *
 * Arguments:
 * gint tag32 - the tag returned from previous call to gnome_dns_lookup().
 *   
 * Results:
 *   callback function is not called.
 *
 * Side effects:
 * 
 *--------------------------------------------------------------
 */
void gnome_dns_abort (guint32 tag);

END_GNOME_DECLS

#endif /* __GNOME_DNS_H__ */
