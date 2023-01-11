/* AADL plugin for DIA
*
* Copyright (C) 2005 Laboratoire d'Informatique de Paris 6
* Author: Pierre Duquesne
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


/* AADL plugin for DIA
 * Author: Pierre Duquesne
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include <glib/gi18n-lib.h>

#include "aadl.h"

/***********************************************
 **           PORT-RELATED FUNCTIONS          **
 ***********************************************/

void
rotate_around_origin (Point *p, real angle)   /* FIXME: no namespace */
{
  double x = p->x;
  double y = p->y;

  p->x = x * cos(angle) - y * sin(angle) ;
  p->y = x * sin(angle) + y * cos(angle) ;

}




#define AADL_PORT_WIDTH_A  0.2
#define AADL_PORT_WIDTH_B  0.5
#define AADL_PORT_HEIGHT 0.6
#define AADL_PORT_LINEWIDTH 0.1
                                            /*	   A| B C	   */
#define AADL_ACCESS_WIDTH_A 0.2             /* 	    ---\	   */
#define AADL_ACCESS_WIDTH_B 0.3             /*	   |    +	   */
#define AADL_ACCESS_WIDTH_C 0.2	            /*	    ---/	   */
#define AADL_ACCESS_HEIGHT 0.9	            /* 	    | 	       	   */

#define AADL_EVENT_DELTA_WIDTH 0.4
#define AADL_EVENT_DELTA_HEIGHT 0.3



#define draw_in_data_port()                                       \
    p[0].x =  AADL_PORT_WIDTH_A;                                  \
    p[0].y =  -AADL_PORT_HEIGHT/2;                                \
                                                                  \
    p[1].x =  -AADL_PORT_WIDTH_B;                                 \
    p[1].y =  0;                                                  \
                                                                  \
    p[2].x =  AADL_PORT_WIDTH_A;                                  \
    p[2].y =  AADL_PORT_HEIGHT/2;                                 \
                                                                  \
    rotate_around_origin(p, port->angle);                         \
    rotate_around_origin(p+1, port->angle);                       \
    rotate_around_origin(p+2, port->angle);                       \
                                                                  \
    point_add(p, &port->handle->pos);                             \
    point_add(p+1, &port->handle->pos);                           \
    point_add(p+2, &port->handle->pos);                           \
                                                                  \
    dia_renderer_set_linewidth(renderer, AADL_PORT_LINEWIDTH);    \
    dia_renderer_set_linejoin(renderer, DIA_LINE_JOIN_MITER);     \
    dia_renderer_set_linestyle(renderer, DIA_LINE_STYLE_SOLID, 0.0);   \
                                                                  \
    dia_renderer_draw_polygon(renderer, p,  3, &color_black, &color_black);

#define draw_in_event_port()                                      \
    p[0].x =  AADL_PORT_WIDTH_A;                                  \
    p[0].y =  -AADL_PORT_HEIGHT/2 - AADL_EVENT_DELTA_HEIGHT;      \
                                                                  \
    p[1].x =  - AADL_PORT_WIDTH_B - AADL_EVENT_DELTA_WIDTH;       \
    p[1].y =  0;                                                  \
                                                                  \
    p[2].x =  AADL_PORT_WIDTH_A;                                  \
    p[2].y =  AADL_PORT_HEIGHT/2 + AADL_EVENT_DELTA_HEIGHT;       \
                                                                  \
    rotate_around_origin(p, port->angle);                         \
    rotate_around_origin(p+1, port->angle);                       \
    rotate_around_origin(p+2, port->angle);                       \
                                                                  \
    point_add(p, &port->handle->pos);                             \
    point_add(p+1, &port->handle->pos);                           \
    point_add(p+2, &port->handle->pos);                           \
                                                                  \
    dia_renderer_set_linewidth(renderer, AADL_PORT_LINEWIDTH);    \
    dia_renderer_set_linejoin(renderer, DIA_LINE_JOIN_MITER);     \
    dia_renderer_set_linestyle(renderer, DIA_LINE_STYLE_SOLID, 0.0);   \
                                                                  \
    dia_renderer_draw_polyline(renderer, p,  3, &color_black);

