/* GnomeThemeParser - a parser of icon-theme files
 * gnome-theme-parser.c Copyright (C) 2002 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <string.h>
#include <locale.h>
#include <stdlib.h>

#include "gnome-theme-parser.h"

typedef struct _GnomeThemeFileSection GnomeThemeFileSection;
typedef struct _GnomeThemeFileLine GnomeThemeFileLine;
typedef struct _GnomeThemeFileParser GnomeThemeFileParser;

struct _GnomeThemeFileSection {
  GQuark section_name; /* 0 means just a comment block (before any section) */
  gint n_lines;
  GnomeThemeFileLine *lines;
};

struct _GnomeThemeFileLine {
  GQuark key; /* 0 means comment or blank line in value */
  char *locale;
  gchar *value;
};

struct _GnomeThemeFile {
  gint n_sections;
  GnomeThemeFileSection *sections;
  char *current_locale[2];
};

struct _GnomeThemeFileParser {
  GnomeThemeFile *df;
  gint current_section;
  gint n_allocated_lines;
  gint n_allocated_sections;
  gint line_nr;
  char *line;
};

#define VALID_KEY_CHAR 1
#define VALID_LOCALE_CHAR 2
static const guchar valid[256] = { 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x3 , 0x2 , 0x0 , 
   0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 
   0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x0 , 0x0 , 0x0 , 0x2 , 
   0x0 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 
   0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x3 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
   0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 0x0 , 
};

static void                     report_error   (GnomeThemeFileParser   *parser,
						char                     *message,
						GnomeThemeFileParseError    error_code,
						GError                  **error);
static GnomeThemeFileSection *lookup_section (GnomeThemeFile         *df,
					      const char               *section);
static GnomeThemeFileLine *   lookup_line    (GnomeThemeFile         *df,
					      GnomeThemeFileSection  *section,
					      const char               *keyname,
					      const char               *locale);




/**
 * gnome_theme_file_parse_error_quark:
 * 
 * This function gets  a #GQuark representing the error string. 
 *
 * Returns: a #GQuark.
 *
 * Since: 2.2
 **/
GQuark
gnome_theme_file_parse_error_quark (void)
{
  static GQuark quark;
  if (!quark)
    quark = g_quark_from_static_string ("g_desktop_parse_error");

  return quark;
}

static void
parser_free (GnomeThemeFileParser *parser)
{
  gnome_theme_file_free (parser->df);
}

static void
gnome_theme_file_line_free (GnomeThemeFileLine *line)
{
  g_free (line->locale);
  g_free (line->value);
}

static void
gnome_theme_file_section_free (GnomeThemeFileSection *section)
{
  int i;

  for (i = 0; i < section->n_lines; i++)
    gnome_theme_file_line_free (&section->lines[i]);
  
  g_free (section->lines);
}

/**
 * gnome_theme_file_free:
 * @df: a #GnomeThemeFile.
 * 
 * Frees the #GnomeThemeFile structure.
 *
 * Since: 2.2
 **/
void
gnome_theme_file_free (GnomeThemeFile *df)
{
  int i;

  for (i = 0; i < df->n_sections; i++)
    gnome_theme_file_section_free (&df->sections[i]);
  g_free (df->sections);
  g_free (df->current_locale[0]);
  g_free (df->current_locale[1]);

  g_free (df);
}

static void
grow_lines (GnomeThemeFileParser *parser)
{
  int new_n_lines;
  GnomeThemeFileSection *section;

  if (parser->n_allocated_lines == 0)
    new_n_lines = 1;
  else
    new_n_lines = parser->n_allocated_lines*2;

  section = &parser->df->sections[parser->current_section];

  section->lines = g_realloc (section->lines,
			      sizeof (GnomeThemeFileLine) * new_n_lines);
  parser->n_allocated_lines = new_n_lines;
}

