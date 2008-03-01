
#include "units.h"

/* from gnome-libs/libgnome/gnome-paper.c */
const DIAVAR DiaUnitDef units[] =
{
  /* XXX does anyone *really* measure paper size in feet?  meters? */

  /* human name, abreviation, points per unit */
  { "Centimeter", "cm", 28.346457, 2 },
  { "Decimeter",  "dm", 283.46457, 3 },
  { "Feet",       "ft", 864, 4 },
  { "Inch",       "in", 72, 3 },
  { "Meter",      "m",  2834.6457, 4 },
  { "Millimeter", "mm", 2.8346457, 2 },
  { "Point",      "pt", 1, 2 },
  { "Pica",       "pi", 12, 2 },
  { 0 }
};
