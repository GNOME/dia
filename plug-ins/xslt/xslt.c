/*
 * File: plug-ins/xslt/xslt.c
 * 
 * Made by Matthieu Sozeau <mattam@netcourrier.com>
 * 
 * Started on  Thu May 16 23:21:43 2002 Matthieu Sozeau
 * Last update Tue Jun  4 21:00:30 2002 Matthieu Sozeau
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
 *
 */

#include <config.h>
#include <stdio.h>
#include "filter.h"
#include "intl.h"
#include "message.h"
#include "dia_dirs.h"
#include <libxslt/xslt.h>
#include <libxslt/xsltInternals.h>
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>
#include <libxml/tree.h>

#include "xslt.h"

void xslt_ok(void);

static char *diafilename;
static char *filename;

static GModule *xslt_module;

static void
export_xslt(DiagramData *data, const gchar *f, 
            const gchar *diaf, void* user_data)
{
	if(filename != NULL)
		g_free(filename);
	
	filename = g_strdup(f);
	if(diafilename != NULL)
		g_free(diafilename);

	diafilename = g_strdup(diaf);

	xslt_dialog_create();	
}


void xslt_ok() {
	FILE *file, *out;
	int err;
	gchar *stylefname;
	char *params[] = { "directory", NULL, NULL };
	xsltStylesheetPtr style, codestyle;
	xmlDocPtr doc, res;

	params[1] = g_strconcat("'", g_dirname(filename), G_DIR_SEPARATOR_S, "'", NULL);
	
	file = fopen(diafilename, "r");

	if (file == NULL) {
	    message_error(_("Couldn't open: '%s' for reading.\n"), diafilename);
	    return;
	}

	out = fopen(filename, "w+");
	
	if (out == NULL) {
	    message_error(_("Couldn't open: '%s' for writing.\n"), filename);
	    return;
	}
	
	xmlSubstituteEntitiesDefault(0);
	doc = xmlParseFile(diafilename);

	if(doc == NULL) {
		message_error(_("Error while parsing %s\n"), diafilename);
		return;
	}
	
	stylefname = xsl_from->xsl;

	style = xsltParseStylesheetFile((const xmlChar *) stylefname);
	if(style == NULL) {
		message_error(_("Error while parsing stylesheet %s\n"), stylefname);
		return;
	}
	
	res = xsltApplyStylesheet(style, doc, NULL);
	if(res == NULL) {
		message_error(_("Error while applying stylesheet %s\n"), stylefname);
		return;
	}       
	
	stylefname = xsl_to->xsl;
	
	codestyle = xsltParseStylesheetFile((const xmlChar *) stylefname);
	if(codestyle == NULL) {
	    message_error(_("Error while parsing stylesheet: %s\n"), xsl_to->xsl);
	    return;
	}
	
	xmlFreeDoc(doc);
	
	doc = xsltApplyStylesheet(codestyle, res, (const char **) params);
	if(doc == NULL) {
	    message_error(_("Error while applying stylesheet: %s\n"), xsl_to->xsl);
	    return;
	}


	err = xsltSaveResultToFile(out, doc, codestyle);
      /* printf("Saved to file (%i) using stylesheet: %s\n",
         err, stylefname); */
		
	fclose(out);
	fclose(file);

	xsltFreeStylesheet(codestyle);
	xsltFreeStylesheet(style);
	xmlFreeDoc(res);
	xmlFreeDoc(doc);
	xsltCleanupGlobals();
	xmlCleanupParser();

	xslt_clear();
}

static toxsl_t *read_implementations(xmlNodePtr cur) 
{
    toxsl_t *first, *curto;
    first = curto = NULL;

    cur = cur->xmlChildrenNode;

    while (cur) {
        toxsl_t *to;
	if (xmlIsBlankNode(cur) || xmlNodeIsText(cur)) { cur = cur->next; continue; }
	to = g_malloc(sizeof(toxsl_t));	
	to->next = NULL;
	
	to->name = xmlGetProp(cur, (const xmlChar *) "name");
	to->xsl = xmlGetProp(cur, (const xmlChar *) "stylesheet");
	
	if (!(to->name && to->xsl)) {
	    g_warning ("Name and stylesheet attributes are required for the implementation element %s in XSLT plugin configuration file", cur->name);
	    g_free(to);
	    to = NULL;
	} else {
	    if (first == NULL) {
		first = curto = to;
	    } else {
		curto->next = to;
		curto = to;
	    }
	}	    
	cur = cur->next;
    }
    
    return first;
}

