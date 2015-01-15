/* -*- c-basic-offset:4; -*- */
/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1999 Alexander Larsson
 *
 * Sozi objects support
 * Copyright (C) 2015 Paul Chavent
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

/* standard libs*/
#include <assert.h>

/* xpath for searching in the svg doc */
#include <libxml/xpath.h>

/* dia stuff */
#include "object.h"
#include "diarenderer.h"
#include "diasvgrenderer.h"
#include "properties.h"

/* FIXME : legal dia stuff ? */
#include "prop_inttypes.h"
#include "propinternals.h"

/* sozi stuff */
#include "pixmaps/sozi-frame.xpm"
#include "sozi-object.h"

/******************************************************************************
 *
 *****************************************************************************/

typedef enum
{
    TRANSITION_LINEAR                      ,
    TRANSITION_ACCELERATE                  ,
    TRANSITION_STRONG_ACCELERATE           ,
    TRANSITION_DECELERATE                  ,
    TRANSITION_STRONG_DECELERATE           ,
    TRANSITION_ACCELERATE_DECELERATE       ,
    TRANSITION_STRONG_ACCELERATE_DECELERATE,
    TRANSITION_DECELERATE_ACCELERATE       ,
    TRANSITION_STRONG_DECELERATE_ACCELERATE,
    TRANSITION_IMMEDIATE_BEGINNING         ,
    TRANSITION_IMMEDIATE_END               ,
    TRANSITION_IMMEDIATE_MIDDLE            ,
} TransitionProfileType;

static const gchar * TransitionProfileDescr [] =
{
    "linear",
    "accelerate",
    "strong-accelerate",
    "decelerate",
    "strong-decelerate",
    "accelerate-decelerate",
    "strong-accelerate-decelerate",
    "decelerate-accelerate",
    "strong-decelerate-accelerate",
    "immediate-beginning",
    "immediate-end",
    "immediate-middle",
};

typedef struct _SoziFrame
{
    /* sozi object inheritance */

    SoziObject sozi_object;

    /* sozi frame specific stuff */

    int                   old_sequence; /* internal sequence numbering managment */
    int                   sequence;     /* final sequence number */

    gchar  *              title;
    gboolean              hide;
    gboolean              clip;
    gboolean              timeout_enable;
    int                   timeout_ms;
    TransitionProfileType transition_profile;
    int                   transition_duration_ms;

} SoziFrame;

/******************************************************************************
 * Dia object type operations
 *****************************************************************************/

static DiaObject * sozi_frame_create(Point *startpoint,
                                     void *user_data,
                                     Handle **handle1,
                                     Handle **handle2);
static DiaObject * sozi_frame_load(ObjectNode obj_node, int version, DiaContext *ctx);

static ObjectTypeOps sozi_frame_type_ops =
{
    (CreateFunc)        sozi_frame_create,            /* create */
    (LoadFunc)          sozi_frame_load,              /* load (== object_load_using_properties ?) */
    (SaveFunc)          object_save_using_properties, /* save  */
    (GetDefaultsFunc)   NULL,
    (ApplyDefaultsFunc) NULL,
};

DiaObjectType sozi_frame_type =
{
    "Sozi - Frame",           /* name */
    0,                        /* version */
    sozi_frame_xpm,           /* pixmap */
    &sozi_frame_type_ops,     /* ops */
    NULL,                     /* pixmap_file */
    0                         /* default_user_data */
};

/******************************************************************************
 * Dia object operations
 *****************************************************************************/

static DiaObject *       sozi_frame_copy(SoziFrame *sozi_frame);

static void              sozi_frame_destroy(SoziFrame *sozi_frame);

static void              sozi_frame_select(SoziFrame *sozi_frame, Point *clicked_point,
                                           DiaRenderer *interactive_renderer);

static void              sozi_frame_draw(SoziFrame *sozi_frame, DiaRenderer *renderer);

static real              sozi_frame_distance_from(SoziFrame *sozi_frame, Point *point);

static ObjectChange*     sozi_frame_move(SoziFrame *sozi_frame, Point *to);

static ObjectChange*     sozi_frame_move_handle(SoziFrame *sozi_frame, Handle *handle,
                                                Point *to, ConnectionPoint *cp,
                                                HandleMoveReason reason, ModifierKeys modifiers);
static GtkWidget *       sozi_frame_get_properties(SoziFrame *sozi_frame, gboolean is_default);

