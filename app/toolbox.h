#include "tools/tool.h"
#include "tools/create_object.h"

G_BEGIN_DECLS

#define DIA_TYPE_TOOLBOX (dia_toolbox_get_type ())
G_DECLARE_FINAL_TYPE (DiaToolbox, dia_toolbox, DIA, TOOLBOX, GtkBox)

struct _DiaToolbox {
  GtkBox parent;

  GtkWidget  *tools;
  GtkWidget  *items;
  GListStore *sheets;
};

typedef struct _ToolButton ToolButton;

typedef struct _ToolButtonData ToolButtonData;

struct _ToolButtonData
{
  GType    type;
  gpointer extra_data;
  gpointer user_data; /* Used by create_object_tool */
  GtkWidget *widget;
};

struct _ToolButton
{
  const gchar **icon_data;
  char  *tool_desc;
  char	*tool_accelerator;
  char  *action_name;
  ToolButtonData callback_data;
};

extern const int num_tools;
extern ToolButton tool_data[];
extern gchar *interface_current_sheet_name;

void tool_select_update (GtkWidget *w, gpointer   data);

void toolbox_setup_drag_dest (GtkWidget *canvas);
void canvas_setup_drag_dest (GtkWidget *canvas);

GtkWidget *dia_toolbox_new           (void);
void       dia_toolbox_update_sheets (DiaToolbox *self);

G_END_DECLS
