NULL =

INCLUDES =					\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	$(GTK_CFLAGS)		\
	$(GTK_CFLAGS) 			\
	$(VFS_CFLAGS)				\
	-D_FILE_OFFSET_BITS=64			\
	-D_BSD_SOURCE				\
	-D_GNU_SOURCE				\
	-D_LARGEFILE64_SOURCE			\
	-D_POSIX_PTHREAD_SEMANTICS		\
	-D_REENTRANT				\
	-DSSH_PROGRAM=\"$(SSH_PROGRAM)\"	\
	-DG_DISABLE_DEPRECATED			\
	-DDATADIR=\"$(datadir)\"		\
	-DPREFIX=\"$(prefix)\"			\
	-DLIBDIR=\"$(libdir)\"			\
	-DSYSCONFDIR=\"$(sysconfdir)\"		\
	-DG_LOG_DOMAIN=\"gnome-vfs-modules\"	\
	$(NULL)

module_flags = -export_dynamic -avoid-version -module -no-undefined
modulesdir = $(libdir)/gtk-2.0/$(GTK_VERSION)/filesystems

modules_LTLIBRARIES =				\
	libgnome-vfs.la			\
	$(NULL)

libgnome_vfs_la_SOURCES = gtkfilesystemgnomevfs.c gtkfilesystemgnomevfs.h
libgnome_vfs_la_LDFLAGS = $(module_flags)
libgnome_vfs_la_LIBADD = $(GTK_LIBS) ../libgnomevfs/libgnomevfs-2.la
