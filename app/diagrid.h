#ifndef DIA_GRID_H
#define DIA_GRID_H

typedef struct _DiaGrid DiaGrid;

struct _DiaGrid  {
  /* grid line intervals */
  real width_x, width_y;
  /* the interval between visible grid lines */
  guint visible_x, visible_y;
  /* the interval between major lines (non-stippled).
   * if 0, no major lines are drawn (all lines are stippled).
   * if 1, all lines are solid.
   */
  guint major_lines;
  /* True if the grid is dynamically calculated.
   * When true, width_x and width_y are ignored.
   */
  gboolean dynamic;
  /* The color of the grid lines.
   */
  Color colour;
  /** True if this grid is a hex grid. */
  gboolean hex;
  /** Size of each edge on a hex grid. */
  real hex_size;
};

#endif /* DIA_GRID_H */
