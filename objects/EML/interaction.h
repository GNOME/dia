#ifndef INTERACTION_H
#define INTERACTION_H

#include <assert.h>
#include <gtk/gtk.h>
#include <math.h>
#include <string.h>

#include "config.h"
#include "intl.h"
#include "object.h"
#include "connection.h"
#include "handle.h"
#include "orth_conn.h"
#include "render.h"
#include "attributes.h"
#include "arrows.h"
#include "text.h"

#include "eml.h"
  
#define INTERACTION_WIDTH 0.1
#define INTERACTION_FONTHEIGHT 0.8
#define INTERACTION_ARROWLEN 0.8
#define INTERACTION_ARROWWIDTH 0.8

typedef enum {
    INTER_UNIDIR,
    INTER_BIDIR
} InteractionType;

typedef struct _InteractionState InteractionState;
typedef struct _InteractionDialog InteractionDialog;

struct _InteractionState {
  ObjectState obj_state;
  
  InteractionType type;
  char *text;
};

struct _InteractionDialog {
  GtkWidget *dialog;
  
  GtkToggleButton *type;
  GtkEntry *text;
};

extern void interaction_draw_buttom_halfhead(Renderer *, Point *, Point *,
                                             real, real, real, Color *);

#endif
