#ifndef CUSTOM_H
#define CUSTOM_H

#include <glib.h>
#include "object.h"
#include "sheet.h"

/* this can be used by an object library to explicitely load a custom shape
 * from a file. */
gboolean custom_object_load      (gchar *filename,
				  ObjectType **otype,
				  SheetObject **sheetobj);


/* These function will register the custom objects installed on the system */
void     custom_register_objects (void);
void     custom_register_sheets  (void);


#endif
