/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <glib.h>
#include <glib/gstdio.h> /* g_stat() */
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlmemory.h>

#include "intl.h"
#include "sheet.h"
#include "message.h"
#include "object.h"
#include "object-alias.h"
#include "dia_dirs.h"
#include "dia-io.h"

static GSList *sheets = NULL;

Sheet *
new_sheet (char       *name,
           char       *description,
           char       *filename,
           SheetScope  scope,
           Sheet      *shadowing)
{
  Sheet *sheet;

  sheet = g_new (Sheet, 1);

  sheet->name = g_strdup (name);
  sheet->description = g_strdup (description);

  sheet->filename = filename;
  sheet->scope = scope;
  sheet->shadowing = shadowing;
  sheet->objects = NULL;
  return sheet;
}

void
sheet_prepend_sheet_obj (Sheet       *sheet,
                         SheetObject *obj)
{
  DiaObjectType *type;

  type = object_get_type (obj->object_type);
  if (type == NULL) {
    message_warning (_("DiaObject '%s' needed in sheet '%s' was not found.\n"
                       "It will not be available for use."),
                       obj->object_type, sheet->name);
  } else {
    sheet->objects = g_slist_prepend (sheet->objects, (gpointer) obj);
  }
}

void
sheet_append_sheet_obj (Sheet       *sheet,
                        SheetObject *obj)
{
  DiaObjectType *type;

  type = object_get_type (obj->object_type);
  if (type == NULL) {
    message_warning (_("DiaObject '%s' needed in sheet '%s' was not found.\n"
                       "It will not be available for use."),
                       obj->object_type, sheet->name);
  } else {
    sheet->objects = g_slist_append (sheet->objects, (gpointer) obj);
  }
}

void
register_sheet (Sheet *sheet)
{
  sheets = g_slist_append (sheets, (gpointer) sheet);
}

GSList *
get_sheets_list (void)
{
  return sheets;
}

/* Sheet file management */

static void load_sheets_from_dir (const char *directory,
                                  SheetScope  scope);
static void load_register_sheet  (const char *directory,
                                  const char *filename,
                                  SheetScope  scope);


/**
 * dia_sheet_sort_callback:
 * @a: #Sheet A
 * @b: #Sheet B
 *
 * Sort the list of sheets by *locale*.
 */
static int
dia_sheet_sort_callback (gconstpointer a, gconstpointer b)
{
  // TODO: Don't gettext random strings
  return g_utf8_collate (gettext (((Sheet *) (a))->name),
                         gettext (((Sheet *) (b))->name));
}


void
dia_sort_sheets (void)
{
  sheets = g_slist_sort (sheets, dia_sheet_sort_callback);
}

void
load_all_sheets (void)
{
  char *sheet_path;
  char *home_dir;

  home_dir = dia_config_filename ("sheets");
  if (home_dir) {
    dia_log_message ("sheets from '%s'", home_dir);
    load_sheets_from_dir (home_dir, SHEET_SCOPE_USER);
    g_clear_pointer (&home_dir, g_free);
  }

  sheet_path = getenv ("DIA_SHEET_PATH");
  if (sheet_path) {
    char **dirs = g_strsplit (sheet_path, G_SEARCHPATH_SEPARATOR_S, 0);
    int i;

    for (i = 0; dirs[i] != NULL; i++) {
      dia_log_message ("sheets from '%s'", dirs[i]);
      load_sheets_from_dir (dirs[i], SHEET_SCOPE_SYSTEM);
    }
    g_strfreev (dirs);
  } else {
    char *thedir = dia_get_data_directory ("sheets");
    dia_log_message ("sheets from '%s'", thedir);
    load_sheets_from_dir (thedir, SHEET_SCOPE_SYSTEM);
    g_clear_pointer (&thedir, g_free);
  }

  /* Sorting their sheets alphabetically makes user merging easier */

  dia_sort_sheets ();
}


