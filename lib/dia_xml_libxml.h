/* Dia -- an diagram creation/manipulation program
 *
 * Prototypes which went in the way when trying to compile files without 
 * pulling in the whole libxml includes.
 * Copyright (C) 2001, C. Chepelov <chepelov@calixo.net>
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
#ifndef DIA_XML_LIBXML_H
#define DIA_XML_LIBXML_H

#include <tree.h>
#include <parser.h>
#include "dia_xml.h"

xmlDocPtr xmlDiaParseFile(const char *filename); 
/* for potentially broken files */
xmlDocPtr xmlDoParseFile(const char *filename); 
/* use this one instead of xmlParseFile */

void warn_about_broken_libxml1(void);

#endif