#define draw_out_data_port()                                      \
    p[0].x =  -AADL_PORT_WIDTH_A;                                 \
    p[0].y =  -AADL_PORT_HEIGHT/2;                                \
                                                                  \
    p[1].x =  AADL_PORT_WIDTH_B;                                  \
    p[1].y =  0;                                                  \
                                                                  \
    p[2].x =  -AADL_PORT_WIDTH_A;                                 \
    p[2].y =  AADL_PORT_HEIGHT/2;                                 \
                                                                  \
    rotate_around_origin(p, port->angle);                         \
    rotate_around_origin(p+1, port->angle);                       \
    rotate_around_origin(p+2, port->angle);                       \
                                                                  \
    point_add(p, &port->handle->pos);                             \
    point_add(p+1, &port->handle->pos);                           \
    point_add(p+2, &port->handle->pos);                           \
                                                                  \
    dia_renderer_set_linewidth(renderer, AADL_PORT_LINEWIDTH);    \
    dia_renderer_set_linejoin(renderer, DIA_LINE_JOIN_MITER);     \
    dia_renderer_set_linestyle(renderer, DIA_LINE_STYLE_SOLID, 0.0);   \
                                                                  \
    dia_renderer_draw_polygon(renderer, p,  3, &color_black, &color_black);

#define draw_out_event_port()                                     \
    p[0].x =  - AADL_PORT_WIDTH_A;                                \
    p[0].y =  - AADL_PORT_HEIGHT/2 - AADL_EVENT_DELTA_HEIGHT;     \
                                                                  \
    p[1].x =  AADL_PORT_WIDTH_B + AADL_EVENT_DELTA_WIDTH;         \
    p[1].y =  0;                                                  \
                                                                  \
    p[2].x =  - AADL_PORT_WIDTH_A;                                \
    p[2].y =  AADL_PORT_HEIGHT/2 + AADL_EVENT_DELTA_HEIGHT;       \
                                                                  \
    rotate_around_origin(p, port->angle);                         \
    rotate_around_origin(p+1, port->angle);                       \
    rotate_around_origin(p+2, port->angle);                       \
                                                                  \
    point_add(p, &port->handle->pos);                             \
    point_add(p+1, &port->handle->pos);                           \
    point_add(p+2, &port->handle->pos);                           \
                                                                  \
    dia_renderer_set_linewidth(renderer, AADL_PORT_LINEWIDTH);    \
    dia_renderer_set_linejoin(renderer, DIA_LINE_JOIN_MITER);     \
    dia_renderer_set_linestyle(renderer, DIA_LINE_STYLE_SOLID, 0.0);   \
                                                                  \
    dia_renderer_draw_polyline(renderer, p,  3, &color_black);

#define draw_in_out_data_port()                                   \
    p[0].x =  0;                                                  \
    p[0].y =  -AADL_PORT_HEIGHT/2;                                \
                                                                  \
    p[1].x =  AADL_PORT_WIDTH_B;                                  \
    p[1].y =  0;                                                  \
                                                                  \
    p[2].x =  0;                                                  \
    p[2].y =  AADL_PORT_HEIGHT/2;                                 \
                                                                  \
    p[3].x =  -AADL_PORT_WIDTH_B;                                 \
    p[3].y =  0;                                                  \
                                                                  \
    rotate_around_origin(p, port->angle);                         \
    rotate_around_origin(p+1, port->angle);                       \
    rotate_around_origin(p+2, port->angle);                       \
    rotate_around_origin(p+3, port->angle);                       \
                                                                  \
    point_add(p, &port->handle->pos);                             \
    point_add(p+1, &port->handle->pos);                           \
    point_add(p+2, &port->handle->pos);                           \
    point_add(p+3, &port->handle->pos);                           \
                                                                  \
    dia_renderer_set_linewidth(renderer, AADL_PORT_LINEWIDTH);    \
    dia_renderer_set_linejoin(renderer, DIA_LINE_JOIN_MITER);     \
    dia_renderer_set_linestyle(renderer, DIA_LINE_STYLE_SOLID, 0.0);   \
                                                                  \
    dia_renderer_draw_polygon(renderer, p,  4, &color_black, &color_black);