static void
load_sheets_from_dir (const char *directory,
                      SheetScope  scope)
{
  GDir *dp;
  const char *dentry;
  char *p;

  dp = g_dir_open (directory, 0, NULL);
  if (!dp) {
    return;
  }

  while ((dentry = g_dir_read_name (dp))) {
    char *filename = g_strconcat (directory, G_DIR_SEPARATOR_S, dentry, NULL);

    if (!g_file_test (filename, G_FILE_TEST_IS_REGULAR)) {
      g_clear_pointer (&filename, g_free);
      continue;
    }

    /* take only .sheet files */
    p = filename + strlen (filename) - 6 /* strlen(".sheet") */;
    if (0 != strncmp (p, ".sheet", 6)) {
      g_clear_pointer (&filename, g_free);
      continue;
    }

    load_register_sheet (directory, filename, scope);
    g_clear_pointer (&filename, g_free);
  }

  g_dir_close (dp);
}


static void
load_register_sheet (const char *dirname,
                     const char *filename,
                     SheetScope  scope)
{
  DiaContext *ctx = dia_context_new (_("Load Sheet"));
  xmlDocPtr doc = NULL;
  xmlNsPtr ns;
  xmlNodePtr node, contents,subnode,root;
  xmlChar *tmp;
  char *sheetdir = NULL;
  char *name = NULL, *description = NULL;
  int name_score = -1;
  int descr_score = -1;
  Sheet *sheet = NULL;
  GSList *sheetp;
  gboolean set_line_break = FALSE;
  gboolean name_is_gmalloced = FALSE;
  Sheet *shadowing = NULL;
  Sheet *shadowing_sheet = NULL;

  /* the XML fun begins here. */

  dia_context_set_filename (ctx, filename);

  doc = dia_io_load_document (filename, ctx, NULL);
  if (!doc) {
    dia_context_add_message (ctx,
                             _("Loading Sheet from %s failed"),
                             filename);

    goto out;
  }
  root = doc->xmlRootNode;

  while (root && (root->type != XML_ELEMENT_NODE)) {
    root = root->next;
  }

  if (!root || xmlIsBlankNode (root)) {
    goto out;
  }

  if (!(ns = xmlSearchNsByHref (doc, root, (const xmlChar *)
                                DIA_XML_NAME_SPACE_BASE "dia-sheet-ns"))) {
    dia_context_add_message (ctx, _("Could not find sheet namespace"));

    goto out;
  }

  if ((root->ns != ns) || (xmlStrcmp (root->name, (const xmlChar *) "sheet"))) {
    dia_context_add_message (ctx,
                             _("root element was %s — expecting sheet"),
                             doc->xmlRootNode->name);

    goto out;
  }

  contents = NULL;
  for (node = root->xmlChildrenNode; node != NULL; node = node->next) {
    if (xmlIsBlankNode (node)) {
      continue;
    }

    if (node->type != XML_ELEMENT_NODE) {
      continue;
    }

    if (node->ns == ns && !xmlStrcmp (node->name, (const xmlChar *) "name")) {
      int score;

      /* compare the xml:lang property on this element to see if we get a
       * better language match.  LibXML seems to throw away attribute
       * namespaces, so we use "lang" instead of "xml:lang" */
      /* Now using the C locale for internal sheet names, instead gettexting
       * when the name is used in the menus.  Not going to figure out the
       * XML lang system more than absolutely necessary now.   --LC
       */
      /*
      tmp = xmlGetProp(node, "xml:lang");
      if (!tmp) tmp = xmlGetProp(node, "lang");
      */
      score = intl_score_locale ("C");
      /*
      if (tmp) xmlFree(tmp);
      */

      if (name_score < 0 || score < name_score) {
        name_score = score;
        dia_clear_xml_string (&name);
        name = (char *) xmlNodeGetContent (node);
      }
    } else if (node->ns == ns && !xmlStrcmp (node->name, (const xmlChar *) "description")) {
      int score;

      /* compare the xml:lang property on this element to see if we get a
       * better language match.  LibXML seems to throw away attribute
       * namespaces, so we use "lang" instead of "xml:lang" */
      tmp = xmlGetProp (node, (const xmlChar *) "xml:lang");
      if (!tmp) {
        tmp = xmlGetProp (node, (const xmlChar *) "lang");
      }
      score = intl_score_locale ((char *) tmp);
      dia_clear_xml_string (&tmp);

      if (descr_score < 0 || score < descr_score) {
        descr_score = score;
        dia_clear_xml_string (&description);
        description = (char *) xmlNodeGetContent (node);
      }
    } else if (node->ns == ns && !xmlStrcmp (node->name, (const xmlChar *) "contents")) {
      contents = node;
    }
  }

  if (!name || !contents) {
    dia_context_add_message (ctx,
                             _("No <name> and/or <contents> in sheet %s — skipping"),
                             filename);

    goto out;
  }

  /* Notify the user when we load a sheet that appears to be an updated
     version of a sheet loaded previously (i.e. from ~/.dia/sheets). */

  sheetp = get_sheets_list ();
  while (sheetp) {
    if (sheetp->data && !strcmp (((Sheet *) (sheetp->data))->name, name)) {
      GStatBuf first_file, this_file;
      int stat_ret;

      stat_ret = g_stat (((Sheet *) (sheetp->data))->filename, &first_file);
      g_assert (!stat_ret);

      stat_ret = g_stat (filename, &this_file);
      g_assert (!stat_ret);

      if (this_file.st_mtime > first_file.st_mtime) {
        char *tmp2 = g_strdup_printf ("%s [Copy of system]", name);
        message_notice (_("The system sheet '%s' appears to be more recent"
                          " than your custom\n"
                          "version and has been loaded as '%s' for this session."
                          "\n\n"
                          "Move new objects (if any) from '%s' into your custom"
                          " sheet\n"
                          "or remove '%s', using the 'Sheets and Objects' dialog."),
                          name, tmp2, tmp2, tmp2);
        dia_clear_xml_string (&name);
        name = tmp2;
        name_is_gmalloced = TRUE;
        shadowing = sheetp->data;  /* This copy-of-system sheet shadows
                                      a user sheet */
      } else {
        /* The already-created user sheet shadows this sheet (which will be
           invisible), but we don't know this sheet's address yet */
        shadowing_sheet = sheetp->data;
      }
    }
    sheetp = g_slist_next (sheetp);
  }

  sheet = new_sheet (name, description, g_strdup (filename), scope, shadowing);

  if (shadowing_sheet) {
    shadowing_sheet->shadowing = sheet;                   /* Hilarious :-) */
  }

  if (name_is_gmalloced == TRUE) {
    g_clear_pointer (&name, g_free);
  } else {
    dia_clear_xml_string (&name);
  }
  dia_clear_xml_string (&description);

  sheetdir = dia_get_data_directory ("sheets");

  for (node = contents->xmlChildrenNode; node != NULL; node = node->next) {
    SheetObject *sheet_obj;
    DiaObjectType *otype;
    char *iconname = NULL;

    int subdesc_score = -1;
    xmlChar *objdesc = NULL;

    int intdata = 0;

    gboolean has_intdata = FALSE;
    gboolean has_icon_on_sheet = FALSE;

    xmlChar *ot_name = NULL;

    if (xmlIsBlankNode (node)) {
      continue;
    }

    if (node->type != XML_ELEMENT_NODE) {
      continue;
    }

    if (node->ns != ns) {
      continue;
    }

    if (!xmlStrcmp (node->name, (const xmlChar *) "object")) {
      /* nothing */
    } else if (!xmlStrcmp (node->name, (const xmlChar *) "shape")) {
      g_message (_("%s: you should use object tags rather than shape tags now"),
                 filename);
    } else if (!xmlStrcmp (node->name, (const xmlChar *) "br")) {
      /* Line break tag. */
      set_line_break = TRUE;
      continue;
    } else {
      continue; /* unknown tag */
    }

    tmp = xmlGetProp (node, (const xmlChar *) "intdata");
    if (tmp) {
      char *p;
      intdata = (int) strtol ((char *) tmp, &p, 0);
      if (*p != 0) intdata = 0;
      dia_clear_xml_string (&tmp);
      has_intdata = TRUE;
    }

    ot_name = xmlGetProp (node, (xmlChar *) "name");

    for (subnode = node->xmlChildrenNode;
         subnode != NULL ;
         subnode = subnode->next) {
      if (xmlIsBlankNode (subnode)) {
        continue;
      }

      if (subnode->ns == ns && !xmlStrcmp (subnode->name, (const xmlChar *) "description")) {
        int score;

        /* compare the xml:lang property on this element to see if we get a
        * better language match.  LibXML seems to throw away attribute
        * namespaces, so we use "lang" instead of "xml:lang" */

        tmp = xmlGetProp (subnode, (xmlChar *) "xml:lang");
        if (!tmp) {
          tmp = xmlGetProp (subnode, (xmlChar *) "lang");
        }
        score = intl_score_locale ((char *) tmp);
        dia_clear_xml_string (&tmp);

        if (subdesc_score < 0 || score < subdesc_score) {
          subdesc_score = score;
          dia_clear_xml_string (&objdesc);
          objdesc = xmlNodeGetContent (subnode);
        }
      } else if (subnode->ns == ns && !xmlStrcmp (subnode->name, (const xmlChar *) "icon")) {
        tmp = xmlNodeGetContent (subnode);
        if (g_str_has_prefix ((char *) tmp, "res:")) {
          iconname = g_strdup ((char *) tmp);
        } else {
          iconname = g_build_filename (dirname, (char *) tmp, NULL);
          if (!shadowing_sheet && !g_file_test (iconname, G_FILE_TEST_EXISTS)) {
            g_free (iconname);
            /* Fall back to system directory if there is no user icon */
            iconname = g_build_filename (sheetdir, (char *) tmp, NULL);
          }
        }
        has_icon_on_sheet = TRUE;
        dia_clear_xml_string (&tmp);
      } else if (subnode->ns == ns && !xmlStrcmp (subnode->name, (const xmlChar *) "alias")) {
        if (ot_name) {
          object_register_alias_type (object_get_type ((char *) ot_name), subnode);
        }
      }
    }

    sheet_obj = g_new (SheetObject, 1);
    sheet_obj->object_type = g_strdup ((char *) ot_name);
    sheet_obj->description = g_strdup ((char *) objdesc);

    dia_clear_xml_string (&objdesc);

    sheet_obj->user_data = GINT_TO_POINTER (intdata); /* XXX modify user_data type ? */
    sheet_obj->user_data_type = has_intdata ? USER_DATA_IS_INTDATA /* sure,   */
                                            : USER_DATA_IS_OTHER;  /* why not */
    if (iconname && g_str_has_prefix (iconname, "res:")) {
      // Apparently we hate the world
      sheet_obj->pixmap = (const char **) iconname;
      sheet_obj->pixmap_file = NULL;
    } else {
      sheet_obj->pixmap = NULL;
      sheet_obj->pixmap_file = iconname;
    }
    sheet_obj->has_icon_on_sheet = has_icon_on_sheet;
    sheet_obj->line_break = set_line_break;
    set_line_break = FALSE;

    if ((otype = object_get_type ((char *) ot_name)) == NULL) {
      /* Don't complain. This does happen when disabling plug-ins too.
      g_warning("object_get_type(%s) returned NULL", tmp); */
      g_clear_pointer (&sheet_obj->description, g_free);
      g_clear_pointer (&sheet_obj->pixmap_file, g_free);
      g_clear_pointer (&sheet_obj->object_type, g_free);
      g_clear_pointer (&sheet_obj, g_free);
      if (tmp) {
        dia_clear_xml_string (&ot_name);
      }
      continue;
    }

    /* set defaults */
    if (sheet_obj->pixmap_file == NULL &&
        sheet_obj->pixmap == NULL) {
      g_assert (otype->pixmap || otype->pixmap_file);
      sheet_obj->pixmap = otype->pixmap;
      sheet_obj->pixmap_file = otype->pixmap_file;
      sheet_obj->has_icon_on_sheet = has_icon_on_sheet;
    }
    if (!(otype->flags & DIA_OBJECT_HAS_VARIANTS)) {
      sheet_obj->user_data = otype->default_user_data;
    }

    dia_clear_xml_string (&ot_name);

    /* we don't need to fix up the icon and descriptions for simple objects,
       since they don't have their own description, and their icon is
       already automatically handled. */
    sheet_append_sheet_obj (sheet, sheet_obj);
  }

  if (!shadowing_sheet) {
    register_sheet (sheet);
  }

out:
  if (name_is_gmalloced) {
    g_clear_pointer (&name, g_free);
  } else {
    dia_clear_xml_string (&name);
  }
  dia_clear_xml_string (&description);

  g_clear_pointer (&sheetdir, g_free);
  g_clear_pointer (&doc, xmlFreeDoc);
  g_clear_pointer (&ctx, dia_context_release);
}