static void
grow_sections (GnomeThemeFileParser *parser)
{
  int new_n_sections;

  if (parser->n_allocated_sections == 0)
    new_n_sections = 1;
  else
    new_n_sections = parser->n_allocated_sections*2;

  parser->df->sections = g_realloc (parser->df->sections,
				    sizeof (GnomeThemeFileSection) * new_n_sections);
  parser->n_allocated_sections = new_n_sections;
}

static gchar *
unescape_string (gchar *str, gint len)
{
  gchar *res;
  gchar *p, *q;
  gchar *end;

  /* len + 1 is enough, because unescaping never makes the
   * string longer */
  res = g_new (gchar, len + 1);
  p = str;
  q = res;
  end = str + len;

  while (p < end)
    {
      if (*p == 0)
	{
	  /* Found an embedded null */
	  g_free (res);
	  return NULL;
	}
      if (*p == '\\')
	{
	  p++;
	  if (p >= end)
	    {
	      /* Escape at end of string */
	      g_free (res);
	      return NULL;
	    }
	  
	  switch (*p)
	    {
	    case 's':
              *q++ = ' ';
              break;
           case 't':
              *q++ = '\t';
              break;
           case 'n':
              *q++ = '\n';
              break;
           case 'r':
              *q++ = '\r';
              break;
           case '\\':
              *q++ = '\\';
              break;
           default:
	     /* Invalid escape code */
	     g_free (res);
	     return NULL;
	    }
	  p++;
	}
      else
	*q++ = *p++;
    }
  *q = 0;

  return res;
}

static gchar *
escape_string (const gchar *str, gboolean escape_first_space)
{
  gchar *res;
  char *q;
  const gchar *p;
  const gchar *end;

  /* len + 1 is enough, because unescaping never makes the
   * string longer */
  res = g_new (gchar, strlen (str)*2 + 1);
  
  p = str;
  q = res;
  end = str + strlen (str);

  while (*p)
    {
      if (*p == ' ')
	{
	  if (escape_first_space && p == str)
	    {
	      *q++ = '\\';
	      *q++ = 's';
	    }
	  else
	    *q++ = ' ';
	}
      else if (*p == '\\')
	{
	  *q++ = '\\';
	  *q++ = '\\';
	}
      else if (*p == '\t')
	{
	  *q++ = '\\';
	  *q++ = 't';
	}
      else if (*p == '\n')
	{
	  *q++ = '\\';
	  *q++ = 'n';
	}
      else if (*p == '\r')
	{
	  *q++ = '\\';
	  *q++ = 'r';
	}
      else
	*q++ = *p;
      p++;
    }
  *q = 0;

  return res;
}


static void 
open_section (GnomeThemeFileParser *parser,
	      const char           *name)
{
  int n;
  
  if (parser->n_allocated_sections == parser->df->n_sections)
    grow_sections (parser);

  if (parser->current_section == 0 &&
      parser->df->sections[0].section_name == 0 &&
      parser->df->sections[0].n_lines == 0)
    {
      if (!name)
	g_warning ("non-initial NULL section\n");
      
      /* The initial section was empty. Piggyback on it. */
      parser->df->sections[0].section_name = g_quark_from_string (name);

      return;
    }
  
  n = parser->df->n_sections++;

  if (name)
    parser->df->sections[n].section_name = g_quark_from_string (name);
  else
    parser->df->sections[n].section_name = 0;
  parser->df->sections[n].n_lines = 0;
  parser->df->sections[n].lines = NULL;

  parser->current_section = n;
  parser->n_allocated_lines = 0;
  grow_lines (parser);
}

static GnomeThemeFileLine *
new_line (GnomeThemeFileParser *parser)
{
  GnomeThemeFileSection *section;
  GnomeThemeFileLine *line;

  section = &parser->df->sections[parser->current_section];
  
  if (parser->n_allocated_lines == section->n_lines)
    grow_lines (parser);

  line = &section->lines[section->n_lines++];

  memset (line, 0, sizeof (GnomeThemeFileLine));
  
  return line;
}

