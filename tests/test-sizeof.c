/* not really a test but only an interactive check */

#include "config.h"

#include <glib/gi18n-lib.h>

#include <stdio.h>

#include "object.h"
#include "connection.h"
#include "element.h"
#include "../objects/UML/class.h"

int
main (int argc, char** argv)
{
#define DUMP(o) g_print ("%s: %d\n", #o, (int)sizeof(o))

  DUMP(DiaObject);
  DUMP(Connection);
  DUMP(Element);
  DUMP(UMLClass);

  DUMP(Handle);
  DUMP(ConnectionPoint);

#undef DUMP

  return 0;
}
