/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998, 1999 Alexander Larsson
 *
 * Custom Objects -- objects defined in XML rather than C.
 * Copyright (C) 1999 James Henstridge.
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

#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <glib.h>

#include <tree.h>
#include <parser.h>

#include "load_sheet.h"
#include "custom_util.h"
#include "shape_info.h"
#include "object.h"
#include "intl.h"

void custom_object_new (ShapeInfo *info,
			ObjectType **otype,
			SheetObject **sheetobj);

static Sheet *
load_from_index(const gchar *directory)
{
  Sheet *sheet = NULL;
  struct stat statbuf;
  gchar *index;
  xmlDocPtr doc;
  xmlNsPtr ns;
  xmlNodePtr node, shapes = NULL;
  char *tmp;
  gchar *name = NULL, *description = NULL, *filename;
  int name_score = -1;
  int descr_score = -1;

  index = g_strconcat(directory, G_DIR_SEPARATOR_S, "index.sheet", NULL);
  if (stat(index, &statbuf) < 0) {
    g_free(index);
    index = g_strconcat(directory, G_DIR_SEPARATOR_S, "index.xml", NULL);
    if (stat(index, &statbuf) < 0) {
      g_free(index);
      return NULL;
    }
  }
  /* index is the index file name, and it exists */
  doc = xmlParseFile(index);
  if (!doc)
    return NULL;
  if (!(ns = xmlSearchNsByHref(doc, doc->root,
		"http://www.daa.com.au/~james/dia-sheet-ns"))) {
    g_warning("could not find sheet namespace");
    xmlFreeDoc(doc); 
    g_free(index);
    return NULL;
  }
  if (doc->root->ns != ns || strcmp(doc->root->name, "sheet")) {
    g_warning("root element was %s -- expecting sheet", doc->root->name);
    xmlFreeDoc(doc);
    g_free(index);
    return NULL;
  }
  for (node = doc->root->childs; node != NULL; node = node->next) {
    if (node->type != XML_ELEMENT_NODE)
      continue;
    if (node->ns == ns && !strcmp(node->name, "name")) {
      gint score;

      /* compare the xml:lang property on this element to see if we get a
       * better language match.  LibXML seems to throw away attribute
       * namespaces, so we use "lang" instead of "xml:lang" */
      tmp = xmlGetProp(node, "xml:lang");
      if (!tmp) tmp = xmlGetProp(node, "lang");
      score = intl_score_locale(tmp);
      if (tmp) free(tmp);

      if (name_score < 0 || score < name_score) {
        name_score = score;
        tmp = xmlNodeGetContent(node);
        g_free(name);
        name = g_strdup(tmp);
        free(tmp);
      }
    } else if (node->ns == ns && !strcmp(node->name, "description")) {
      gint score;

      /* compare the xml:lang property on this element to see if we get a
       * better language match.  LibXML seems to throw away attribute
       * namespaces, so we use "lang" instead of "xml:lang" */
      tmp = xmlGetProp(node, "xml:lang");
      if (!tmp) tmp = xmlGetProp(node, "lang");
      score = intl_score_locale(tmp);
      if (tmp) free(tmp);

      if (descr_score < 0 || score < descr_score) {
        descr_score = score;
        tmp = xmlNodeGetContent(node);
        g_free(description);
        description = g_strdup(tmp);
        free(tmp);
      }
    } else if (node->ns == ns && !strcmp(node->name, "shapes")) {
      shapes = node;
    }
  }
  if (!shapes) {
    g_warning("no shapes specified in sheet index.");
    xmlFreeDoc(doc);
    g_free(index);
    g_free(name);
    g_free(description);
    return NULL;
  }
  for (node = shapes->childs; node != NULL; node = node->next) {
    ShapeInfo *info;
    ObjectType *obj_type;
    SheetObject *sheet_obj;

    if (node->type != XML_ELEMENT_NODE)
      continue;
    if (node->ns != ns || strcmp(node->name, "shape"))
      continue;
    tmp = xmlNodeGetContent(node);
    filename = get_relative_filename(index, tmp);
    free(tmp);

    info = shape_info_load(filename);
    if (!info) {
      g_warning("could not load shape from file %s", filename);
      g_free(filename);
      continue;
    }
    g_free(filename);

#ifdef DEBUG_CUSTOM
    shape_info_print(info); /* debugging ... */
#endif
    custom_object_new(info, &obj_type, &sheet_obj);

    object_register_type(obj_type);

    /* create sheet object.  new_sheet takes ownership of the two strings */
    if (!sheet)
      sheet = new_sheet(name, description);
    sheet_append_sheet_obj(sheet, sheet_obj);
  }
  if (!sheet) {
    g_free(name);
    g_free(description);
  }
  g_free(index);
  return sheet;
}

static Sheet *
load_with_readdir(const gchar *directory, const gchar *name)
{
  DIR *dp;
  struct dirent *dirp;
  Sheet *sheet = NULL;

  dp = opendir(directory);
  if (dp == NULL) {
    return;
  }
  while ( (dirp = readdir(dp)) ) {
    int len = strlen(dirp->d_name);
    if ((len > 4 && !strcmp(".xml", &dirp->d_name[len-4])) ||
        (len > 6 && !strcmp(".shape", &dirp->d_name[len-6]))) {
      gchar *filename = g_strconcat(directory, G_DIR_SEPARATOR_S,
                                    dirp->d_name, NULL);
      ShapeInfo *info = shape_info_load(filename);
      ObjectType  *obj_type;
      SheetObject *sheet_obj;

      g_free(filename);
      if (!info)
        continue;

#ifdef DEBUG_CUSTOM
      shape_info_print(info); /* debugging ... */
#endif
      custom_object_new(info, &obj_type, &sheet_obj); /* create new type */

      object_register_type(obj_type);

      /* register the sheet object */
      if (!sheet)
        sheet = new_sheet(g_strdup(_(name)),
                          NULL);
      sheet_append_sheet_obj(sheet, sheet_obj);
    }
  }
  closedir(dp);
  return sheet;
}

Sheet *
load_custom_sheet(const gchar *directory, const gchar *name)
{
  Sheet *sheet = load_from_index(directory);

  if (!sheet)
    sheet = load_with_readdir(directory, name);
  return sheet;
}