static gboolean
is_blank_line (GnomeThemeFileParser *parser)
{
  gchar *p;

  p = parser->line;

  while (*p && *p != '\n')
    {
      if (!g_ascii_isspace (*p))
	return FALSE;

      p++;
    }
  return TRUE;
}

static void
parse_comment_or_blank (GnomeThemeFileParser *parser)
{
  GnomeThemeFileLine *line;
  gchar *line_end;

  line_end = strchr (parser->line, '\n');
  if (line_end == NULL)
    line_end = parser->line + strlen (parser->line);

  line = new_line (parser);
  
  line->value = g_strndup (parser->line, line_end - parser->line);

  parser->line = (line_end) ? line_end + 1 : NULL;
  parser->line_nr++;
}

static gboolean
parse_section_start (GnomeThemeFileParser *parser, GError **error)
{
  gchar *line_end;
  gchar *section_name;

  line_end = strchr (parser->line, '\n');
  if (line_end == NULL)
    line_end = parser->line + strlen (parser->line);

  if (line_end - parser->line <= 2 ||
      line_end[-1] != ']')
    {
      report_error (parser, "Invalid syntax for section header", GNOME_THEME_FILE_PARSE_ERROR_INVALID_SYNTAX, error);
      parser_free (parser);
      return FALSE;
    }

  section_name = unescape_string (parser->line + 1, line_end - parser->line - 2);

  if (section_name == NULL)
    {
      report_error (parser, "Invalid escaping in section name", GNOME_THEME_FILE_PARSE_ERROR_INVALID_ESCAPES, error);
      parser_free (parser);
      return FALSE;
    }

  open_section (parser, section_name);
  
  parser->line = (line_end) ? line_end + 1 : NULL;
  parser->line_nr++;

  g_free (section_name);
  
  return TRUE;
}

static gboolean
parse_key_value (GnomeThemeFileParser *parser, GError **error)
{
  GnomeThemeFileLine *line;
  gchar *line_end;
  gchar *key_start;
  gchar *key_end;
  gchar *key;
  gchar *locale_start = NULL;
  gchar *locale_end = NULL;
  gchar *value_start;
  gchar *value;
  gchar *p;

  line_end = strchr (parser->line, '\n');
  if (line_end == NULL)
    line_end = parser->line + strlen (parser->line);

  p = parser->line;
  key_start = p;
  while (p < line_end &&
	 (valid[(guchar)*p] & VALID_KEY_CHAR)) 
    p++;
  key_end = p;

  if (key_start == key_end)
    {
      report_error (parser, "Empty key name", GNOME_THEME_FILE_PARSE_ERROR_INVALID_SYNTAX, error);
      parser_free (parser);
      return FALSE;
    }

  if (p < line_end && *p == '[')
    {
      p++;
      locale_start = p;
      while (p < line_end && *p != ']')
	p++;
      locale_end = p;

      if (p == line_end)
	{
	  report_error (parser, "Unterminated locale specification in key", GNOME_THEME_FILE_PARSE_ERROR_INVALID_SYNTAX, error);
	  parser_free (parser);
	  return FALSE;
	}
      
      p++;
    }
  
  /* Skip space before '=' */
  while (p < line_end && *p == ' ')
    p++;

  if (p < line_end && *p != '=')
    {
      report_error (parser, "Invalid characters in key name", GNOME_THEME_FILE_PARSE_ERROR_INVALID_CHARS, error);
      parser_free (parser);
      return FALSE;
    }

  if (p == line_end)
    {
      report_error (parser, "No '=' in key/value pair", GNOME_THEME_FILE_PARSE_ERROR_INVALID_SYNTAX, error);
      parser_free (parser);
      return FALSE;
    }

  /* Skip the '=' */
  p++;

  /* Skip space after '=' */
  while (p < line_end && *p == ' ')
    p++;

  value_start = p;

  value = unescape_string (value_start, line_end - value_start);
  if (value == NULL)
    {
      report_error (parser, "Invalid escaping in value", GNOME_THEME_FILE_PARSE_ERROR_INVALID_ESCAPES, error);
      parser_free (parser);
      return FALSE;
    }

  line = new_line (parser);
  key = g_strndup (key_start, key_end - key_start);
  line->key = g_quark_from_string (key);
  g_free (key);
  if (locale_start)
    line->locale = g_strndup (locale_start, locale_end - locale_start);
  line->value = value;
  
  parser->line = (line_end) ? line_end + 1 : NULL;
  parser->line_nr++;
  
  return TRUE;
}


