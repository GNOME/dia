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
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <glib.h>
#include <tree.h>
#include <parser.h>
#include <string.h>

#include "config.h"
#include "intl.h"

#include "sheet.h"
#include "message.h"
#include "object.h"
#include "dia_dirs.h"

#if defined(LIBXML_VERSION) && LIBXML_VERSION >= 20000
#define root children
#define childs children
#endif

static GSList *sheets = NULL;

Sheet *
new_sheet(char *name, char *description)
{
  Sheet *sheet;

  sheet = g_new(Sheet, 1);
  sheet->name = name;
  sheet->description = description;

  sheet->objects = NULL;
  return sheet;
}

void
sheet_prepend_sheet_obj(Sheet *sheet, SheetObject *obj)
{
  ObjectType *type;

  type = object_get_type(obj->object_type);
  if (type == NULL) {
    message_warning("Object '%s' needed in sheet '%s' was not found.\n"
		    "It will not be available for use.",
		    obj->object_type, sheet->name);
  } else {
    sheet->objects = g_slist_prepend( sheet->objects, (gpointer) obj);
  }
}

void
sheet_append_sheet_obj(Sheet *sheet, SheetObject *obj)
{
  ObjectType *type;

  type = object_get_type(obj->object_type);
  if (type == NULL) {
    message_warning("Object '%s' needed in sheet '%s' was not found.\n"
		    "It will not be availible for use.",
		    obj->object_type, sheet->name);
  } else {
    sheet->objects = g_slist_append( sheet->objects, (gpointer) obj);
  }
}

void
register_sheet(Sheet *sheet)
{
  sheets = g_slist_append(sheets, (gpointer) sheet);
}

GSList *
get_sheets_list(void)
{
  return sheets;
}

/* Sheet file management */

static void load_sheets_from_dir(const gchar *directory);
static void load_register_sheet(const gchar *directory,const gchar *filename);

void load_all_sheets(void) {
  char *sheet_path;
  char *home_dir;

  home_dir = dia_config_filename("sheets");
  if (home_dir) {
    load_sheets_from_dir(home_dir);
    g_free(home_dir);
  }

  sheet_path = getenv("DIA_SHEET_PATH");
  if (sheet_path) {
    char **dirs = g_strsplit(sheet_path,G_SEARCHPATH_SEPARATOR_S,0);
    int i;

    for (i=0; dirs[i] != NULL; i++) 
      load_sheets_from_dir(dirs[i]);
    g_strfreev(dirs);
  } else {
    char *thedir = dia_get_data_directory("sheets");
    load_sheets_from_dir(thedir);
    g_free(thedir);
  }
}

static void 
load_sheets_from_dir(const gchar *directory)
{
  DIR *dp;
  struct dirent *dirp;
  struct stat statbuf;
  gchar *p;

  dp = opendir(directory);
  if (!dp) return;

  while ( (dirp = readdir(dp)) ) {
    gchar *filename = g_strconcat(directory,G_DIR_SEPARATOR_S,
				  dirp->d_name,NULL);

    if (!strcmp(dirp->d_name, ".") || !strcmp(dirp->d_name, "..")) {
      g_free(filename);
      continue;
    }
    /* filter out non-files */
    if (stat(filename, &statbuf) < 0) {
      g_free(filename);
      continue;
    }
    if (!S_ISREG(statbuf.st_mode)) {
      g_free(filename);
      continue;
    }

    /* take only .sheet files */
    p = filename + strlen(filename) - 6 /* strlen(".sheet") */;
    if (0!=strncmp(p,".sheet",6)) {
      g_free(filename);
      continue;
    }

    load_register_sheet(directory,filename);
    g_free(filename);
				
  }

  closedir(dp);
}