static PropDescription * sozi_frame_describe_props(SoziFrame *sozi_frame);

static void              sozi_frame_get_props(SoziFrame *sozi_frame, GPtrArray *props);

static void              sozi_frame_set_props(SoziFrame *sozi_frame, GPtrArray *props);


static ObjectOps sozi_frame_ops =
{
  (DestroyFunc)               sozi_frame_destroy,
  (DrawFunc)                  sozi_frame_draw,
  (DistanceFunc)              sozi_frame_distance_from,
  (SelectFunc)                sozi_frame_select,
  (CopyFunc)                  sozi_frame_copy,
  (MoveFunc)                  sozi_frame_move,
  (MoveHandleFunc)            sozi_frame_move_handle,
  (GetPropertiesFunc)         sozi_frame_get_properties,
  (ApplyPropertiesDialogFunc) object_apply_props_from_dialog,
  (ObjectMenuFunc)            0,
  (DescribePropsFunc)         sozi_frame_describe_props,
  (GetPropsFunc)              sozi_frame_get_props,
  (SetPropsFunc)              sozi_frame_set_props,
  (TextEditFunc)              0,
  (ApplyPropertiesListFunc)   object_apply_props,
};

/******************************************************************************
 * Functions for managing Sozi frame sequencing
 *****************************************************************************/

struct sequence_pair
{
    int old;
    int new;
};

static void
sozi_frames_reorder(gpointer data, gpointer user_data)
{
    /* see object.h for the description of the object structure */
    DiaObject *dia_object = (DiaObject *)data;
    if (strcmp(dia_object->type->name, sozi_frame_type.name) == 0) {
        SoziFrame *sozi_frame = (SoziFrame *)dia_object;
        struct sequence_pair * seq_pair = (struct sequence_pair *)user_data;
        int this_seq = sozi_frame->sequence;
        if ((seq_pair->new <= this_seq) && (this_seq < seq_pair->old)) {
            /* fprintf(stdout, "old value %d ++ new value %d\n", sozi_frame->sequence, sozi_frame->sequence + 1); */
            sozi_frame->sequence++;
            sozi_frame->old_sequence = sozi_frame->sequence;
            /* FIXME : not sure it's the good way to redraw an object */
            /* sozi_frame_draw(sozi_frame); */
        }
        else if ((seq_pair->old < this_seq) && (this_seq <= seq_pair->new)) {
            /* fprintf(stdout, "old value %d -- new value %d\n", sozi_frame->sequence, sozi_frame->sequence - 1); */
            sozi_frame->sequence--;
            sozi_frame->old_sequence = sozi_frame->sequence;
            /* FIXME : not sure it's the good way to redraw an object */
            /* sozi_frame_draw(sozi_frame); */
        }
    }
}

static void
sozi_frames_count(gpointer data, gpointer user_data)
{
    /* see object.h for the description of the object structure */
    DiaObject *dia_object = (DiaObject *)data;
    if (strcmp(dia_object->type->name, sozi_frame_type.name) == 0) {
        SoziFrame *sozi_frame = (SoziFrame *)dia_object;
        /* only object with old_sequence different of zero should be
           taken into account */
        if (sozi_frame->sequence) {
            int * counter = (int *)user_data;
            *counter = *counter + 1;
        }
    }
}

