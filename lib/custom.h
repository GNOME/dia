#ifndef CUSTOM_H
#define CUSTOM_H

#include <glib.h>
#include "object.h"
#include "sheet.h"

/* This can be used by an object library to explicitely load a custom shape
 * from a file. */
extern gboolean custom_object_load(gchar *filename, ObjectType **otype,
				   SheetObject **sheetobj);


/* This function will register the custom objects installed on the system */
extern void custom_register_objects(void);

#endif