static void
report_error (GnomeThemeFileParser *parser,
	      char                   *message,
	      GnomeThemeFileParseError  error_code,
	      GError                **error)
{
  GnomeThemeFileSection *section;
  const gchar *section_name = NULL;

  section = &parser->df->sections[parser->current_section];

  if (section->section_name)
    section_name = g_quark_to_string (section->section_name);
  
  if (error)
    {
      if (section_name)
	*error = g_error_new (GNOME_THEME_FILE_PARSE_ERROR,
			      error_code,
			      "Error in section %s at line %d: %s", section_name, parser->line_nr, message);
      else
	*error = g_error_new (GNOME_THEME_FILE_PARSE_ERROR,
			      error_code,
			      "Error at line %d: %s", parser->line_nr, message);
    }
}

/**
 * gnome_theme_file_new_from_string:
 * @data:  the string used to create a #GnomeThemeFile.
 * @error: location to store the error occuring, or NULL to ignore errors 
 *
 * Creates a #GnomeThemeFile from the data string passed.
 * 
 * Returns: a #GnomeThemeFile.
 * 
 * Since: 2.2
 **/
GnomeThemeFile *
gnome_theme_file_new_from_string (char                       *data,
				  GError                    **error)
{
  GnomeThemeFileParser parser;

  parser.df = g_new0 (GnomeThemeFile, 1);
  parser.current_section = -1;

  parser.n_allocated_lines = 0;
  parser.n_allocated_sections = 0;
  parser.line_nr = 1;

  parser.line = data;

  /* Put any initial comments in a NULL segment */
  open_section (&parser, NULL);
  
  while (parser.line && *parser.line)
    {
      if (*parser.line == '[') {
	if (!parse_section_start (&parser, error))
	  return NULL;
      } else if (is_blank_line (&parser) ||
		 *parser.line == '#')
	parse_comment_or_blank (&parser);
      else
	{
	  if (!parse_key_value (&parser, error))
	    return NULL;
	}
    }

  return parser.df;
}

/**
 * gnome_theme_file_to_string:
 * @df: A #GnomeThemeFile
 *
 * This function retrieves the string representing the #GnomeThemeFile.
 *
 * Returns: a char *.
 *
 * Since: 2.2
 **/
char *
gnome_theme_file_to_string (GnomeThemeFile *df)
{
  GnomeThemeFileSection *section;
  GnomeThemeFileLine *line;
  GString *str;
  char *s;
  int i, j;
  
  str = g_string_sized_new (800);

  for (i = 0; i < df->n_sections; i ++)
    {
      section = &df->sections[i];

      if (section->section_name)
	{
	  g_string_append_c (str, '[');
	  s = escape_string (g_quark_to_string (section->section_name), FALSE);
	  g_string_append (str, s);
	  g_free (s);
	  g_string_append (str, "]\n");
	}
      
      for (j = 0; j < section->n_lines; j++)
	{
	  line = &section->lines[j];
	  
	  if (line->key == 0)
	    {
	      g_string_append (str, line->value);
	      g_string_append_c (str, '\n');
	    }
	  else
	    {
	      g_string_append (str, g_quark_to_string (line->key));
	      if (line->locale)
		{
		  g_string_append_c (str, '[');
		  g_string_append (str, line->locale);
		  g_string_append_c (str, ']');
		}
	      g_string_append_c (str, '=');
	      s = escape_string (line->value, TRUE);
	      g_string_append (str, s);
	      g_free (s);
	      g_string_append_c (str, '\n');
	    }
	}
    }
  
  return g_string_free (str, FALSE);
}