static void
sozi_frame_update(SoziFrame *sozi_frame)
{
    char legend[32];
    int nb_char;

    /* fprintf(stdout, "sozi_frame_update\n"); */

    /* probably editing default configuration or loading object 
       -> do not care */
    if (!sozi_frame->sozi_object.dia_object.parent_layer) {
        return;
    }

    /* fprintf(stdout, "sozi_frame_update : before : sequence = %d, old_sequence = %d\n", sozi_frame->sequence, sozi_frame->old_sequence); */

    /*
       sequence managment
       We use exemples of data_foreach_object of diagramdata.h
    */
    if (!sozi_frame->sequence) {
        /* this condition is true iif the object have just been created */
        int frames_count = 0;
        /* trig COUNTING of all frames */
        data_foreach_object(layer_get_parent_diagram(sozi_frame->sozi_object.dia_object.parent_layer), sozi_frames_count, &frames_count);
        /* initialize the current sequence number */
        /* fprintf(stdout, "seq must be initialized after object creation (%d frames)\n", frames_count); */
        sozi_frame->sequence = frames_count + 1;
    }
    else if (sozi_frame->old_sequence &&
            (sozi_frame->old_sequence != sozi_frame->sequence)) {
        /* this condition is true iif the object's sequence number
           have just been changed (in sozi_frame_set_props for instance) */
        struct sequence_pair seq_pair;
        seq_pair.old = sozi_frame->old_sequence;
        seq_pair.new = sozi_frame->sequence;
        /* fprintf(stdout, "seq %d has changed to %d\n", sozi_frame->old_sequence, sozi_frame->sequence); */
        /* temporary revert to the old sequence number for avoiding twins */
        sozi_frame->sequence = sozi_frame->old_sequence;
        /* trig REORDERING of all frames */
        data_foreach_object(layer_get_parent_diagram(sozi_frame->sozi_object.dia_object.parent_layer), sozi_frames_reorder, &seq_pair);
        /* restore the current sequence number */
        sozi_frame->sequence = seq_pair.new;
    }
    /* register the current sequence number */
    sozi_frame->old_sequence = sozi_frame->sequence;

    /* fprintf(stdout, "sozi_frame_update : after : sequence = %d, old_sequence = %d\n", sozi_frame->sequence, sozi_frame->old_sequence); */

    /* update legend */
    nb_char = g_snprintf(legend, sizeof(legend), "#%d : %s", sozi_frame->sequence, sozi_frame->title);
    if (sizeof(legend) < nb_char)
    {
       legend[sizeof(legend) - 1] = 0;
       legend[sizeof(legend) - 2] = '.';
       legend[sizeof(legend) - 3] = '.';
       legend[sizeof(legend) - 4] = '.';
    }
    text_set_string(sozi_frame->sozi_object.legend.text, legend);
}

/******************************************************************************
 * Functions for managing the Sozi object life managment (init, copy, kill)
 *****************************************************************************/

static void
sozi_frame_init(SoziFrame *sozi_frame)
{
    /* it will be initialized later, only if the object is placed on a */
    /* sheet, see sozi_frame_update here under */
    sozi_frame->old_sequence = 0;
    sozi_frame->sequence = 0;

    if (sozi_frame->title == NULL) {
        sozi_frame->title = g_strdup("frame title");
    }

    /* the sozi defaults can be found in Sozi/player/js/document.js */
    sozi_frame->hide = TRUE;
    sozi_frame->clip = TRUE;
    sozi_frame->timeout_enable = FALSE;
    sozi_frame->timeout_ms = 5000;
    sozi_frame->transition_profile = TRANSITION_LINEAR;
    sozi_frame->transition_duration_ms = 1000;
}

static void
sozi_frame_kill(SoziFrame *sozi_frame)
{
    if (sozi_frame->title != NULL) {
        g_free(sozi_frame->title);
    }
}

/******************************************************************************
 * Functions that implement the Dia fops
 *****************************************************************************/

static DiaObject *
sozi_frame_create(Point *startpoint,
                  void *user_data,
                  Handle **handle1,
                  Handle **handle2)
{
    SoziFrame *sozi_frame;

    sozi_frame = g_new0(SoziFrame, 1);

    /* init dia object */
    sozi_frame->sozi_object.dia_object.type = &sozi_frame_type;
    sozi_frame->sozi_object.dia_object.ops = &sozi_frame_ops;

    /* init sozi object : that is also the place where the dia object
     * geometry (position, handles, ...) will be initialized
     */
    sozi_object_init(&sozi_frame->sozi_object, startpoint);

    /* init sozi frame */
    sozi_frame_init(sozi_frame);

    /* update sozi object geometry */
    sozi_object_update(&sozi_frame->sozi_object);

    /* handle are not used */
    *handle1 = NULL; /* first handle to connect: not here */
    /* we intend to use the lower "SE" handle */
    g_assert (sozi_frame->sozi_object.dia_object.handles[2]->id == HANDLE_RESIZE_SE);
    *handle2 = sozi_frame->sozi_object.dia_object.handles[2]; /* second handle to resize */

    return &sozi_frame->sozi_object.dia_object;
}

/******************************************************************************
 *
 *****************************************************************************/
