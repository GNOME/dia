#include "custom_util.h"

gchar *
get_relative_filename(const gchar *current, const gchar *relative)
{
  gchar *dirname, *tmp;

  g_return_val_if_fail(current != NULL, NULL);
  g_return_val_if_fail(relative != NULL, NULL);

  if (g_path_is_absolute(relative))
    return g_strdup(relative);
  dirname = g_dirname(current);
  tmp = g_strconcat(dirname, G_DIR_SEPARATOR_S, relative, NULL);
  g_free(dirname);
  return tmp;
}
