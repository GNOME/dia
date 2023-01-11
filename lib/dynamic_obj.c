/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * List of "dynamic" (animated) objects, and their refresh rates
 * Copyright (C) 2002 Cyrille Chépélov
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

#include "dynamic_obj.h"

typedef struct {
    DiaObject* obj;
    guint timeout;
} DynobjRec;

static GList* dyn_obj_list = NULL;

void dynobj_list_add_object(DiaObject* obj, guint timeout) {
    DynobjRec *dor = g_new(DynobjRec,1);
    dor->obj = obj;
    dor->timeout = timeout;

    dyn_obj_list = g_list_append(dyn_obj_list,dor);
}

static gint dor_found(gconstpointer data, gconstpointer user_data) {
    const DynobjRec* dor = (const DynobjRec*)data;
    const DiaObject* obj = (const DiaObject*)user_data;

    if ((!dor) || (!obj)) return 1;
    if (dor->obj != obj) return 1;
    return 0;
}


void
dynobj_list_remove_object (DiaObject *obj)
{
  GList* item = g_list_find_custom (dyn_obj_list, obj, dor_found);

  if (item) {
    DynobjRec* dor = item->data;
    dyn_obj_list = g_list_remove(dyn_obj_list,dor);
    g_clear_pointer (&dor, g_free);
  }
}


typedef struct {
    ObjectDynobjFunc odf;
    gpointer data;
} TrampolineData;

static void foreach_trampoline(gpointer data, gpointer user_data) {
    TrampolineData* td = (TrampolineData*)user_data;
    DynobjRec* dor = (DynobjRec*)data;

    td->odf(dor->obj,td->data);
}

void dynobj_list_foreach(ObjectDynobjFunc odf, gpointer data) {
    TrampolineData td;
    td.odf = odf;
    td.data = data;
    g_list_foreach(dyn_obj_list,foreach_trampoline,&td);
}

static void accum_timeout(gpointer data, gpointer user_data) {
    guint* accum = (guint*)user_data;
    DynobjRec* dor = (DynobjRec*)data;

    if (!dor) return;

    *accum = MAX(*accum,dor->timeout);
}

guint dynobj_list_get_dynobj_rate(void) {
    guint timeout = 250;

    if (dyn_obj_list)
        g_list_foreach(dyn_obj_list,accum_timeout,&timeout);
    else
        timeout = 0; /* no objet to refresh, no timeout */

    return timeout;
}