static DiaObject *
sozi_frame_load(ObjectNode obj_node, int version, DiaContext *ctx)
{
    DiaObject * obj;
    obj = object_load_using_properties(&sozi_frame_type, obj_node, version, ctx);
    /* sync sequence numbers since the object should not trig neither
       count neither reorder (see sozi_frame_update) */
    ((SoziFrame *)(obj))->old_sequence = ((SoziFrame *)(obj))->sequence;
    return obj;
}

/******************************************************************************
 *
 *****************************************************************************/
static DiaObject *
sozi_frame_copy(SoziFrame *sozi_frame)
{
    DiaObject * obj;
    obj = object_copy_using_properties((DiaObject *)sozi_frame);
    /* reset the sequence number so it can trigger the count (see
       sozi_frame_update) */
    ((SoziFrame *)(obj))->sequence = 0;
    ((SoziFrame *)(obj))->old_sequence = 0;
    return obj;
}

/******************************************************************************
 *
 *****************************************************************************/
static void
sozi_frame_destroy(SoziFrame *sozi_frame)
{
    /* kill sozi frame */
    sozi_frame_kill(sozi_frame);

    /* kill sozi object */
    sozi_object_kill(&sozi_frame->sozi_object);

    /* do NOT free sozi_frame, caller will take care about that */
}

/******************************************************************************
 *
 *****************************************************************************/
static void
sozi_frame_select(SoziFrame *sozi_frame, Point *clicked_point,
                  DiaRenderer *interactive_renderer)
{
    /* this callback will be called the first time the object is
       placed on the sheet, it allows to initialize the object */
    sozi_frame_update(sozi_frame);
    /* this one will update the legend */
    sozi_object_update(&sozi_frame->sozi_object);
}

/******************************************************************************
 *
 *****************************************************************************/
static void
sozi_frame_draw_svg(SoziFrame *sozi_frame, DiaSvgRenderer *svg_renderer)
{
    static unsigned refid_cnt = 0;
    xmlNs *sozi_name_space;
    xmlNodePtr root;
    xmlNodePtr node;
    xmlNodePtr rect;
    /* buffers for storing attributes */
    gchar * refid;
    gchar * sequence;
    gchar * timeout_ms;
    gchar * transition_duration_ms;
    const gchar * transition_profile;

    /* format attributes */

    refid                  = g_strdup_printf("sozi_frame_%d", refid_cnt++);
    sequence               = g_strdup_printf("%d", sozi_frame->sequence);
    timeout_ms             = g_strdup_printf("%d", sozi_frame->timeout_ms);
    transition_duration_ms = g_strdup_printf("%d", sozi_frame->transition_duration_ms);
    transition_profile     = (sozi_frame->transition_profile < sizeof(TransitionProfileDescr)/sizeof(TransitionProfileDescr[0]))?(TransitionProfileDescr[sozi_frame->transition_profile]):(TransitionProfileDescr[0]);

    /* draw the sozi base object and get a reference to the sozi
    namespace and document root */

    sozi_object_draw_svg(&sozi_frame->sozi_object, svg_renderer, refid, &sozi_name_space, &root, &rect);

    assert(sozi_name_space != NULL);

    /* add a "custom" frame for sozi */

    node = xmlNewChild(root, sozi_name_space, (const xmlChar *)"frame", NULL);
    xmlSetProp(node, (const xmlChar *)"sozi:refid"                  , (const xmlChar *)refid);
    xmlSetProp(node, (const xmlChar *)"sozi:title"                  , (const xmlChar *)sozi_frame->title);
    xmlSetProp(node, (const xmlChar *)"sozi:sequence"               , (const xmlChar *)sequence);
    xmlSetProp(node, (const xmlChar *)"sozi:hide"                   , (const xmlChar *)((sozi_frame->hide)?"true":"false"));
    xmlSetProp(node, (const xmlChar *)"sozi:clip"                   , (const xmlChar *)((sozi_frame->clip)?"true":"false"));
    xmlSetProp(node, (const xmlChar *)"sozi:show-in-frame-list"     , (const xmlChar *)"true");
    xmlSetProp(node, (const xmlChar *)"sozi:timeout-enable"         , (const xmlChar *)((sozi_frame->timeout_enable)?"true":"false"));
    xmlSetProp(node, (const xmlChar *)"sozi:timeout-ms"             , (const xmlChar *)timeout_ms);
    xmlSetProp(node, (const xmlChar *)"sozi:transition-duration-ms" , (const xmlChar *)transition_duration_ms);
    xmlSetProp(node, (const xmlChar *)"sozi:transition-zoom-percent", (const xmlChar *)"0");
    xmlSetProp(node, (const xmlChar *)"sozi:transition-profile"     , (const xmlChar *)transition_profile);
    xmlSetProp(node, (const xmlChar *)"sozi:transition-path-hide"   , (const xmlChar *)"true");

    /* free resources */
    g_free(refid);
    g_free(sequence);
    g_free(timeout_ms);
    g_free(transition_duration_ms);
}

