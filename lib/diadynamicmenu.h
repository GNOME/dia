
#ifndef DIA_DYNAMIC_MENU_H
#define DIA_DYNAMIC_MENU_H

#include <gtk/gtk.h>

/* DiaDynamicMenu */

#define DIA_DYNAMIC_MENU(obj) G_TYPE_CHECK_INSTANCE_CAST(obj, dia_dynamic_menu_get_type(), DiaDynamicMenu)
#define DIA_DYNAMIC_MENU_CLASS(klass) G_TYPE_CHECK_CLASS_CAST(klass, dia_dynamic_menu_get_type(), DiaDynamicMenuClass)
#define DIA_IS_DYNAMIC_MENU(obj) G_TYPE_CHECK_INSTANCE_TYPE(obj, dia_dynamic_menu_get_type())

typedef struct _DiaDynamicMenu DiaDynamicMenu;
typedef struct _DiaDynamicMenuClass DiaDynamicMenuClass;

/** The ways the non-default entries in the menu can be sorted:
 * DDM_SORT_TOP: Just add new ones at the top, removing them from the middle.
 * Not currently implemented.
 * DDM_SORT_NEWEST:  Add new ones to the top, and move selected ones to the
 * top as well.
 * DDM_SORT_SORT:  Sort the entries according to the CompareFunc order.
 */
typedef enum { DDM_SORT_TOP, DDM_SORT_NEWEST, DDM_SORT_SORT } DdmSortType;

typedef GtkWidget *(* DDMCreateItemFunc)(DiaDynamicMenu *, gchar *);
typedef void (* DDMCallbackFunc)(DiaDynamicMenu *, const gchar *, gpointer);

GType      dia_dynamic_menu_get_type  (void);

GtkWidget *dia_dynamic_menu_new(DDMCreateItemFunc create,
				gpointer userdata,
				GtkMenuItem *otheritem, gchar *persist);
GtkWidget *dia_dynamic_menu_new_stringbased(GtkMenuItem *otheritem, 
					    gpointer userdata,
					    gchar *persist);
GtkWidget *dia_dynamic_menu_new_listbased(DDMCreateItemFunc create,
					  gpointer userdata,
					  gchar *other_label,
					  GList *items, gchar *persist);
GtkWidget *dia_dynamic_menu_new_stringlistbased(gchar *other_label,
						GList *items, 
						gpointer userdata,
						gchar *persist);
void dia_dynamic_menu_add_default_entry(DiaDynamicMenu *ddm, const gchar *entry);
gint dia_dynamic_menu_add_entry(DiaDynamicMenu *ddm, const gchar *entry);
void dia_dynamic_menu_set_sorting_method(DiaDynamicMenu *ddm, DdmSortType sort);
void dia_dynamic_menu_reset(GtkWidget *widget, gpointer userdata);
void dia_dynamic_menu_set_max_entries(DiaDynamicMenu *ddm, gint max);
void dia_dynamic_menu_set_columns(DiaDynamicMenu *ddm, gint cols);
gchar *dia_dynamic_menu_get_entry(DiaDynamicMenu *ddm);
void dia_dynamic_menu_select_entry(DiaDynamicMenu *ddm, const gchar *entry);

GList *dia_dynamic_menu_get_default_entries(DiaDynamicMenu *ddm);
const gchar *dia_dynamic_menu_get_persistent_name(DiaDynamicMenu *ddm);

#endif /* DIA_DYNAMIC_MENU_H */
