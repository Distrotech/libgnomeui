/* GnomeCanvas widget - Tk-like canvas widget for Gnome
 *
 * This widget is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_CANVAS_H
#define GNOME_CANVAS_H

#include <stdarg.h>
#include <stdlib.h>
#include <libgnome/gnome-defs.h>
#include <gtk/gtkcontainer.h>


BEGIN_GNOME_DECLS


/* Standard tags and keys.  These point to unique strings so that we can quickly compare pointers
 * instead of doing a strcmp().  So be sure you use these macros instead of explicit strings.
 */

#define GNOME_CANVAS_CURRENT       _gnome_canvas_current
#define GNOME_CANVAS_ALL           _gnome_canvas_all

#define GNOME_CANVAS_END           _gnome_canvas_end
#define GNOME_CANVAS_X             _gnome_canvas_x
#define GNOME_CANVAS_Y             _gnome_canvas_y
#define GNOME_CANVAS_X1            _gnome_canvas_x1
#define GNOME_CANVAS_Y1            _gnome_canvas_y1
#define GNOME_CANVAS_X2            _gnome_canvas_x2
#define GNOME_CANVAS_Y2            _gnome_canvas_y2
#define GNOME_CANVAS_FILL_COLOR    _gnome_canvas_fill_color
#define GNOME_CANVAS_OUTLINE_COLOR _gnome_canvas_outline_color
#define GNOME_CANVAS_WIDTH_PIXELS  _gnome_canvas_width_pixels
#define GNOME_CANVAS_WIDTH_UNITS   _gnome_canvas_width_units

/* Standard tags declarations.  Normally not used; you should use the above macros instead. */

typedef const char * const GnomeCanvasKey;

extern GnomeCanvasKey _gnome_canvas_current;
extern GnomeCanvasKey _gnome_canvas_all;

extern GnomeCanvasKey _gnome_canvas_end;
extern GnomeCanvasKey _gnome_canvas_x;
extern GnomeCanvasKey _gnome_canvas_y;
extern GnomeCanvasKey _gnome_canvas_x1;
extern GnomeCanvasKey _gnome_canvas_y1;
extern GnomeCanvasKey _gnome_canvas_x2;
extern GnomeCanvasKey _gnome_canvas_y2;
extern GnomeCanvasKey _gnome_canvas_fill_color;
extern GnomeCanvasKey _gnome_canvas_outline_color;
extern GnomeCanvasKey _gnome_canvas_width_pixels;
extern GnomeCanvasKey _gnome_canvas_width_units;


/* "Small" value used by canvas stuff */

#define GNOME_CANVAS_EPSILON 1e-10


#define GNOME_CANVAS(obj)         GTK_CHECK_CAST (obj, gnome_canvas_get_type (), GnomeCanvas)
#define GNOME_CANVAS_CLASS(klass) GTK_CHECK_CLASS_CAST (klass, gnome_canvas_get_type (), GnomeCanvasClass)
#define GNOME_IS_CANVAS(obj)      GTK_CHECK_TYPE (obj, gnome_canvas_get_type ())

typedef struct _GnomeCanvas GnomeCanvas;
typedef struct _GnomeCanvasClass GnomeCanvasClass;


/* These indicate in which ways a color can be specified to the canvas */

typedef enum {
	GNOME_CANVAS_COLOR_NONE,        /* no color (used to clear color parameters) */
	GNOME_CANVAS_COLOR_STRING,	/* as seen by gdk_color_parse() */
	GNOME_CANVAS_COLOR_RGB,		/* three gushort RGB values */
	GNOME_CANVAS_COLOR_RGB_D,	/* three double RGB values */
	GNOME_CANVAS_COLOR_GDK		/* already-allocated GdkColor* */
} GnomeCanvasColor;

/* Flag bits for the GnomeCanvas flags */

enum {
	GNOME_CANVAS_NEED_REDRAW       = 1 << 0,
	GNOME_CANVAS_BG_SET            = 1 << 1,
	GNOME_CANVAS_NEED_REPICK       = 1 << 2,
	GNOME_CANVAS_LEFT_GRABBED_ITEM = 1 << 3,
	GNOME_CANVAS_IN_REPICK         = 1 << 4
};


/* Canvas item base structures.  We do not use the Gtk object system for canvas items because
 * these must be as lightweight as possible.
 */

typedef struct GnomeCanvasItem GnomeCanvasItem;