static void 
load_register_sheet(const gchar *dirname, const gchar *filename)
{
  xmlDocPtr doc;
  xmlNsPtr ns;
  xmlNodePtr node, contents,subnode,root;
  char *tmp;
  gchar *name = NULL, *description = NULL;
  int name_score = -1;
  int descr_score = -1;
  Sheet *sheet = NULL;
  GSList *sheetp;
  gboolean set_line_break = FALSE;

  /* the XML fun begins here. */

  doc = xmlParseFile(filename);
  if (!doc) return;
  root = doc->root;
  while (root && (root->type != XML_ELEMENT_NODE)) root=root->next;
  if (!root) return;

  if (!(ns = xmlSearchNsByHref(doc,root,
	   "http://www.lysator.liu.se/~alla/dia/dia-sheet-ns"))) {
    g_warning("could not find sheet namespace");
    xmlFreeDoc(doc); 
    return;
  }
  
  if ((root->ns != ns) || (strcmp(root->name,"sheet"))) {
    g_warning("root element was %s -- expecting sheet", doc->root->name);
    xmlFreeDoc(doc);
    return;
  }

  contents = NULL;
  for (node = root->childs; node != NULL; node = node->next) {
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
        if (name) free(name);
        name = xmlNodeGetContent(node);
      }      
    } else if (node->ns == ns && !strcmp(node->name, "description")) {
      gint score;

      /* compare the xml:lang property on this element to see if we get a
       * better language match.  LibXML seems to throw away attribute
       * namespaces, so we use "lang" instead of "xml:lang" */
      tmp = xmlGetProp(node, "xml:lang");
      if (!tmp) tmp = xmlGetProp(node, "lang");
      score = intl_score_locale(tmp);
      g_free(tmp);

      if (descr_score < 0 || score < descr_score) {
        descr_score = score;
        if (description) free(description);
        description = xmlNodeGetContent(node);
      }
      
    } else if (node->ns == ns && !strcmp(node->name, "contents")) {
      contents = node;
    }
  }
  
  if (!contents) {
    g_warning("no contents in sheet %s.", filename);
    xmlFreeDoc(doc);
    if (name) free(name);
    if (description) free(description);
    return;
  }

  sheetp = get_sheets_list();
  while (sheetp) {
    if (sheetp->data && !strcmp(((Sheet *)(sheetp->data))->name,name)) {
      sheet = sheetp->data;
      break;
    }
    sheetp = sheetp->next;
  }
  
  if (!sheet)
    sheet = new_sheet(name,description);
  
  for (node = contents->childs ; node != NULL; node = node->next) {
    SheetObject *sheet_obj;
    ObjectType *otype;
    gchar *iconname = NULL;

    int subdesc_score = -1;
    gchar *objdesc = NULL;

    gint intdata = 0;
    gchar *chardata = NULL;

    if (node->type != XML_ELEMENT_NODE) 
      continue;
    if (node->ns != ns) continue;
    if (!strcmp(node->name,"object")) {
      /* nothing */
    } else if (!strcmp(node->name,"shape")) {
      g_message("%s: you should use object tags rather than shape tags now",
		filename);
    } else if (!strcmp(node->name,"br")) {
      /* Line break tag. */
      set_line_break = TRUE;
      continue;
    } else
      continue; /* unknown tag */
    
    tmp = xmlGetProp(node,"intdata");
    if (tmp) { 
      char *p;
      intdata = (gint)strtol(tmp,&p,0);
      if (*p != 0) intdata = 0;
      free(tmp);
    }
    chardata = xmlGetProp(node,"chardata");
    /* TODO.... */
    if (chardata) free(chardata);
    
    for (subnode = node->childs; subnode != NULL ; subnode = subnode->next) {
      if (subnode->ns == ns && !strcmp(subnode->name, "description")) {
	gint score;

	/* compare the xml:lang property on this element to see if we get a
	 * better language match.  LibXML seems to throw away attribute
	 * namespaces, so we use "lang" instead of "xml:lang" */
	  
	tmp = xmlGetProp(subnode, "xml:lang");
	if (!tmp) tmp = xmlGetProp(subnode, "lang");
	score = intl_score_locale(tmp);
	if (tmp) free(tmp);

	if (subdesc_score < 0 || score < subdesc_score) {
	  subdesc_score = score;
	  if (objdesc) free(objdesc);
	  objdesc = xmlNodeGetContent(subnode);
	}
	  
      } else if (subnode->ns == ns && !strcmp(subnode->name,"icon")) {
	tmp = xmlNodeGetContent(subnode);
	iconname = g_strconcat(dirname,G_DIR_SEPARATOR_S,tmp,NULL);
	if (tmp) free(tmp);
      }
    }

    tmp = xmlGetProp(node,"name");

    sheet_obj = g_new(SheetObject,1);
    sheet_obj->object_type = g_strdup(tmp);
    sheet_obj->description = objdesc;
    sheet_obj->pixmap = NULL;
    sheet_obj->user_data = (void *)intdata; /* XXX modify user_data type ? */
    sheet_obj->pixmap_file = iconname; 
    sheet_obj->line_break = set_line_break;
    set_line_break = FALSE;

    if ((otype = object_get_type(tmp)) == NULL) {
      g_free(sheet_obj->description);
      g_free(sheet_obj->pixmap_file);
      g_free(sheet_obj->object_type);
      g_free(sheet_obj);
      if (tmp) free(tmp);
      continue; 
    }	  

    /* set defaults */
    if (sheet_obj->pixmap_file == NULL) {
      sheet_obj->pixmap = otype->pixmap;
      sheet_obj->pixmap_file = otype->pixmap_file;
    }
    if (sheet_obj->user_data == NULL)
      sheet_obj->user_data = otype->default_user_data;
    if (tmp) free(tmp);
      
    /* we don't need to fix up the icon and descriptions for simple objects,
       since they don't have their own description, and their icon is 
       already automatically handled. */
    sheet_append_sheet_obj(sheet,sheet_obj);
  }
  
  if (sheet) register_sheet(sheet);
  xmlFreeDoc(doc);
}