/******************************************************************************
 *
 *****************************************************************************/

#define DIA_IS_INTERACTIVE_RENDERER(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), DIA_TYPE_INTERACTIVE_RENDERER))

static void
sozi_frame_draw(SoziFrame *sozi_frame, DiaRenderer *renderer)
{

    if (DIA_IS_SVG_RENDERER(renderer)) {
        sozi_frame_draw_svg(sozi_frame, DIA_SVG_RENDERER (renderer));
    }
    else if (DIA_GET_INTERACTIVE_RENDERER_INTERFACE (renderer) ||
            !sozi_frame->hide) {
        DiaRendererClass *renderer_ops = DIA_RENDERER_GET_CLASS (renderer);

        renderer_ops->set_linewidth(renderer, SOZI_OBJECT_LINE_WIDTH);
        renderer_ops->set_linecaps(renderer, LINECAPS_BUTT);
        renderer_ops->set_linejoin(renderer, LINEJOIN_MITER);
        renderer_ops->set_linestyle(renderer, LINESTYLE_SOLID, 0);

        renderer_ops->draw_polygon(renderer, sozi_frame->sozi_object.corners, 4, NULL, &color_black);

        if (sozi_frame->sozi_object.legend.disp == TRUE) {
            text_draw(sozi_frame->sozi_object.legend.text, renderer);
        }
    }
}

/******************************************************************************
 *
 *****************************************************************************/

static real
sozi_frame_distance_from(SoziFrame *sozi_frame, Point *point)
{
    return distance_polygon_point(sozi_frame->sozi_object.corners, 4, SOZI_OBJECT_LINE_WIDTH, point);
}

/******************************************************************************
 *
 *****************************************************************************/

static ObjectChange*
sozi_frame_move(SoziFrame *sozi_frame, Point *to)
{
    return sozi_object_move(&sozi_frame->sozi_object, to);
}

/******************************************************************************
 *
 *****************************************************************************/

static ObjectChange*
sozi_frame_move_handle(SoziFrame *sozi_frame, Handle *handle,
                       Point *to, ConnectionPoint *cp,
                       HandleMoveReason reason, ModifierKeys modifiers)
{
    return sozi_object_move_handle(&sozi_frame->sozi_object, handle,
                            to, cp, reason, modifiers);
}

/******************************************************************************
 *
 *****************************************************************************/

static PropEnumData prop_aspect_data[] =
{
    { N_("Free")  , ASPECT_FREE   },
    { N_("Fixed") , ASPECT_FIXED  },
    { NULL, 0 }
};

static PropEnumData prop_transition_profile_data[] =
{
    { N_("Constant speed")              , TRANSITION_LINEAR                      },
    { N_("Speed up")                    , TRANSITION_ACCELERATE                  },
    { N_("Speed up (strong)")           , TRANSITION_STRONG_ACCELERATE           },
    { N_("Speed down")                  , TRANSITION_DECELERATE                  },
    { N_("Speed down (strong)")         , TRANSITION_STRONG_DECELERATE           },
    { N_("Speed up, then down")         , TRANSITION_ACCELERATE_DECELERATE       },
    { N_("Speed up, then down (strong)"), TRANSITION_STRONG_ACCELERATE_DECELERATE},
    { N_("Speed down, then up")         , TRANSITION_DECELERATE_ACCELERATE       },
    { N_("Speed down, then up (strong)"), TRANSITION_STRONG_DECELERATE_ACCELERATE},
    { N_("Immediate (beginning)")       , TRANSITION_IMMEDIATE_BEGINNING         },
    { N_("Immediate (end)")             , TRANSITION_IMMEDIATE_END               },
    { N_("Immediate (middle)")          , TRANSITION_IMMEDIATE_MIDDLE            },
    { NULL, 0 }
};

