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


#include <config.h>

#include <math.h>
#include <string.h>

#include "object.h"
#include "element.h"
#include "diarenderer.h"
#include "attributes.h"
#include "text.h"
#include "properties.h"

#define DIA_AADL_TYPE_POINT_OBJECT_CHANGE dia_aadl_point_object_change_get_type ()
G_DECLARE_FINAL_TYPE (DiaAADLPointObjectChange,
                      dia_aadl_point_object_change,
                      DIA_AADL, POINT_OBJECT_CHANGE,
                      DiaObjectChange)


#undef min
#define min(a,b) (a<b?a:b)
#undef max
#define max(a,b) (a>b?a:b)

/***********************************************
 **              CONSTANTS                    **
 ***********************************************/

static const double AADLBOX_BORDERWIDTH = 0.1;
static const double AADLBOX_FONTHEIGHT = 0.8;
static const double AADLBOX_TEXT_MARGIN = 0.5;

#define AADL_PORT_MAX_OUT 1.0  /* Maximum size out of the box */

#define AADL_MEMORY_FACTOR 0.1
#define AADLBOX_INCLINE_FACTOR 0.2
#define AADLBOX_DASH_LENGTH 0.3
#define AADL_BUS_ARROW_SIZE_FACTOR 0.16
#define AADL_BUS_HEIGHT_FACTOR 0.3
#define AADL_ROUNDEDBOX_CORNER_SIZE_FACTOR 0.25

/***********************************************
 **                  TYPES                    **
 ***********************************************/

typedef struct _Aadlport Aadlport;
typedef struct _Aadlbox Aadlbox;

typedef struct _Aadlbox_specific Aadlbox_specific;


struct _Aadlbox
{
  Element element;

  gchar *declaration;

  Text *name;

  int num_ports;
  Aadlport **ports;

  int num_connections;
  ConnectionPoint **connections;

  Color line_color;
  Color fill_color;

  Aadlbox_specific *specific;
};


typedef enum {
  BUS,
  DEVICE,
  MEMORY,
  PROCESSOR,
  PROCESS,
  SUBPROGRAM,
  SYSTEM,
  THREAD,
  THREAD_GROUP,

  ACCESS_PROVIDER,
  ACCESS_REQUIRER,
  IN_DATA_PORT,
  IN_EVENT_PORT,
  IN_EVENT_DATA_PORT,
  OUT_DATA_PORT,
  OUT_EVENT_PORT,
  OUT_EVENT_DATA_PORT,
  IN_OUT_DATA_PORT,
  IN_OUT_EVENT_PORT,
  IN_OUT_EVENT_DATA_PORT,

  PORT_GROUP
} Aadl_type;


struct _Aadlport {
  Aadl_type type;
  Handle *handle;
  real angle;

  ConnectionPoint in;
  ConnectionPoint out;

  gchar *declaration;
};

typedef void (*AadlProjectionFunc) (Aadlbox *aadlbox,Point *p,real *angle);
typedef void (*AadlDrawFunc) (Aadlbox* obj, DiaRenderer* ddisp);
typedef void (*AadlTextPosFunc) (Aadlbox* obj, Point *p);
typedef void (*AadlSizeFunc) (Aadlbox* obj, Point *size);

/* In the abstract class system (see aadlbox.c), these are the
   functions an inherited class *must* define, because they are
   used by aadlbox_ functions   */

struct _Aadlbox_specific
{
  AadlProjectionFunc project_point_on_nearest_border;
  AadlTextPosFunc text_position;
  AadlSizeFunc min_size;
};


/***********************************************
 **                 FUNCTIONS                 **
 ***********************************************/


/* aadltext.c */
void aadldata_text_position(Aadlbox *aadlbox, Point *p);
void aadldata_minsize(Aadlbox *aadlbox, Point *size);

void aadlprocess_text_position(Aadlbox *aadlbox, Point *p);
void aadlprocess_minsize(Aadlbox *aadlbox, Point *size);

void aadlbus_text_position(Aadlbox *aadlbox, Point *p);
void aadlbus_minsize(Aadlbox *aadlbox, Point *size);

void aadlsystem_text_position(Aadlbox *aadlbox, Point *p);
void aadlsystem_minsize(Aadlbox *aadlbox, Point *size);

void aadlsubprogram_text_position(Aadlbox *aadlbox, Point *p);
void aadlsubprogram_minsize(Aadlbox *aadlbox, Point *size);

void aadlmemory_text_position(Aadlbox *aadlbox, Point *p);
void aadlmemory_minsize(Aadlbox *aadlbox, Point *size);

/* aadlport.c */
void rotate_around_origin (Point *p, real angle);
void aadlbox_draw_port(Aadlport *port, DiaRenderer *renderer);
void aadlbox_update_port(Aadlbox *aadlbox, Aadlport *port);
void aadlbox_update_ports(Aadlbox *aadlbox);

/* aadlbox.c */
real aadlbox_distance_from(Aadlbox *aadlbox, Point *point);
void aadlbox_select(Aadlbox *aadlbox, Point *clicked_point,
		    DiaRenderer *interactive_renderer);
DiaObjectChange *aadlbox_move_handle (Aadlbox          *aadlbox,
                                      Handle           *handle,
                                      Point            *to,
                                      ConnectionPoint  *cp,
                                      HandleMoveReason  reason,
                                      ModifierKeys      modifiers);
DiaObjectChange *aadlbox_move        (Aadlbox          *aadlbox,
                                      Point            *to);
void aadlbox_draw(Aadlbox *aadlbox, DiaRenderer *renderer);
DiaObject *aadlbox_create(Point *startpoint, void *user_data,
			  Handle **handle1, Handle **handle2);
void aadlbox_destroy(Aadlbox *aadlbox);
PropDescription *aadlbox_describe_props(Aadlbox *aadlbox);
void aadlbox_get_props(Aadlbox *aadlbox, GPtrArray *props);
void aadlbox_set_props(Aadlbox *aadlbox, GPtrArray *props);
DiaObject *aadlbox_copy(DiaObject *obj);
DiaMenu * aadlbox_get_object_menu(Aadlbox *aadlbox, Point *clickedpoint);
void aadlbox_save(Aadlbox *aadlbox, ObjectNode obj_node, DiaContext *ctx);
void aadlbox_load(ObjectNode obj_node, int version, DiaContext *ctx,
			Aadlbox *aadlbox);

/* aadldata.c */
void aadlbox_project_point_on_rectangle(DiaRectangle *rectangle,
					Point *p,real *angle);
 void aadldata_project_point_on_nearest_border(Aadlbox *aadlbox,
 					 Point *p,real *angle);

/* aadlprocess.c */

void
aadlbox_inclined_project_point_on_nearest_border(Aadlbox *aadlbox,Point *p,
						 real *angle);
void aadlprocess_text_position(Aadlbox *aadlbox, Point *p);

/* aadlthread.c */
void aadlbox_draw_inclined_box (Aadlbox      *aadlbox,
                                DiaRenderer  *renderer,
                                DiaLineStyle  linestyle);
/* aadlsubprogram.c */
void
aadlsubprogram_project_point_on_nearest_border(Aadlbox *aadlbox,Point *p,
					       real *angle);
/* aadlsystem.c */
void aadlbox_draw_rounded_box  (Aadlbox      *aadlbox,
                                DiaRenderer  *renderer,
                                DiaLineStyle  linestyle);


