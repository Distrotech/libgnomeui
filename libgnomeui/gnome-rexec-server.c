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

#include <liboaf/liboaf.h>

#include <libgnome/gnome-exec.h>
#include <errno.h>
#include "gnome-corba-rexec.h"

static CORBA_long
impl_execVectorEnvPath(POA_GNOME_RemoteExecution *servant,
		       GNOME_stringlist *argv,
		       GNOME_stringlist *envp,
		       CORBA_Environment *ev)
{
  int retval;

  retval = gnome_execute_async_with_env(NULL, argv->_length, argv->_buffer, envp->_length, envp->_buffer);
  if(retval < 0) {
    int myerrno = errno;
    GNOME_RemoteExecution_POSIXError *raiseme = GNOME_RemoteExecution_POSIXError__alloc();

    raiseme->unportable_errno = myerrno;
    raiseme->errstr = CORBA_string_dup(g_strerror(myerrno));

    CORBA_exception_set(ev, CORBA_USER_EXCEPTION, ex_GNOME_RemoteExecution_POSIXError, raiseme);
  }

  return retval;
}

int main(int argc, char *argv[])
{
  static POA_GNOME_RemoteExecution__epv POA_GNOME_RemoteExecution_epv = {
    NULL, /* _private */
    (gpointer)&impl_execVectorEnvPath
  };
  static POA_GNOME_RemoteExecution__vepv POA_GNOME_RemoteExecution_vepv = {
    NULL, /* _private */
    &POA_GNOME_RemoteExecution_epv
  };
  static POA_GNOME_RemoteExecution rexec_servant = {
    NULL, /* _private */
    &POA_GNOME_RemoteExecution_vepv
  };
  GMainLoop *ml;
  CORBA_ORB orb;
  PortableServer_POA poa;
  CORBA_Environment ev;
  CORBA_Object rexec_client;

  CORBA_exception_init(&ev);
  orb = oaf_init(argc, argv);

  poa = (PortableServer_POA)CORBA_ORB_resolve_initial_references(orb, "RootPOA", &ev);
  PortableServer_POAManager_activate(PortableServer_POA__get_the_POAManager(poa, &ev), &ev);
  PortableServer_POA_activate_object(poa, &rexec_servant, &ev);

  rexec_client = PortableServer_POA_servant_to_reference(poa,
							 &rexec_servant,
							 &ev);

  oaf_active_server_register("OAFIID:gnome-rexec:19991122", rexec_client);

  ml = g_main_new(FALSE);
  g_main_run(ml);

  return 0;
}
