/*
 * gnome-icon-text.c: Formats and wraps around text to be used as
 * icon captions.
 *
 * Author:
 *    Federico Mena <federico@nuclecu.unam.mx>
 */
#include <config.h>
#include <gtk/gtk.h>
#include <string.h>
#include <libgnomeui/gnome-icon-text.h>

static void
free_row (gpointer data, gpointer user_data)
{
	GnomeIconTextInfoRow *row;

	if (data) {
		row = data;
		g_free (row->text);
		g_free (row);
	}
}

/**
 * gnome_icon_text_info_free:
 * @ti: An icon text info structure.
 *
 * Frees a &GnomeIconTextInfo structure.  You should call this instead of
 * freeing the structure yourself.
 */
void
gnome_icon_text_info_free (GnomeIconTextInfo *ti)
{
	g_list_foreach (ti->rows, free_row, NULL);
	g_list_free (ti->rows);
	g_free (ti);
}

/**
 * gnome_icon_layout_text:
 * @font:       Name of the font that will be used to render the text.
 * @text:       Text to be formatted.
 * @separators: Separators used for word wrapping, can be NULL.
 * @max_width:  Width in pixels to be used for word wrapping.
 * @confine:    Whether it is mandatory to wrap at @max_width.
 *
 * Creates a new &GnomeIconTextInfo structure by wrapping the specified
 * text.  If non-NULL, the @separators argument defines a set of characters
 * to be used as word delimiters for performing word wrapping.  If it is
 * NULL, then only spaces will be used as word delimiters.
 *
 * The @max_width argument is used to specify the width at which word
 * wrapping will be performed.  If there is a very long word that does not
 * fit in a single line, the @confine argument can be used to specify
 * whether the word should be unconditionally split to fit or whether
 * the maximum width should be increased as necessary.
 *
 * Return value: A newly-created &GnomeIconTextInfo structure.
 */
GnomeIconTextInfo *
gnome_icon_layout_text (GdkFont *font, char *text, char *separators, int max_width, int confine)
{
	GnomeIconTextInfo *ti;
	GnomeIconTextInfoRow *row;
	char *row_end;
	char *s, *word_start, *word_end, *old_word_end;
	char *sub_text;
	int sub_len;
	int i, w_len, w;

	g_return_val_if_fail (font != NULL, NULL);
	g_return_val_if_fail (text != NULL, NULL);

	if (!separators)
		separators = " ";

	ti = g_new (GnomeIconTextInfo, 1);

	ti->rows = NULL;
	ti->font = font;
	ti->width = 0;
	ti->height = 0;
	ti->baseline_skip = font->ascent + font->descent;

	word_end = NULL;

	while (*text) {
		row_end = strchr (text, '\n');
		if (!row_end)
			row_end = strchr (text, '\0');

		/* Accumulate words from this row until they don't fit in the max_width */

		s = text;

		while (s < row_end) {
			word_start = s;
			old_word_end = word_end;
			word_end = word_start + strcspn (word_start, separators) + 1;

			if (word_end > row_end)
				word_end = row_end;

			if (gdk_text_width (font, text, word_end - text) > max_width) {
				if (word_start == text) {
					if (confine) {
						/* We must force-split the word.  Look for a proper
                                                 * place to do it.
						 */

						w_len = word_end - word_start;

						for (i = 1; i < w_len; i++) {
							w = gdk_text_width (font, word_start, i);
							if (w > max_width) {
								if (i == 1)
									/* Shit, not even a single character fits */
									max_width = w;
								else
									break;
							}
						}

						/* Create sub-row with the chars that fit */

						sub_text = g_malloc (i * sizeof (char));
						memcpy (sub_text, word_start, (i - 1) * sizeof (char));
						sub_text[i - 1] = 0;

						row = g_new (GnomeIconTextInfoRow, 1);
						row->text = sub_text;
						row->width = gdk_string_width (font, sub_text);

						ti->rows = g_list_append (ti->rows, row);

						if (row->width > ti->width)
							ti->width = row->width;

						ti->height += ti->baseline_skip;

						/* Bump the text pointer */

						text += i - 1;
						s = text;

						continue;
					} else
						max_width = gdk_text_width (font, word_start, word_end - word_start);

					continue; /* Retry split */
				} else {
					word_end = old_word_end; /* Restore to region that does fit */
					break; /* Stop the loop because we found something that doesn't fit */
				}
			}

			s = word_end;
		}

		/* Append row */

		if (text == row_end) {
			/* We are on a newline, so append an empty row */

			ti->rows = g_list_append (ti->rows, NULL);
			ti->height += ti->baseline_skip / 2;

			/* Next! */

			text = row_end + 1;
		} else {
			/* Create subrow and append it to the list */

			sub_len = word_end - text;

			sub_text = g_malloc ((sub_len + 1) * sizeof (char));
			memcpy (sub_text, text, sub_len * sizeof (char));
			sub_text[sub_len] = 0;

			row = g_new (GnomeIconTextInfoRow, 1);
			row->text = sub_text;
			row->width = gdk_string_width (font, sub_text);

			ti->rows = g_list_append (ti->rows, row);

			if (row->width > ti->width)
				ti->width = row->width;

			ti->height += ti->baseline_skip;

			/* Next! */

			text = word_end;
		}
	}

	return ti;
}

/**
 * gnome_icon_paint_text:
 * @ti:       An icon text info structure.
 * @drawable: Target drawable.
 * @gc:       GC used to render the string.
 * @x:        Left coordinate for text.
 * @y:        Upper coordinate for text.
 * @just:     Justification for text.
 *
 * Paints the formatted text in the icon text info structure onto a drawable.
 * This is just a sample implementation; applications can choose to use other
 * rendering functions.
 */
void
gnome_icon_paint_text (GnomeIconTextInfo *ti, GdkDrawable *drawable, GdkGC *gc,
		       int x, int y, GtkJustification just)
{
	GList *item;
	GnomeIconTextInfoRow *row;
	int xpos;

	g_return_if_fail (ti != NULL);
	g_return_if_fail (drawable != NULL);
	g_return_if_fail (gc != NULL);

	y += ti->font->ascent;

	for (item = ti->rows; item; item = item->next) {
		if (item->data) {
			row = item->data;

			switch (just) {
			case GTK_JUSTIFY_LEFT:
				xpos = 0;
				break;

			case GTK_JUSTIFY_RIGHT:
				xpos = ti->width - row->width;
				break;

			case GTK_JUSTIFY_CENTER:
				xpos = (ti->width - row->width) / 2;
				break;

			default:
				/* Anyone care to implement GTK_JUSTIFY_FILL? */
				g_warning ("Justification type %d not supported.  Using left-justification.",
					   (int) just);
				xpos = 0;
			}

			gdk_draw_string (drawable, ti->font, gc, x + xpos, y, row->text);

			y += ti->baseline_skip;
		} else
			y += ti->baseline_skip / 2;
	}
}