static PropDescription sozi_frame_props[] =
{
    OBJECT_COMMON_PROPERTIES,

    PROP_STD_NOTEBOOK_BEGIN,

    PROP_NOTEBOOK_PAGE("geometry",0,N_("Geometry")),
    { "x"                , PROP_TYPE_REAL , PROP_FLAG_VISIBLE, N_("Center x")         , NULL, NULL },
    { "y"                , PROP_TYPE_REAL , PROP_FLAG_VISIBLE, N_("Center y")         , NULL, NULL },
    { "width"            , PROP_TYPE_REAL , PROP_FLAG_VISIBLE, N_("Width")            , NULL, NULL },
    { "height"           , PROP_TYPE_REAL , PROP_FLAG_VISIBLE, N_("Height")           , NULL, NULL },
    { "angle"            , PROP_TYPE_INT  , PROP_FLAG_VISIBLE, N_("Angle (deg)")      , NULL, NULL },
    { "aspect"           , PROP_TYPE_ENUM , PROP_FLAG_VISIBLE, N_("Aspect ratio")     , NULL, prop_aspect_data },
    { "scale_from_center", PROP_TYPE_BOOL , PROP_FLAG_VISIBLE, N_("Scale from center"), NULL, NULL },

    PROP_NOTEBOOK_PAGE("legend",0,N_("Legend")),
    { "legend_disp"      , PROP_TYPE_BOOL , PROP_FLAG_VISIBLE, N_("Legend is visible"), NULL, NULL},
    { "legend"           , PROP_TYPE_TEXT , 0                , NULL                   , NULL, NULL},
    PROP_STD_TEXT_ALIGNMENT,
    PROP_STD_TEXT_FONT,
    PROP_STD_TEXT_HEIGHT,
    PROP_STD_TEXT_COLOUR,

    PROP_NOTEBOOK_PAGE("sozi",0,N_("Sozi")),
    { "frame_sequence"                , PROP_TYPE_INT   , PROP_FLAG_VISIBLE, N_("Frame sequence")                 , NULL, NULL },
    { "frame_title"                   , PROP_TYPE_STRING, PROP_FLAG_VISIBLE, N_("Frame title")                    , NULL, NULL },
    { "frame_hide"                    , PROP_TYPE_BOOL  , PROP_FLAG_VISIBLE, N_("Frame hide")                     , NULL, NULL },
    { "frame_clip"                    , PROP_TYPE_BOOL  , PROP_FLAG_VISIBLE, N_("Frame clip")                     , NULL, NULL },
    { "frame_timeout_enable"          , PROP_TYPE_BOOL  , PROP_FLAG_VISIBLE, N_("Frame timeout enable")           , NULL, NULL },
    { "frame_timeout_ms"              , PROP_TYPE_INT   , PROP_FLAG_VISIBLE, N_("Frame timeout (ms)")             , NULL, NULL },
    { "frame_transition_profile"      , PROP_TYPE_ENUM  , PROP_FLAG_VISIBLE, N_("Frame transition profile")       , NULL, prop_transition_profile_data},
    { "frame_transition_duration_ms"  , PROP_TYPE_INT   , PROP_FLAG_VISIBLE, N_("Frame transition duration (ms)") , NULL, NULL },
    PROP_STD_NOTEBOOK_END,

    PROP_DESC_END
};