typedef struct {
	char *type;			/* this is NOT malloc'ed, it should point to a static string */
	size_t item_size;		/* size of an item structure of this type */
	int always_redraw;		/* specifies whether items of this type should be redrawn
					 * (i.e. for items that have their own X windows)
					 */

	/* create a new item of this type */
	void (*create) (GnomeCanvas *canvas, GnomeCanvasItem *item, va_list args);

	/* destroy an item of this type */
	void (*destroy) (GnomeCanvas *canvas, GnomeCanvasItem *item);

	/* configure an item of this type */
	void (*configure) (GnomeCanvas *canvas, GnomeCanvasItem *item, va_list args, int reconfigure);

	/* realize an item of this type (create GCs, etc.) */
	void (*realize) (GnomeCanvas *canvas, GnomeCanvasItem *item);

	/* unrealize an item of this type */
	void (*unrealize) (GnomeCanvas *canvas, GnomeCanvasItem *item);

	/* Notify an item that the canvas was mapped.  Normally only needed by items with their own
         * windows.
	 */
	void (*map) (GnomeCanvas *canvas, GnomeCanvasItem *item);

	/* notify an item that the canvas was unmapped */
	void (*unmap) (GnomeCanvas *canvas, GnomeCanvasItem *item);

	/* Draw an item of this type.  (x, y) are the upper-left canvas coordinates of the drawable,
	 * which usually is the temporary pixmap where stuff is drawn.  (width, height) are the
	 * dimensions of the drawable.
	 */
	void (*draw) (GnomeCanvas *canvas, GnomeCanvasItem *item, GdkDrawable *drawable,
		      int x, int y, int width, int height);

	/* calculate the distance from the item to the specified point */
	double (*point) (GnomeCanvas *canvas, GnomeCanvasItem *item, double x, double y);

	/* compute whether the item intersects the specified area */
	GtkVisibility (*intersect) (GnomeCanvas *canvas, GnomeCanvasItem *item,
				    double x, double y, double width, double height);
} GnomeCanvasItemType;

struct GnomeCanvasItem {
	GnomeCanvasItemType *type;	/* the type structure for this item */

	GHashTable *tags;		/* tags associated to this item */

	int x1, y1, x2, y2;		/* Bounding box for this item, in canvas pixel
					 * coordinates. This is set by the item-specific code.  The
					 * bounding box contains x1 and y1, but not x2 and y2.
					 */
};


/* Type for user items or tags.  You should not set this by hand; use the provided functions
 * instead.
 */

typedef struct {
	enum {
		GNOME_CANVAS_ID_EMPTY,
		GNOME_CANVAS_ID_ITEM,
		GNOME_CANVAS_ID_TAG
	} type;
	gpointer data;		/* pointer to a GnomeCanvasItem (ID_ITEM) or a GnomeCanvasKey (ID_TAG) */
} GnomeCanvasId;


/* GnomeCanvas structures */

struct _GnomeCanvas {
	GtkContainer container;

	int flags;				/* Internal flags */
	guint idle_id;				/* Idle handler ID */

	GList *item_list;			/* the list of items in this canvas */
	GList *item_list_end;			/* end of the list of items */

	double scroll_x1, scroll_y1;		/* scrolling limit */
	double scroll_x2, scroll_y2;

	double pixels_per_unit;			/* scaling factor to be used for display */

	int display_x1, display_y1;		/* Top-left projection coordinates in canvas pixel coordinates */
	int draw_x1, draw_y1;			/* Top-left canvas pixel coordinates of temporary draw pixmap */

	int redraw_x1, redraw_y1;		/* Area that needs redrawing.  Contains x1, y1 but not x2, y2 */
	int redraw_x2, redraw_y2;

	int width, height;			/* Size of canvas window in pixels */

	GHashTable *bindings;			/* Table of bindings to item events */
	int binding_id;				/* Sequential numbering of bindings */

	int state;				/* Last known modifier state, for deferred repick when a button is down */

	GnomeCanvasItem *current_item;		/* The item containing the mouse pointer, or NULL if none */
	GnomeCanvasItem *new_current_item;	/* Item that is about to become current (to track deletions and such) */

	GdkEvent pick_event;			/* Event on which selection of current item is based */

	int close_enough;			/* Tolerance distance for picking items */

	GdkVisual *visual;			/* Visual and colormap to use */
	GdkColormap *colormap;
	GdkColorContext *cc;			/* Color context used for color allocation */
	gulong bg_pixel;			/* Background color pixel value */
	GdkGC *pixmap_gc;			/* GC for temporary pixmap */
};

struct _GnomeCanvasClass {
	GtkContainerClass parent_class;

	/* Same semantics as "event" signal in GtkWidget.  Your callback function will get *two*
	 * user data pointers, though.  One is the user_data here from the item binding, and the
	 * other one is the normal user data from the Gtk signal connection.
	 *
	 * We pass a pointer to an item instead of passing the GnomeCanvasId structure by value (as
	 * in the rest of the canvas), because Gtk signals do not let us pass structures by value.
	 */
	gint (*item_event) (GnomeCanvas *canvas, GnomeCanvasId *item, GdkEvent *event, gpointer user_data);
};


/* Registers a new canvas item type.  The type parameter must point to static storage.  The
 * predefined types will be registered automatically by gnome_canvas_class_init().
 */
void gnome_canvas_register_type (GnomeCanvasItemType *type);

/* Unregisters all item types.  This should be called by the global cleanup routine. */
void gnome_canvas_uninit (void);

