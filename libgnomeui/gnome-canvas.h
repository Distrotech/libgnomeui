/* GnomeCanvas widget - Tk-like canvas widget for Gnome
 *
 * GnomeCanvas is basically a port of the Tk toolkit's most excellent canvas widget.  Tk is
 * copyrighted by the Regents of the University of California, Sun Microsystems, and other parties.
 *
 * Copyright (C) 1998 The Free Software Foundation
 *
 * Author: Federico Mena <federico@nuclecu.unam.mx>
 */

#ifndef GNOME_CANVAS_H
#define GNOME_CANVAS_H

#include <libgnome/gnome-defs.h>
#include <gtk/gtkcontainer.h>


BEGIN_GNOME_DECLS


/* "Small" value used by canvas stuff */
#define GNOME_CANVAS_EPSILON 1e-10


typedef struct _GnomeCanvas           GnomeCanvas;
typedef struct _GnomeCanvasClass      GnomeCanvasClass;
typedef struct _GnomeCanvasGroup      GnomeCanvasGroup;
typedef struct _GnomeCanvasGroupClass GnomeCanvasGroupClass;


/* GnomeCanvasItem - base item class for canvas items
 *
 * All canvas items are derived from GnomeCanvasItems.  The only information a GnomeCanvasItem
 * contains is its parent canvas, its parent canvas item group, and its bounding box in canvas pixel
 * coordinates.
 *
 * Items inside a canvas are organized in a tree of GnomeCanvasItemGroup nodes and GnomeCanvasItem
 * leaves.  Each canvas has a single root group, which can be obtained with the
 * gnome_canvas_get_root() function.
 *
 */


/* Object flags for items */
enum {
	GNOME_CANVAS_ITEM_ALWAYS_REDRAW = 1 << 4
};

#define GNOME_TYPE_CANVAS_ITEM            (gnome_canvas_item_get_type ())
#define GNOME_CANVAS_ITEM(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_ITEM, GnomeCanvasItem))
#define GNOME_CANVAS_ITEM_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_ITEM, GnomeCanvasItemClass))
#define GNOME_IS_CANVAS_ITEM(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_ITEM))
#define GNOME_IS_CANVAS_ITEM_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_ITEM))


typedef struct _GnomeCanvasItem GnomeCanvasItem;
typedef struct _GnomeCanvasItemClass GnomeCanvasItemClass;

struct _GnomeCanvasItem {
	GtkObject object;

	GnomeCanvas *canvas;		/* The parent canvas for this item */
	GnomeCanvasItem *parent;	/* The parent canvas item (of type GnomeCanvasGroup) */

	int x1, y1, x2, y2;		/* Bounding box for this item, in canvas pixel coordinates.
					 * The bounding box contains (x1, y1) but not (x2, y2).
					 */
};

struct _GnomeCanvasItemClass {
	GtkObjectClass parent_class;

	/* Reconfigure an item -- fixup GCs, recalc bounds, etc. */
	void (* reconfigure) (GnomeCanvasItem *item);

	/* Realize an item -- create GCs, etc. */
	void (* realize) (GnomeCanvasItem *item);

	/* Unrealize an item */
	void (* unrealize) (GnomeCanvasItem *item);

	/* Map an item - normally only need by items with their own GdkWindows */
	void (* map) (GnomeCanvasItem *item);

	/* Unmap an item */
	void (* unmap) (GnomeCanvasItem *item);

	/* Draw an item of this type.  (x, y) are the upper-left canvas pixel coordinates of the *
	 * drawable, a temporary pixmap, where things get drawn.  (width, height) are the dimensions
	 * of the drawable.
	 */
	void (* draw) (GnomeCanvasItem *item, GdkDrawable *drawable,
		       int x, int y, int width, int height);

	/* Calculate the distance from an item to the specified point.  It also returns a canvas
         * item which is the item itself in the case of the object being an actual leaf item, or a
         * child in case of the object being a canvas group.  (cx, cy) are the canvas pixel
         * coordinates that correspond to the item-relative coordinates (x, y).
	 */
	double (* point) (GnomeCanvasItem *item, double x, double y, int cx, int cy, GnomeCanvasItem **actual_item);

	/* Move an item by the specified amount */
	void (* translate) (GnomeCanvasItem *item, double dx, double dy);

	/* Signal: an event ocurred for an item of this type.  The (x, y) coordinates are in the
	 * canvas world coordinate system.
	 */
	gint (* event) (GnomeCanvasItem *item, GdkEvent *event);
};


/* Standard Gtk function */
GtkType gnome_canvas_item_get_type (void);

/* Create a canvas item using the standard Gtk argument mechanism.  The item is automatically
 * inserted at the top of the specified canvas group.
 */
