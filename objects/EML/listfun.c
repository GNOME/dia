#include "listfun.h"

void 
list_free_foreach(gpointer data, gpointer user_fun)
{
  if (user_fun != NULL) {
    ((void (*) (gpointer)) user_fun) ((gpointer) data);
  }
  else {
    g_free(data);
  }
}

void 
list_foreach_fun(gpointer data, gpointer user_fun)
{
    ((void (*) (gpointer)) user_fun) ((gpointer) data);
}

GList*
list_map(GList *inlist, MapFun fun)
{
  GList *retlist;
  GList *list;

  retlist = NULL;

  list = inlist;
  while (list != NULL) {
    retlist = g_list_append(retlist, fun(list->data));
    list = g_list_next(list);
  }
  
  return retlist;
}