/* Standard Gtk function */
GtkType gnome_canvas_get_type (void);

/* Creates a new canvas with the specified visual and colormap.  We cannot default to imlib's
 * visual and colormap because that could be expensive if the user never used imlib images in
 * the canvas.
 */
GtkWidget *gnome_canvas_new (GdkVisual *visual, GdkColormap *colormap);

/* Creates a new canvas item and inserts it at the top of the item list */
GnomeCanvasId gnome_canvas_create_item (GnomeCanvas *canvas, char *type, ...);

/* Creates a tag identifier for use with other canvas functions, tag must point to a
 * statically-allocated string.  You should use the GNOME_CANVAS_* macros defined above for
 * this.
 */
GnomeCanvasId gnome_canvas_create_tag (GnomeCanvas *canvas, GnomeCanvasKey tag);

/* Configures the specified item(s).  If the GnomeCanvasId specifies a single item, then only
 * that item is affected.  If it specifies a tag, then all the items with that tag are
 * modified.
 */
void gnome_canvas_configure (GnomeCanvas *canvas, GnomeCanvasId cid, ...);

/* Creates a binding for canvas items.  Event types may only be mouse, keyboard, and enter/leave events.
 * The "item_event" signal will be emitted when a bound event occurs.  This function returns the binding id.
 */
int gnome_canvas_bind (GnomeCanvas *canvas, GnomeCanvasId cid, GdkEventType type, gpointer user_data);

/* Removes a binding by binding id */
void gnome_canvas_unbind (GnomeCanvas *canvas, int binding_id);

/* Sets the color of the canvas background. */
void gnome_canvas_set_background (GnomeCanvas *canvas, GnomeCanvasColor color_type, ...);

/* Sets the limits of the scrolling region */
void gnome_canvas_set_scroll_region (GnomeCanvas *canvas, double x1, double y1, double x2, double y2);

/* Sets the number of pixels that correspond to one unit in world coordinates */
void gnome_canvas_set_pixels_per_unit (GnomeCanvas *canvas, double n);

/* Sets the size in pixels of the canvas */
void gnome_canvas_set_size (GnomeCanvas *canvas, int width, int height);

/* Informs the canvas that the specified area needs to be redrawn at some time.  It will most
 * likely get redrawn in the idle loop.  The area includes x1 and y1 but not x2 and y2.
 */
void gnome_canvas_request_redraw (GnomeCanvas *canvas, int x1, int y1, int x2, int y2);

/* These functions convert from a coordinate system to another.  "w" is world coordinates (the ones
 * in which objects are specified), "c" is canvas coordinates (pixel coordinates that are (0,0) for
 * the upper-left scrolling limit and something else for the lower-left scrolling limit), and "s" is
 * screen coordinates (relative to the canvas X window).
 */
void gnome_canvas_w2c (GnomeCanvas *canvas, double wx, double wy, int *cx, int *cy);
void gnome_canvas_c2w (GnomeCanvas *canvas, int cx, int cy, double *wx, double *wy);
void gnome_canvas_c2s (GnomeCanvas *canvas, int cx, int cy, int *sx, int *sy);
void gnome_canvas_s2c (GnomeCanvas *canvas, int sx, int sy, int *cx, int *cy);

/* Takes a color specification and allocates it into the GdkColor.  This is to be used mainly by the
 * internal item type code.  Returns FALSE if the color is GNOME_CANVAS_COLOR_NONE, TRUE otherwise.
 */
int gnome_canvas_get_color (GnomeCanvas *canvas, GdkColor *color, GnomeCanvasColor color_type, ...);

/* This macro calls gnome_canvas_get_color while extracting the arguments from a va_list.  I know it
 * sucks.
 */
#define GNOME_CANVAS_VGET_COLOR(canvas, color, color_type, args)						\
	((color_type == GNOME_CANVAS_COLOR_NONE) ? gnome_canvas_get_color (canvas, color, color_type) :		\
	 (color_type == GNOME_CANVAS_COLOR_STRING) ? gnome_canvas_get_color (canvas, color, color_type,		\
									     va_arg (args, gchar *)) :		\
	 (color_type == GNOME_CANVAS_COLOR_RGB) ? gnome_canvas_get_color (canvas, color, color_type,		\
									  va_arg (args, unsigned int),		\
									  va_arg (args, unsigned int),		\
									  va_arg (args, unsigned int)) :	\
	 (color_type == GNOME_CANVAS_COLOR_RGB_D) ? gnome_canvas_get_color (canvas, color, color_type,		\
									    va_arg (args, double),		\
									    va_arg (args, double),		\
									    va_arg (args, double)) :		\
	 (color_type == GNOME_CANVAS_COLOR_GDK) ? gnome_canvas_get_color (canvas, color, color_type,		\
									  va_arg (args, GdkColor *)) :		\
	 gnome_canvas_get_color (canvas, color, color_type))


END_GNOME_DECLS

#endif