static PluginInitResult read_configuration(const char *config) 
{
    xmlDocPtr doc;
    xmlNodePtr cur;
    /* Primary xsl */
    fromxsl_t *cur_from = NULL;

    if (!g_file_test(config, G_FILE_TEST_EXISTS))
	return DIA_PLUGIN_INIT_ERROR;
    
    doc = xmlParseFile(config);
    
    if (doc == NULL) 
    {
	g_error ("Couldn't parse XSLT plugin's configuration file %s", config);
	return DIA_PLUGIN_INIT_ERROR;
    }
    
    cur = xmlDocGetRootElement(doc);
    if (cur == NULL)
    {
	g_error ("XSLT plugin's configuration file %s is empty", config);
	return DIA_PLUGIN_INIT_ERROR;
    }
    
    /* We don't care about the top level element's name */
    
    cur = cur->xmlChildrenNode;

    while (cur) {
	if (xmlIsBlankNode(cur) || xmlNodeIsText(cur)) { cur = cur->next; continue; }
	else if (!xmlStrcmp(cur->name, "language")) {
	    fromxsl_t *new_from = g_malloc(sizeof(fromxsl_t));
	    new_from->next = NULL;

	    new_from->name = xmlGetProp(cur, (const xmlChar *) "name");
	    new_from->xsl = xmlGetProp(cur, (const xmlChar *) "stylesheet");
	    
	    if (!(new_from->name && new_from->xsl)) {
		g_warning ("'name' and 'stylesheet' attributes are required for the language element %s in XSLT plugin configuration file", cur->name);
		g_free(new_from);
		new_from = NULL;
	    } else {
		if (froms == NULL)
		    froms = cur_from = new_from;
		else {
		    cur_from->next = new_from;
		    cur_from = new_from;
		}
		
		cur_from->xsls = read_implementations(cur);
		if (cur_from->xsls == NULL) {
		    g_warning ("No implementation stylesheet for language %s in XSLT plugin configuration file", cur_from->name);
		}
	    }	    
	} else {
	    g_warning ("Wrong node name %s in XSLT plugin configuration file, 'language' expected", cur->name);
	}
	cur = cur -> next;	
    }
   
    if (froms == NULL) {
	g_warning ("No stylesheets configured in %s for XSLT plugin", config);
    }
    
    /*cur_from = froms;

    printf("XSLT plugin configuration: \n");
    while(cur_from != NULL)
    {
	printf("From: %s (%s)\n", cur_from->name, cur_from->xsl);
	
	cur_to = cur_from->xsls;
	while(cur_to != NULL) {
	    printf("\tTo: %s (%s)\n", cur_to->name, cur_to->xsl);
	    cur_to = cur_to->next;
	}
	cur_from = cur_from->next;
    }
    */
    xmlFreeDoc(doc);
    xmlCleanupParser();

    return DIA_PLUGIN_INIT_OK;
}


/* --- dia plug-in interface --- */

#define MY_RENDERER_NAME "XSL Transformation filter"

static const gchar *extensions[] = { "code", NULL };
static DiaExportFilter my_export_filter = {
    N_(MY_RENDERER_NAME),
    extensions,
    export_xslt
};


PluginInitResult
dia_plugin_init(PluginInfo *info)
{
	gchar *path;
	PluginInitResult global_res, user_res;

	if (!dia_plugin_info_init(info, "XSLT",
			          _("XSL Transformation filter"),
			          NULL, NULL))
	{
			return DIA_PLUGIN_INIT_ERROR;
	}
	
	path = dia_get_data_directory("plugins" G_DIR_SEPARATOR_S
				      "xslt" G_DIR_SEPARATOR_S
				      "stylesheets.xml");
	global_res = read_configuration(path);
	g_free (path);

	path = g_strconcat(dia_config_filename("plugins"),
			   G_DIR_SEPARATOR_S, "xslt",
			   G_DIR_SEPARATOR_S, "stylesheets.xml", NULL);
	
	user_res = read_configuration(path);
	
	g_free (path);
	
	if (global_res == DIA_PLUGIN_INIT_OK || user_res == DIA_PLUGIN_INIT_OK)
	{
	    xsl_from = froms;
	    xsl_to = xsl_from->xsls;

	    filter_register_export(&my_export_filter);
	    return DIA_PLUGIN_INIT_OK;
	} else {
	    g_warning (_("No valid configuration files found for the XSLT plugin, not loading."));
	    return DIA_PLUGIN_INIT_ERROR;
	}
}

gboolean xslt_can_unload(PluginInfo *info)
{
	return 1;
}

void xslt_unload(PluginInfo *info)
{
#if 0
	language_t *cur = languages, *next;
	
      /*printf("Unloading xslt\n"); */

	while(cur != NULL) {
		next = cur->next;
		g_free(cur);
		cur = next;
	}
#endif 
  
}