static PropOffset sozi_frame_offsets[] =
{
    OBJECT_COMMON_PROPERTIES_OFFSETS,

    PROP_OFFSET_STD_NOTEBOOK_BEGIN,

    PROP_OFFSET_NOTEBOOK_PAGE("geometry"),
    { "x"                     , PROP_TYPE_REAL          , offsetof(SoziFrame, sozi_object.center.x)          },
    { "y"                     , PROP_TYPE_REAL          , offsetof(SoziFrame, sozi_object.center.y)          },
    { "width"                 , PROP_TYPE_REAL          , offsetof(SoziFrame, sozi_object.width)             },
    { "height"                , PROP_TYPE_REAL          , offsetof(SoziFrame, sozi_object.height)            },
    { "angle"                 , PROP_TYPE_INT           , offsetof(SoziFrame, sozi_object.angle)             },
    { "aspect"                , PROP_TYPE_ENUM          , offsetof(SoziFrame, sozi_object.aspect)            },
    { "scale_from_center"     , PROP_TYPE_BOOL          , offsetof(SoziFrame, sozi_object.scale_from_center) },

    PROP_OFFSET_NOTEBOOK_PAGE("legend"),
    { "legend_disp"           , PROP_TYPE_BOOL          , offsetof(SoziFrame, sozi_object.legend.disp)             },
    { "legend"                , PROP_TYPE_TEXT          , offsetof(SoziFrame, sozi_object.legend.text)             },
    { "text_alignment"        , PROP_TYPE_ENUM          , offsetof(SoziFrame, sozi_object.legend.attrs.alignment)  },
    { "text_font"             , PROP_TYPE_FONT          , offsetof(SoziFrame, sozi_object.legend.attrs.font)       },
    { PROP_STDNAME_TEXT_HEIGHT, PROP_STDTYPE_TEXT_HEIGHT, offsetof(SoziFrame, sozi_object.legend.attrs.height)     },
    { "text_colour"           , PROP_TYPE_COLOUR        , offsetof(SoziFrame, sozi_object.legend.attrs.color)      },

    PROP_OFFSET_NOTEBOOK_PAGE("sozi"),
    { "frame_sequence"                 , PROP_TYPE_INT    , offsetof(SoziFrame, sequence)               },
    { "frame_title"                    , PROP_TYPE_STRING , offsetof(SoziFrame, title)                  },
    { "frame_hide"                     , PROP_TYPE_BOOL   , offsetof(SoziFrame, hide)                   },
    { "frame_clip"                     , PROP_TYPE_BOOL   , offsetof(SoziFrame, clip)                   },
    { "frame_timeout_enable"           , PROP_TYPE_BOOL   , offsetof(SoziFrame, timeout_enable)         },
    { "frame_timeout_ms"               , PROP_TYPE_INT    , offsetof(SoziFrame, timeout_ms)             },
    { "frame_transition_profile"       , PROP_TYPE_ENUM   , offsetof(SoziFrame, transition_profile)     },
    { "frame_transition_duration_ms"   , PROP_TYPE_INT    , offsetof(SoziFrame, transition_duration_ms) },

    PROP_OFFSET_STD_NOTEBOOK_END,

    {NULL}
};

static GtkWidget *
sozi_frame_get_properties(SoziFrame *sozi_frame, gboolean is_default)
{
    DiaObject * dia_object = &sozi_frame->sozi_object.dia_object;
    GtkWidget * widget = object_create_props_dialog(dia_object, is_default);

    /* the following code allow to set the range of the "frame sequence" property */
    /* see propdialogs.c for reference code with PropWidgetAssoc */
    PropDialog *dialog = prop_dialog_from_widget(widget);
    guint i;
    for (i = 0; i < dialog->prop_widgets->len; ++i) {
        PropWidgetAssoc *pwa = &g_array_index(dialog->prop_widgets,
                                              PropWidgetAssoc, i);
        if (pwa) {
            if (strcmp(pwa->prop->descr->name, "frame_sequence") == 0) {
                if (GTK_IS_SPIN_BUTTON(pwa->widget)) {
                    int count = 0;
                    data_foreach_object(layer_get_parent_diagram(dia_object->parent_layer), sozi_frames_count, &count);

                    gtk_spin_button_set_range(GTK_SPIN_BUTTON(pwa->widget), 1, count);
                }
            }
        }
    }
    return widget;
}

static PropDescription *
sozi_frame_describe_props(SoziFrame *sozi_frame)
{
    if (sozi_frame_props[0].quark == 0)
        prop_desc_list_calculate_quarks(sozi_frame_props);
    return sozi_frame_props;
}

static void
sozi_frame_get_props(SoziFrame *sozi_frame, GPtrArray *props)
{
    text_get_attributes(sozi_frame->sozi_object.legend.text, &sozi_frame->sozi_object.legend.attrs);
    object_get_props_from_offsets(&sozi_frame->sozi_object.dia_object, sozi_frame_offsets, props);
}

static void
sozi_frame_set_props(SoziFrame *sozi_frame, GPtrArray *props)
{
    object_set_props_from_offsets(&sozi_frame->sozi_object.dia_object, sozi_frame_offsets, props);
    apply_textattr_properties(props, sozi_frame->sozi_object.legend.text, "legend" , &sozi_frame->sozi_object.legend.attrs);
    sozi_frame_update(sozi_frame);
    sozi_object_update(&sozi_frame->sozi_object);
}