#define draw_in_out_event_port()                                  \
    p[0].x =  0;                                                  \
    p[0].y =  -AADL_PORT_HEIGHT/2 - AADL_EVENT_DELTA_HEIGHT;      \
                                                                  \
    p[1].x =  AADL_PORT_WIDTH_B + AADL_EVENT_DELTA_WIDTH;         \
    p[1].y =  0;                                                  \
                                                                  \
    p[2].x =  0;                                                  \
    p[2].y =  AADL_PORT_HEIGHT/2 + AADL_EVENT_DELTA_HEIGHT;       \
                                                                  \
    p[3].x =  -AADL_PORT_WIDTH_B - AADL_EVENT_DELTA_WIDTH;        \
    p[3].y =  0;                                                  \
                                                                  \
    rotate_around_origin(p, port->angle);                         \
    rotate_around_origin(p+1, port->angle);                       \
    rotate_around_origin(p+2, port->angle);                       \
    rotate_around_origin(p+3, port->angle);                       \
                                                                  \
    point_add(p, &port->handle->pos);                             \
    point_add(p+1, &port->handle->pos);                           \
    point_add(p+2, &port->handle->pos);                           \
    point_add(p+3, &port->handle->pos);                           \
                                                                  \
    dia_renderer_set_linewidth(renderer, AADL_PORT_LINEWIDTH);    \
    dia_renderer_set_linejoin(renderer, DIA_LINE_JOIN_MITER);     \
    dia_renderer_set_linestyle(renderer, DIA_LINE_STYLE_SOLID, 0.0);   \
                                                                  \
    dia_renderer_draw_polygon(renderer, p,  4, NULL, &color_black);


/* FIXME: should i make methods from this function ? */

