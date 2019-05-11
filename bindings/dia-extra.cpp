/*
 * C++ implementation of Dia Bindings Helper Functions
 *
 * Copyright (C) 2007, Hans Breuer, <Hans@Breuer.Org>
 *
 * This is free software; you can redistribute it and/or modify
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
#include "dia-extra.h"

#include <glib.h>
#include "message.h"
#include "utils.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <glib-object.h>
#include "properties.h"
#include "object.h"
#include "plug-ins.h"
#include "dialib.h"

#include "dia-object.h" // for group_create impl.
#include "dia-diagramdata.h" // for Objects
#include "group.h"

/*!
 *
 *
 * \todo Dia's init function needs to be cleaned. E.g. it would be useful
 * to just initialize the plug-ins needed and not everything. Also this
 * function may be split to be partially called at bindings initialization
 * time.
 */
void dia::register_plugins ()
{
    libdia_init (DIA_MESSAGE_STDERR);

    dia_register_plugins ();
}

void dia::message (int severity, const char* msg)
{
    g_print ("%s: %s\n", severity <= 0 ? "Info" : severity == 1 ? "Warning" : "Error", msg);
}

/*!
 * \brief factory function for Group
 *
 * Dia's group_create eats the given list, so this should to do the same,
 * i.e. passing ownership of the Object in in objects.
 * But
 */
dia::Object*
dia::group_create (Objects* objects)
{
    GList *list = NULL;
    int i;

    g_return_val_if_fail (objects->len() > 0, NULL);

    for (i = 0; i < objects->len(); ++i) {
	Object *o = objects->getitem(i);
	if (o) // paranoid
	    list = g_list_append (list, o->Self());
    }
    return new dia::Object (::group_create (list));
}