GnomeCanvasItem *gnome_canvas_item_new (GnomeCanvasGroup *parent, GtkType type, ...);

/* Same as above, with parsed args */
GnomeCanvasItem *gnome_canvas_item_newv (GnomeCanvasGroup *parent, GtkType type, guint nargs, GtkArg *args);

/* Configure an item using the standard Gtk argument mechanism */
void gnome_canvas_item_set (GnomeCanvasItem *item, ...);

/* Same as above, with parsed args */
void gnome_canvas_item_setv (GnomeCanvasItem *item, guint nargs, GtkArg *args);

/* Move an item by the specified amount */
void gnome_canvas_item_move (GnomeCanvasItem *item, double dx, double dy);

/* Raise an item in the z-order of its parent group by the specified
 * number of positions.  The specified number must be larger than or
 * equal to 1.
 */
void gnome_canvas_item_raise (GnomeCanvasItem *item, int positions);

/* Lower an item in the z-order of its parent group by the specified
 * number of positions.  The specified number must be larger than or
 * equal to 1.
 */
void gnome_canvas_item_lower (GnomeCanvasItem *item, int positions);

/* Raise an item to the top of its parent group's z-order. */
void gnome_canvas_item_raise_to_top (GnomeCanvasItem *item);

/* Lower an item to the bottom of its parent group's z-order */
void gnome_canvas_item_lower_to_bottom (GnomeCanvasItem *item);

/* Grab the mouse for the specified item.  Only the events in event_mask will be reported.  If
 * cursor is non-NULL, it will be used during the duration of the grab.  Time is a proper X event
 * time parameter.  Returns the same values as XGrabPointer().
 */
int gnome_canvas_item_grab (GnomeCanvasItem *item, unsigned int event_mask, GdkCursor *cursor, guint32 etime);

/* Ungrabs the mouse -- the specified item must be the same that was passed to
 * gnome_canvas_item_grab().  Time is a proper X event time parameter.
 */
void gnome_canvas_item_ungrab (GnomeCanvasItem *item, guint32 etime);

/* These functions convert from a coordinate system to another.  "w" is world coordinates and "i" is
 * item coordinates.
 */
void gnome_canvas_item_w2i (GnomeCanvasItem *item, double *x, double *y);
void gnome_canvas_item_i2w (GnomeCanvasItem *item, double *x, double *y);


/* GnomeCanvasGroup - a group of canvas items
 *
 * A group is a node in the hierarchical tree of groups/items inside a canvas.  Groups serve to
 * give a logical structure to the items.
 *
 * Consider a circuit editor application that uses the canvas for its schematic display.
 * Hierarchically, there would be canvas groups that contain all the components needed for an
 * "adder", for example -- this includes some logic gates as well as wires.  You can move stuff
 * around in a convenient way by doing a gnome_canvas_item_move() of the hierarchical groups -- to
 * move an adder, simply move the group that represents the adder.
 *
 * The (xpos, ypos) fields of a canvas group are the relative origin for the group's children.
 */


#define GNOME_TYPE_CANVAS_GROUP            (gnome_canvas_group_get_type ())
#define GNOME_CANVAS_GROUP(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS_GROUP, GnomeCanvasGroup))
#define GNOME_CANVAS_GROUP_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS_GROUP, GnomeCanvasGroupClass))
#define GNOME_IS_CANVAS_GROUP(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS_GROUP))
#define GNOME_IS_CANVAS_GROUP_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS_GROUP))


struct _GnomeCanvasGroup {
	GnomeCanvasItem item;

	GList *item_list;
	GList *item_list_end;

	double xpos, ypos;
};

struct _GnomeCanvasGroupClass {
	GnomeCanvasItemClass parent_class;
};


/* Standard Gtk function */
GtkType gnome_canvas_group_get_type (void);

/* For use only by the core and item type implementations.  If item is non-null, then the group adds
 * the item's bounds to the current group's bounds, and propagates it upwards in the item hierarchy.
 * If item is NULL, then the group asks every sub-group to recalculate its bounds recursively, and
 * then propagates the bounds change upwards in the hierarchy.
 */
void gnome_canvas_group_child_bounds (GnomeCanvasGroup *group, GnomeCanvasItem *item);


/*** GnomeCanvas ***/


#define GNOME_TYPE_CANVAS            (gnome_canvas_get_type ())
#define GNOME_CANVAS(obj)            (GTK_CHECK_CAST ((obj), GNOME_TYPE_CANVAS, GnomeCanvas))
#define GNOME_CANVAS_CLASS(klass)    (GTK_CHECK_CLASS_CAST ((klass), GNOME_TYPE_CANVAS, GnomeCanvasClass))
#define GNOME_IS_CANVAS(obj)         (GTK_CHECK_TYPE ((obj), GNOME_TYPE_CANVAS))
#define GNOME_IS_CANVAS_CLASS(klass) (GTK_CHECK_CLASS_TYPE ((klass), GNOME_TYPE_CANVAS))