void
aadlbox_draw_port (Aadlport *port, DiaRenderer *renderer)
{
  Point p[5];

  g_return_if_fail (port != NULL);
  g_return_if_fail (renderer!=NULL);

  switch (port->type) {

    case ACCESS_PROVIDER:
      p[0].x = - AADL_ACCESS_WIDTH_A;
      p[0].y = - AADL_ACCESS_HEIGHT/2;

      p[1].x =   AADL_ACCESS_WIDTH_B;
      p[1].y = - AADL_ACCESS_HEIGHT/2;

      p[2].x =   AADL_ACCESS_WIDTH_B + AADL_ACCESS_WIDTH_C ;
      p[2].y =   0;

      p[3].x =   AADL_ACCESS_WIDTH_B;
      p[3].y =   AADL_ACCESS_HEIGHT/2;

      p[4].x = - AADL_ACCESS_WIDTH_A;
      p[4].y =   AADL_ACCESS_HEIGHT/2;

      rotate_around_origin (p, port->angle);
      rotate_around_origin (p+1, port->angle);
      rotate_around_origin (p+2, port->angle);
      rotate_around_origin (p+3, port->angle);
      rotate_around_origin (p+4, port->angle);

      point_add (p, &port->handle->pos);
      point_add (p+1, &port->handle->pos);
      point_add (p+2, &port->handle->pos);
      point_add (p+3, &port->handle->pos);
      point_add (p+4, &port->handle->pos);

      dia_renderer_set_linewidth (renderer, AADL_PORT_LINEWIDTH);
      dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
      dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

      dia_renderer_draw_polygon (renderer, p,  5, &color_white, &color_black);

      break;

    case ACCESS_REQUIRER:
      p[0].x =   AADL_ACCESS_WIDTH_A;
      p[0].y = - AADL_ACCESS_HEIGHT/2;

      p[1].x = - AADL_ACCESS_WIDTH_B;
      p[1].y = - AADL_ACCESS_HEIGHT/2;

      p[2].x = - AADL_ACCESS_WIDTH_B - AADL_ACCESS_WIDTH_C ;
      p[2].y =   0;

      p[3].x = - AADL_ACCESS_WIDTH_B;
      p[3].y =   AADL_ACCESS_HEIGHT/2;

      p[4].x =   AADL_ACCESS_WIDTH_A;
      p[4].y =   AADL_ACCESS_HEIGHT/2;

      rotate_around_origin (p, port->angle);
      rotate_around_origin (p+1, port->angle);
      rotate_around_origin (p+2, port->angle);
      rotate_around_origin (p+3, port->angle);
      rotate_around_origin (p+4, port->angle);

      point_add (p, &port->handle->pos);
      point_add (p+1, &port->handle->pos);
      point_add (p+2, &port->handle->pos);
      point_add (p+3, &port->handle->pos);
      point_add (p+4, &port->handle->pos);

      dia_renderer_set_linewidth (renderer, AADL_PORT_LINEWIDTH);
      dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
      dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

      dia_renderer_draw_polygon (renderer, p,  5, &color_white, &color_black);

      break;

    case IN_DATA_PORT:
      draw_in_data_port ();
      break;

    case IN_EVENT_PORT:
      draw_in_event_port ()
      break;

    case IN_EVENT_DATA_PORT:
      draw_in_data_port ();
      draw_in_event_port ();
      break;

    case OUT_DATA_PORT:
      draw_out_data_port ();
      break;

    case OUT_EVENT_PORT:
      draw_out_event_port ();
      break;

    case OUT_EVENT_DATA_PORT:
      draw_out_data_port ();
      draw_out_event_port ();
      break;

    case IN_OUT_DATA_PORT:
      draw_in_out_data_port ();
      break;

    case IN_OUT_EVENT_PORT:
      draw_in_out_event_port ();
      break;

    case IN_OUT_EVENT_DATA_PORT:
      draw_in_out_data_port ();
      draw_in_out_event_port ();
      break;

  #define AADL_PORT_GROUP_SIZE 0.1

    case PORT_GROUP:    /* quick n dirty - should use macros*/
      {
        BezPoint b[5];
        int i;

        p[0].x = 0;  p[0].y = 0;

        rotate_around_origin (p, port->angle);
        point_add (p, &port->handle->pos);

        dia_renderer_set_linewidth (renderer, AADL_PORT_LINEWIDTH);
        dia_renderer_set_linejoin (renderer, DIA_LINE_JOIN_MITER);
        dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);

        dia_renderer_draw_ellipse (renderer,
                                  p,
                                  6 * AADL_PORT_GROUP_SIZE,
                                  6 * AADL_PORT_GROUP_SIZE,
                                  &color_black, &color_black);


        b[0].type = BEZ_MOVE_TO;
        b[1].type = BEZ_CURVE_TO;
        b[2].type = BEZ_LINE_TO;
        b[3].type = BEZ_CURVE_TO;
        b[4].type = BEZ_LINE_TO;

        b[0].p1.x = -2         * AADL_PORT_GROUP_SIZE;
        b[0].p1.y = -6         * AADL_PORT_GROUP_SIZE;

        b[1].p1.x = -6         * AADL_PORT_GROUP_SIZE;
        b[1].p1.y = -4         * AADL_PORT_GROUP_SIZE;
        b[1].p2.x = -6         * AADL_PORT_GROUP_SIZE;
        b[1].p2.y =  4         * AADL_PORT_GROUP_SIZE;
        b[1].p3.x = -2         * AADL_PORT_GROUP_SIZE;
        b[1].p3.y =  6         * AADL_PORT_GROUP_SIZE;

        b[2].p1.x = -2         * AADL_PORT_GROUP_SIZE;
        b[2].p1.y =  9         * AADL_PORT_GROUP_SIZE;

        b[3].p1.x = -9         * AADL_PORT_GROUP_SIZE;
        b[3].p1.y =  7         * AADL_PORT_GROUP_SIZE;
        b[3].p2.x = -9         * AADL_PORT_GROUP_SIZE;
        b[3].p2.y = -7         * AADL_PORT_GROUP_SIZE;
        b[3].p3.x = -2         * AADL_PORT_GROUP_SIZE;
        b[3].p3.y = -9         * AADL_PORT_GROUP_SIZE;

        b[4].p1.x = -2         * AADL_PORT_GROUP_SIZE;
        b[4].p1.y = -6         * AADL_PORT_GROUP_SIZE;

        for (i=0; i<5; i++) {
          rotate_around_origin (&b[i].p1, port->angle);
          point_add (&b[i].p1, &port->handle->pos);

          if (b[i].type == BEZ_CURVE_TO) {
            rotate_around_origin (&b[i].p2, port->angle);
            rotate_around_origin (&b[i].p3, port->angle);

            point_add (&b[i].p2, &port->handle->pos);
            point_add (&b[i].p3, &port->handle->pos);
          }
        }

        dia_renderer_draw_beziergon (renderer, b, 5, &color_black, &color_black);
      }
      break;

    case BUS:
    case DEVICE:
    case MEMORY:
    case PROCESSOR:
    case PROCESS:
    case SUBPROGRAM:
    case SYSTEM:
    case THREAD:
    case THREAD_GROUP:
    default:
      break;
  }
}



