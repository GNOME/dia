#ifndef LISTFUN_H
#define LISTFUN_H

#include <glib.h>

typedef gpointer (*MapFun) (gpointer);

void  list_free_foreach(gpointer , gpointer );
GList* list_map(GList *, gpointer (*) (gpointer));
void list_foreach_fun(gpointer, gpointer);

#endif /* LISTFUN_H */

