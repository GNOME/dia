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

#include "xslt.h"

void xslt_ok(void);

static char *diafilename;
static char *filename;

static GModule *xslt_module;

static void
export_data(DiagramData *data, const gchar *f, 
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
	
	stylefname = g_strconcat(g_getenv("DIA_PLUGIN_PATH"),
				 G_DIR_SEPARATOR_S, "xslt", 
				 G_DIR_SEPARATOR_S, xsl_from->xsl, NULL);

	style = xsltParseStylesheetFile((const xmlChar *) stylefname);
	if(style == NULL) {
		message_error(_("Error while parsing stylesheet %s\n"), stylefname);
		return;
	}
	
	res = xsltApplyStylesheet(style, doc, NULL);
	if(res == NULL) {
		message_error(_("Error while parsing stylesheet %s\n"), stylefname);
		return;
	}
	
	g_free(stylefname);
	
	stylefname = g_strconcat(g_getenv("DIA_PLUGIN_PATH"), 
				 G_DIR_SEPARATOR_S, "xslt", 
				 G_DIR_SEPARATOR_S, xsl_to->xsl, NULL);

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
	g_free(stylefname);

	xsltFreeStylesheet(codestyle);
	xsltFreeStylesheet(style);
	xmlFreeDoc(res);
	xmlFreeDoc(doc);
	xsltCleanupGlobals();
	xmlCleanupParser();

	xslt_clear();
}

/* --- dia plug-in interface --- */

#define MY_RENDERER_NAME "XSL Transformation filter"

static const gchar *extensions[] = { "code", NULL };
static DiaExportFilter my_export_filter = {
	N_(MY_RENDERER_NAME),
	extensions,
	export_data
};


PluginInitResult
dia_plugin_init(PluginInfo *info)
{
	gchar *path;
	FILE *xsls;
	int res;

      /* Primary xsl */
	fromxsl_t *cur_from;
      /* Secondary xsl */
	toxsl_t *cur_to, *prev_to;
	
	if (!dia_plugin_info_init(info, "XSLT",
			          _("XSL Transformation filter"),
			          xslt_can_unload, xslt_unload))
	{
			return DIA_PLUGIN_INIT_ERROR;
	}
	
#ifdef G_OS_WIN32
      /* FIXME: We can't assume \Windows is the right path, can we ? */
	path = g_module_build_path("\Windows", "xslt");
#else
      /* FIXME: We should have a --with-xslt-prefix and use this */
	path = g_module_build_path("/usr/lib", "xslt");
#endif	
	xslt_module = g_module_open(path, 0);
	if(!xslt_module) {
		message_error(_("Could not load XSLT library (%s) : %s"), path, g_module_error());
		return DIA_PLUGIN_INIT_ERROR;
	}
	filter_register_export(&my_export_filter);
		
	g_free(path);
	
	path = g_strconcat(g_getenv("DIA_PLUGIN_PATH"), 
			   G_DIR_SEPARATOR_S, "xslt", 
			   G_DIR_SEPARATOR_S, "stylesheets", NULL);
	
	if(path == NULL)
	{
		message_error(
        _("Could not load XSLT plugin; DIA_PLUGIN_PATH is not set."));
		return DIA_PLUGIN_INIT_ERROR;
	}
		
      /* printf("XSLT plugin config file: %s\n", path); */

	xsls = fopen(path, "r");

	if(xsls == NULL)
	{
		message_error("Could not find XSLT plugin configuration file : %s", path);
		return DIA_PLUGIN_INIT_ERROR;
	}
	
	
	cur_from = froms = g_malloc(sizeof(fromxsl_t));
	cur_from->next = NULL;

	while(1)
	{
		fscanf(xsls, "%as : %as\n", &cur_from->name, &cur_from->xsl);
        /* printf("From: %s (%s)\n", cur_from->name, cur_from->xsl); */
		prev_to = NULL;
		cur_to = cur_from->xsls = g_malloc(sizeof(toxsl_t));
		while(1)
		{
			res = fscanf(xsls, "%as = %as\n", &cur_to->name, &cur_to->xsl);
			
			if(res != 2) {
				if(prev_to != NULL)
					prev_to->next = NULL;
				break;
			}

          /* printf("To: %s (%s)\n", cur_to->name, cur_to->xsl);			 */
			if(!feof(xsls)) {
				prev_to = cur_to;
				cur_to->next = g_malloc(sizeof(toxsl_t));
				cur_to = cur_to->next;
			} else {
				cur_to->next = NULL;
				fclose(xsls);
				break;
			}
		}
		if(!feof(xsls)) {
			cur_from->next = g_malloc(sizeof(fromxsl_t));
			cur_from = cur_from->next;
		} else {
			cur_from->next = NULL;
			break;
		}
	}
	
	cur_from = froms;
	
	/*printf("XSLT plugin configuration: \n");
	while(cur_from != NULL)
	{
		printf("From: %s (%s)\n", cur_from->name, cur_from->xsl);

		cur_to = cur_from->xsls;
		while(cur_to != NULL) {
			printf("\tTo: %s (%s)\n", cur_to->name, cur_to->xsl);
			cur_to = cur_to->next;
		}

		cur_from = cur_from->next;
		}*/

	xsl_from = froms;
	xsl_to = froms->xsls;

	return DIA_PLUGIN_INIT_OK;
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
