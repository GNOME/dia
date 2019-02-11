#include "tool.h"

typedef struct _ToolButton ToolButton;

typedef struct _ToolButtonData ToolButtonData;

struct _ToolButtonData
{
  ToolType type;
  gpointer extra_data;
  gpointer user_data; /* Used by create_object_tool */
  GtkWidget *widget;
};

struct _ToolButton
{
  const gchar *icon_name;
  char  *tool_desc;
  char	*tool_accelerator;
  char  *action_name;
  ToolButtonData callback_data;
};

extern const int num_tools;
extern ToolButton tool_data[];
extern gchar *interface_current_sheet_name;

void tool_select_update (GtkWidget *w, gpointer   data);
GdkPixbuf *tool_get_pixbuf (ToolButton *tb);

void toolbox_setup_drag_dest (GtkWidget *canvas);
void canvas_setup_drag_dest (GtkWidget *canvas);
GtkWidget *toolbox_create(void);

void fill_sheet_menu(void);