static GnomeThemeFileSection *
lookup_section (GnomeThemeFile  *df,
		const char        *section_name)
{
  GnomeThemeFileSection *section;
  GQuark section_quark;
  int i;

  section_quark = g_quark_try_string (section_name);
  if (section_quark == 0)
    return NULL;
  
  for (i = 0; i < df->n_sections; i ++)
    {
      section = &df->sections[i];

      if (section->section_name == section_quark)
	return section;
    }
  return NULL;
}

static GnomeThemeFileLine *
lookup_line (GnomeThemeFile        *df,
	     GnomeThemeFileSection *section,
	     const char              *keyname,
	     const char              *locale)
{
  GnomeThemeFileLine *line;
  GQuark key_quark;
  int i;

  key_quark = g_quark_try_string (keyname);
  if (key_quark == 0)
    return NULL;
  
  for (i = 0; i < section->n_lines; i++)
    {
      line = &section->lines[i];
      
      if (line->key == key_quark &&
	  ((locale == NULL && line->locale == NULL) ||
	   (locale != NULL && line->locale != NULL && strcmp (locale, line->locale) == 0)))
	return line;
    }
  
  return NULL;
}

/**
 * gnome_theme_file_get_raw:
 * @df: A #GnomeThemeFile. 
 * @section: the string representing the section name
 * @keyname: the string representing the key name.
 * @locale: the string representing the locale. 
 * @val: a char**. 
 *
 * Searches section name and line in the #GnomeThemeFile data structure.
 * If found, sets the @val to value field in GnomeThemeFileLine and returns a boolean value.
 * 
 * Returns: TRUE if section and line were found in the #GnomeThemeFile, FALSE otherwise.
 * 
 * Since: 2.2
 **/ 
gboolean
gnome_theme_file_get_raw (GnomeThemeFile	*df,
			    const char		*section_name,
			    const char    	*keyname,
			    const char    	*locale,
			    char         	**val)
{
  GnomeThemeFileSection *section;
  GnomeThemeFileLine *line;

  *val = NULL;

  section = lookup_section (df, section_name);
  if (!section)
    return FALSE;

  line = lookup_line (df,
		      section,
		      keyname,
		      locale);

  if (!line)
    return FALSE;
  
  *val = g_strdup (line->value);
  
  return TRUE;
}

/**
 * gnome_theme_file_foreach_section:
 * @df: a #GnomeThemeFile.
 * @func: a #GnomeThemeFileSectionFunc
 * @user_data: a pointer to the user data.
 * 
 * Calls @func for each section in the #GnomeThemeFile with @user_data.
 *
 * Since: 2.2
 **/
void
gnome_theme_file_foreach_section (GnomeThemeFile            *df,
				  GnomeThemeFileSectionFunc  func,
				  gpointer                     user_data)
{
  GnomeThemeFileSection *section;
  int i;

  for (i = 0; i < df->n_sections; i ++)
    {
      section = &df->sections[i];

      (*func) (df, g_quark_to_string (section->section_name),  user_data);
    }
  return;
}

/**
 * gnome_theme_file_foreach_key:
 * @df: a #GnomeThemeFile.
 * @section: name of the section
 * @include_localized: a boolean value
 * @func: a #GnomeThemeFileLineFunc.
 * @user_data: a pointer to user_data.
 * 
 * Looks for the section @section_name. If found, this function calls @func for each line 
 * in the section with fields of line and @user_data.
 * 
 * Since: 2.2
 */ 