struct _GnomeCanvas {
	GtkContainer container;

	guint idle_id;				/* Idle handler ID */

	GnomeCanvasItem *root;			/* root canvas group */
	guint root_destroy_id;			/* Signal handler ID for destruction of root item */

	double scroll_x1, scroll_y1;		/* scrolling limit */
	double scroll_x2, scroll_y2;

	double pixels_per_unit;			/* scaling factor to be used for display */

	int display_x1, display_y1;		/* Top-left projection coordinates in
						 * canvas pixel coordinates
						 */

	int redraw_x1, redraw_y1;
	int redraw_x2, redraw_y2;		/* Area that needs redrawing.  Contains (x1, y1)
						 * but not (x2, y2)
						 */

	int width, height;			/* Size of canvas window in pixels */

	int state;				/* Last known modifier state, for deferred repick
						 * when a button is down
						 */

	GnomeCanvasItem *current_item;		/* The item containing the mouse pointer, or NULL if none */
	GnomeCanvasItem *new_current_item;	/* Item that is about to become current
						 * (used to track deletions and such)
						 */
	GnomeCanvasItem *grabbed_item;		/* Item that holds a pointer grab, or NULL if none */
	guint grabbed_event_mask;		/* Event mask specified when grabbing an item */

	GdkEvent pick_event;			/* Event on which selection of current item is based */

	int close_enough;			/* Tolerance distance for picking items */

	GdkVisual *visual;			/* Visual and colormap to use */
	GdkColormap *colormap;
	GdkColorContext *cc;			/* Color context used for color allocation */
	gulong bg_pixel;			/* Background color pixel value */
	GdkGC *pixmap_gc;			/* GC for temporary pixmap */

	int need_redraw : 1;			/* Will redraw at next idle loop iteration */
	int need_repick : 1;			/* Will repick current item at next idle loop iteration */
	int left_grabbed_item : 1;		/* For use by the internal pick_event function */
	int in_repick : 1;			/* For use by the internal pick_event function */
	int bg_set : 1;				/* Background color is set */
};

struct _GnomeCanvasClass {
	GtkContainerClass parent_class;
};


/* Standard Gtk function */
GtkType gnome_canvas_get_type (void);

/* Creates a new canvas with the specified visual and colormap.  We cannot default to imlib's
 * visual and colormap because that could be expensive if the user never used imlib images in
 * the canvas.
 */
GtkWidget *gnome_canvas_new (GdkVisual *visual, GdkColormap *colormap);

/* Constructor useful for derived classes and language wrappers */
void gnome_canvas_construct (GnomeCanvas *canvas, GdkVisual *visual, GdkColormap *colormap);

/* Returns the root canvas item group of the canvas */
GnomeCanvasItem *gnome_canvas_root (GnomeCanvas *canvas);

/* Sets the limits of the scrolling region */
void gnome_canvas_set_scroll_region (GnomeCanvas *canvas, double x1, double y1, double x2, double y2);

/* Sets the number of pixels that correspond to one unit in world coordinates */
void gnome_canvas_set_pixels_per_unit (GnomeCanvas *canvas, double n);

/* Sets the size in pixels of the canvas */
void gnome_canvas_set_size (GnomeCanvas *canvas, int width, int height);

/* Scrolls the canvas so that (x, y) canvas pixel coordinate is on the top-left corner of the window */
void gnome_canvas_scroll_to (GnomeCanvas *canvas, int x, int y);

/* Requests that the canvas be repainted immediately instead of in the idle loop. */
void gnome_canvas_update_now (GnomeCanvas *canvas);

/* For use only by item type implementations.  Request that the canvas eventually redraw the
 * specified region.  The region contains (x1, y1) but not (x2, y2).
 */
void gnome_canvas_request_redraw (GnomeCanvas *canvas, int x1, int y1, int x2, int y2);

/* These functions convert from a coordinate system to another.  "w" is world coordinates (the ones
 * in which objects are specified), "c" is canvas coordinates (pixel coordinates that are (0,0) for
 * the upper-left scrolling limit and something else for the lower-left scrolling limit).
 */
void gnome_canvas_w2c (GnomeCanvas *canvas, double wx, double wy, int *cx, int *cy);
void gnome_canvas_c2w (GnomeCanvas *canvas, int cx, int cy, double *wx, double *wy);

/* Takes a string specification for a color and allocates it into the specified GdkColor.  If the
 * string is null, then it returns FALSE. Otherwise, it returns TRUE.
 */
int gnome_canvas_get_color (GnomeCanvas *canvas, char *spec, GdkColor *color);


END_GNOME_DECLS

#endif