void aadlbox_update_port(Aadlbox *aadlbox, Aadlport *port)
{
  /* Ports are always kept on the borders of the box */
  aadlbox->specific->project_point_on_nearest_border(aadlbox,
					     &port->handle->pos, &port->angle);

  /* reinit in & out connection point to default position */
  /* FIXME: should i make methods ? */
  switch (port->type) {

    case ACCESS_PROVIDER:
      port->in.pos.x = - AADL_ACCESS_WIDTH_A;
      port->in.pos.y=0;

      port->out.pos.x=   AADL_ACCESS_WIDTH_B + AADL_ACCESS_WIDTH_C;
      port->out.pos.y=0;
      break;

    case ACCESS_REQUIRER:
      port->in.pos.x =   AADL_ACCESS_WIDTH_A;
      port->in.pos.y=0;

      port->out.pos.x=  - AADL_ACCESS_WIDTH_B - AADL_ACCESS_WIDTH_C;
      port->out.pos.y=0;
      break;

    case IN_DATA_PORT:
      port->in.pos.x =  AADL_PORT_WIDTH_A; port->in.pos.y=0;
      port->out.pos.x= -AADL_PORT_WIDTH_B; port->out.pos.y=0;
      break;

    case IN_EVENT_PORT:
    case IN_EVENT_DATA_PORT:
      port->in.pos.x =  AADL_PORT_WIDTH_A;
      port->in.pos.y=0;

      port->out.pos.x= -AADL_PORT_WIDTH_B - AADL_EVENT_DELTA_WIDTH;
      port->out.pos.y=0;
      break;

    case OUT_DATA_PORT:
      port->in.pos.x = -AADL_PORT_WIDTH_A; port->in.pos.y=0;
      port->out.pos.x=  AADL_PORT_WIDTH_B; port->out.pos.y=0;
      break;

    case OUT_EVENT_PORT:
    case OUT_EVENT_DATA_PORT:
      port->in.pos.x = -AADL_PORT_WIDTH_A;
      port->in.pos.y=0;

      port->out.pos.x=  AADL_PORT_WIDTH_B + AADL_EVENT_DELTA_WIDTH;
      port->out.pos.y=0;
      break;

    case IN_OUT_DATA_PORT:
      port->in.pos.x = -AADL_PORT_WIDTH_B; port->in.pos.y=0;
      port->out.pos.x=  AADL_PORT_WIDTH_B; port->out.pos.y=0;
      break;

    case  IN_OUT_EVENT_PORT:
    case  IN_OUT_EVENT_DATA_PORT:
      port->in.pos.x = -AADL_PORT_WIDTH_B - AADL_EVENT_DELTA_WIDTH;
      port->in.pos.y=0;
      port->out.pos.x=  AADL_PORT_WIDTH_B + AADL_EVENT_DELTA_HEIGHT;
      port->out.pos.y=0;
      break;

    case PORT_GROUP:  /* quick n dirty */
      port->in.pos.x = -9*AADL_PORT_GROUP_SIZE; port->in.pos.y=0;
      port->out.pos.x = 3*AADL_PORT_GROUP_SIZE; port->out.pos.y=0;
      break;

    case BUS:
    case DEVICE:
    case MEMORY:
    case PROCESSOR:
    case PROCESS:
    case SUBPROGRAM:
    case SYSTEM:
    case THREAD:
    case THREAD_GROUP:
    default:
      break;
  }

  rotate_around_origin(&port->in.pos, port->angle);
  rotate_around_origin(&port->out.pos, port->angle);

  point_add(&port->in.pos, &port->handle->pos);
  point_add(&port->out.pos, &port->handle->pos);

  /* direction hints */
/*
  if (port->angle >= M_PI/4.0 && port->angle < 3.0*M_PI/4.0)
    port->out.directions = DIR_SOUTH;
  else if (port->angle >= 3.0*M_PI/4.0 && port->angle < 5.0*M_PI/4.0)
    port->out.directions = DIR_WEST;
  else if (port->angle >= 5.0*M_PI/4.0 && port->angle < 7.0*M_PI/4.0)
    port->out.directions = DIR_NORTH;
  else
    port->out.directions = DIR_EAST;
*/
}


void
aadlbox_update_ports (Aadlbox *aadlbox)
{
  for (int i = 0; i < aadlbox->num_ports; i++) {
    aadlbox_update_port (aadlbox, aadlbox->ports[i]);
  }
}