void
gnome_theme_file_foreach_key (GnomeThemeFile            *df,
			      const char		*section_name,
			      gboolean			include_localized,
			      GnomeThemeFileLineFunc	func,
			      gpointer			user_data)
{
  GnomeThemeFileSection *section;
  GnomeThemeFileLine *line;
  int i;

  section = lookup_section (df, section_name);
  if (!section)
    return;
  
  for (i = 0; i < section->n_lines; i++)
    {
      line = &section->lines[i];

      (*func) (df, g_quark_to_string (line->key), line->locale, line->value, user_data);
    }
  
  return;
}


static void
calculate_locale (GnomeThemeFile   *df)
{
  char *p, *lang;

#ifndef G_OS_WIN32
  lang = g_strdup (setlocale (LC_MESSAGES, NULL));
#else
  lang = g_win32_getlocale ();
#endif
  
  if (lang)
    {
      p = strchr (lang, '.');
      if (p)
	*p = '\0';
      p = strchr (lang, '@');
      if (p)
	*p = '\0';
    }
  else
    lang = g_strdup ("C");
  
  p = strchr (lang, '_');
  if (p)
    {
      df->current_locale[0] = g_strdup (lang);
      *p = '\0';
      df->current_locale[1] = lang;
    }
  else
    {
      df->current_locale[0] = lang;
      df->current_locale[1] = NULL;
    }
}

/**
 * gnome_theme_file_get_locale_string:
 * @df: A #GnomeThemeFile
 * @section: the section name.
 * @keyname: the keyname.
 * @val: a char **.
 *
 * Calculates the locale if the current_locale field of @df is NULL. Then calls gnome_theme_file_get_raw() with the 
 * parameters and returns the boolean value obtained.
 *
 * Returns: a gboolean value.
 *
 * Since: 2.2
 */
gboolean
gnome_theme_file_get_locale_string  (GnomeThemeFile  *df,
				     const char      *section,
				     const char      *keyname,
				     char           **val)
{
  gboolean res;

  if (df->current_locale[0] == NULL)
    calculate_locale (df);

  if  (df->current_locale[0] != NULL)
    {
      res = gnome_theme_file_get_raw (df,section, keyname,
				      df->current_locale[0], val);
      if (res)
	return TRUE;
    }
  
  if  (df->current_locale[1] != NULL)
    {
      res = gnome_theme_file_get_raw (df,section, keyname,
				      df->current_locale[1], val);
      if (res)
	return TRUE;
    }
  
  return gnome_theme_file_get_raw (df, section, keyname, NULL, val);
}

/**
 * gnome_theme_file_get_string:
 * @df: A #GnomeThemeFile.
 * @section: the section name. 
 * @keyname: the key name.
 * @val: a char**.
 *
 * This function calls gnome_theme_file_get_raw() with the parameters and returns the 
 * boolean value.
 *
 * Returns: a #gboolean value.
 *
 * Since: 2.2
 **/
gboolean
gnome_theme_file_get_string (GnomeThemeFile   *df,
			     const char       *section,
			     const char       *keyname,
			     char            **val)
{
  return gnome_theme_file_get_raw (df, section, keyname, NULL, val);
}

/**
 * gnome_theme_file_get_integer:
 * @df: a #GnomeThemeFile.
 * @section: the section name.
 * @keyname: the key name.
 * @val: an int*.
 *
 * This function calls gnome_theme_file_get_raw() with the parameters. If gnome_theme_file_get_raw returns
 * %TRUE, then converts the value string filled in by the function to an integer and writes it in @val.
 * 
 * Returns: %TRUE if gnome_theme_file_get_raw returns %TRUE, %FALSE otherwise. 
 *
 * Since: 2.2
 **/ 
gboolean
gnome_theme_file_get_integer (GnomeThemeFile   *df,
			      const char       *section,
			      const char       *keyname,
			      int              *val)
{
  gboolean res;
  char *str;
  
  *val = 0;

  res = gnome_theme_file_get_raw (df, section, keyname, NULL, &str);
  if (!res)
    return FALSE;

  
  *val = atoi (str);
  g_free (str);
  
  return TRUE;
  
}

