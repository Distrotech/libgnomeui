#ifndef LIBGNOMEUI_H
#define LIBGNOMEUI_H

#include "libgnomeui/gnome-compat.h"

#include "libgnome/gnome-defs.h"
#include "libgnomeui/gnome-uidefs.h"
#include "libgnomeui/gnome-about.h"
#ifndef GNOME_EXCLUDE_EXPERIMENTAL
#include "libgnomeui/gnome-animator.h"
#endif
#include "libgnomeui/gnome-app.h"
#include "libgnomeui/gnome-appbar.h"
#include "libgnomeui/gnome-app-helper.h"
#ifndef GNOME_EXCLUDE_EXPERIMENTAL
#include "libgnomeui/gnome-app-util.h"
#endif
#include "libgnomeui/gnome-canvas.h"
#include "libgnomeui/gnome-canvas-image.h"
#include "libgnomeui/gnome-canvas-line.h"
#include "libgnomeui/gnome-canvas-load.h"
#include "libgnomeui/gnome-canvas-rect-ellipse.h"
#include "libgnomeui/gnome-canvas-polygon.h"
#include "libgnomeui/gnome-canvas-text.h"
#include "libgnomeui/gnome-canvas-util.h"
#include "libgnomeui/gnome-canvas-widget.h"
#include "libgnomeui/gnome-color-picker.h"
#include "libgnomeui/gnome-dentry-edit.h"
#include "libgnomeui/gnome-dialog.h"
#include "libgnomeui/gnome-dialog-util.h"
/* There are nice third-party libs for this; 
   but maybe we should leave it. Let's assume it's
   up in the air, but people who aren't cutting 
   edge probably don't want to fool with it */
#ifndef GNOME_EXCLUDE_DEPRECATED
#include "libgnomeui/gnome-dns.h"
#endif
#include "libgnomeui/gnome-dock.h"
#include "libgnomeui/gnome-dock-band.h"
#include "libgnomeui/gnome-dock-item.h"
#include "libgnomeui/gnome-entry.h"
#include "libgnomeui/gnome-file-entry.h"
#include "libgnomeui/gnome-font-picker.h"
#ifndef GNOME_EXCLUDE_DEPRECATED
#include "libgnomeui/gnome-font-selector.h"
#endif
#include "libgnomeui/gnome-geometry.h"
#ifndef GNOME_EXCLUDE_DEPRECATED
#include "libgnomeui/gnome-guru.h"
#endif
#include "libgnomeui/gnome-icon-list.h"
#include "libgnomeui/gnome-icon-item.h"
#include "libgnomeui/gnome-icon-sel.h"
#include "libgnomeui/gnome-icon-entry.h"
#include "libgnomeui/gnome-init.h"
#ifndef GNOME_EXCLUDE_DEPRECATED
#include "libgnomeui/gnome-less.h"
#endif
#include "libgnomeui/gnome-messagebox.h"
#include "libgnomeui/gnome-number-entry.h"
/* Considering moving this to gnome-print, so this file 
   is deprecated but the API isn't */
#include "libgnomeui/gnome-paper-selector.h"
#include "libgnomeui/gnome-popup-menu.h"
/* deprecated? */
#include "libgnomeui/gnome-popup-help.h"
#include "libgnomeui/gnome-pixmap.h"
#include "libgnomeui/gnome-pixmap-entry.h"
#ifndef GNOME_EXCLUDE_DEPRECATED
#include "libgnomeui/gnome-preferences.h"
#endif
#include "libgnomeui/gnome-propertybox.h"
#ifndef GNOME_EXCLUDE_DEPRECATED
#include "libgnomeui/gnome-properties.h"
#include "libgnomeui/gnome-property-entries.h"
#endif
#include "libgnomeui/gnome-scores.h"
#ifndef GNOME_EXCLUDE_DEPRECATED
#include "libgnomeui/gnome-spell.h"
#include "libgnomeui/gnome-startup.h"
#endif
#include "libgnomeui/gnome-types.h"
#include "libgnomeui/gnome-stock.h"
#include "libgnomeui/gnome-client.h"
#ifndef GNOME_EXCLUDE_DEPRECATED
#include "libgnomeui/gtkcauldron.h"
#endif
#include "libgnomeui/gtk-clock.h"
#include "libgnomeui/gtkdial.h"
#ifndef GNOME_EXCLUDE_DEPRECATED
#include "libgnomeui/gtk-ted.h"
#endif
#include "libgnomeui/gtkpixmapmenuitem.h"
#include "libgnomeui/gnome-dateedit.h"
#include "libgnomeui/gnome-calculator.h"
#include "libgnomeui/gnome-mdi.h"
#include "libgnomeui/gnome-mdi-child.h"
#include "libgnomeui/gnome-mdi-generic-child.h"
#include "libgnomeui/gnome-mdi-session.h"
#include "libgnomeui/gnometypebuiltins.h"
#include "libgnomeui/gnome-winhints.h"
#include "libgnomeui/gnome-href.h"
#include "libgnomeui/gnome-procbar.h"
#include "libgnomeui/gnome-druid.h"
#include "libgnomeui/gnome-druid-page.h"
#include "libgnomeui/gnome-druid-page-start.h"
#include "libgnomeui/gnome-druid-page-standard.h"
#include "libgnomeui/gnome-druid-page-finish.h"
#endif




