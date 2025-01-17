/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * This code renders boolean equations, as needed by the transitions'
 * receptivities.
 *
 * Copyright (C) 2000 Cyrille Chepelov
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
#include "boolequation.h"

#define OVERLINE_RATIO .1

typedef enum {BLOCK_COMPOUND, BLOCK_OPERATOR, BLOCK_OVERLINE,
	      BLOCK_PARENS, BLOCK_TEXT} BlockType;
typedef enum {OP_AND, OP_OR, OP_XOR, OP_RISE, OP_FALL,
              OP_EQUAL,OP_LT,OP_GT} OperatorType;

typedef void (*BlockGetBBFunc)(Block *block, Point *relpos, Boolequation *booleq,
			       DiaRectangle *rect);
typedef void (*BlockDrawFunc)(Block *block,
			      Boolequation *booleq,
			      DiaRenderer *render);
typedef void (*BlockDestroyFunc)(Block *block);

typedef struct {
  BlockGetBBFunc get_boundingbox;
  BlockDrawFunc draw;
  BlockDestroyFunc destroy;
} BlockOps;


struct _Block {
  BlockType type;
  BlockOps *ops;
  Point bl, ur, pos;
  union {
    Block *inside; /* overline, parens */
    char *text;
    GSList *contained;
    OperatorType operator;
  } d;
};


/* utility */
inline static gboolean isspecial(gunichar c)
{
  switch (c) {
  case '!':
  case '(':
  case ')':
  case '&':
  case '|':
  case '^':
  case '+':
  case '.':
  case '*':
  case '{':
  case '}':
    return TRUE;
  default:
    return FALSE;
  }
}

/* Text block definition */
static void
textblock_get_boundingbox (Block        *block,
                           Point        *relpos,
                           Boolequation *booleq,
                           DiaRectangle *rect)
{
  g_assert(block); g_assert(block->type == BLOCK_TEXT);

  block->pos = *relpos;
  block->bl.x = block->pos.x;
  block->bl.y = block->pos.y + dia_font_descent(block->d.text,
                                                booleq->font,
                                                booleq->fontheight);
  block->ur.y = block->pos.y - dia_font_ascent(block->d.text,
                                               booleq->font,
                                               booleq->fontheight);
  block->ur.x = block->bl.x + dia_font_string_width(block->d.text,
                                                    booleq->font,
                                                    booleq->fontheight);
  rect->left = block->bl.x;
  rect->top = block->ur.y;
  rect->bottom = block->bl.y;
  rect->right = block->ur.x;
}

static void
textblock_draw (Block *block, Boolequation *booleq, DiaRenderer *renderer)
{
  g_assert(block); g_assert(block->type == BLOCK_TEXT);
  dia_renderer_set_font (renderer,booleq->font,booleq->fontheight);
  dia_renderer_draw_string (renderer,
                            block->d.text,
                            &block->pos,
                            DIA_ALIGN_LEFT,
                            &booleq->color);
}


static void
textblock_destroy (Block *block)
{
  if (!block) return;

  g_return_if_fail (block->type == BLOCK_TEXT);

  g_clear_pointer (&block->d.text, g_free);
  g_free (block);
}


static BlockOps text_block_ops = {
  textblock_get_boundingbox,
  textblock_draw,
  textblock_destroy,
};


static Block *
textblock_create (const char **str)
{
  const char *p = *str;
  Block *block;

  while (**str) {
    gunichar c = g_utf8_get_char (*str);
    const char *p1 = g_utf8_next_char (*str);

    if (isspecial (c)) {
      break;
    }

    *str = p1;
  }

  block = g_new0 (Block, 1);
  block->type = BLOCK_TEXT;
  block->ops = &text_block_ops;
  block->d.text = g_strndup (p, *str - p);

  return block;
}


/* Operator block */
static const gchar xor_symbol[] = { 226, 138, 149, 0 }; /* 0x2295 */
static const gchar and_symbol[] = { 226, 136, 153, 0 }; /* 0x2219 */
static const gchar rise_symbol[] = { 226, 134, 145, 0 }; /* 0x2191 */
static const gchar fall_symbol[] = { 226, 134, 147, 0 }; /* 0x2193 */
static const gchar or_symbol[] = { 43, 0 };
static const gchar lt_symbol[] = { 60,0 };
static const gchar equal_symbol[] = { 61,0 };
static const gchar gt_symbol[] = { 62,0 };

static const gchar *opstring(OperatorType optype)
{
  switch (optype) {

  case OP_AND: return and_symbol;
  case OP_OR: return or_symbol;
  case OP_XOR: return xor_symbol;
  case OP_RISE: return rise_symbol;
  case OP_FALL: return fall_symbol;
  case OP_EQUAL: return equal_symbol;
  case OP_LT: return lt_symbol;
  case OP_GT: return gt_symbol;
  default:
    break;
  }
  g_assert_not_reached();
  return NULL; /* never */
}

static void
opblock_get_boundingbox(Block *block, Point *relpos,
			Boolequation *booleq, DiaRectangle *rect)
{
  const gchar* ops;
  g_assert(block); g_assert(block->type == BLOCK_OPERATOR);

  ops = opstring(block->d.operator);

  block->pos = *relpos;
  block->bl.x = block->pos.x;
  block->bl.y = block->pos.y +
      dia_font_descent(ops,booleq->font,booleq->fontheight);
  block->ur.y = block->bl.y - booleq->fontheight;
  block->ur.x = block->bl.x + dia_font_string_width(ops, booleq->font,
                                                    booleq->fontheight);
  rect->left = block->bl.x;
  rect->top = block->ur.y;
  rect->bottom = block->bl.y;
  rect->right = block->ur.x;
}

static void
opblock_draw (Block *block, Boolequation *booleq, DiaRenderer *renderer)
{
  g_assert(block); g_assert(block->type == BLOCK_OPERATOR);
  dia_renderer_set_font (renderer,booleq->font,booleq->fontheight);
  dia_renderer_draw_string (renderer,
                            opstring (block->d.operator),
                            &block->pos,
                            DIA_ALIGN_LEFT,
                            &booleq->color);
}


static void
opblock_destroy (Block *block)
{
  if (!block) return;

  g_return_if_fail (block->type == BLOCK_OPERATOR);

  g_free (block);
}


static BlockOps operator_block_ops = {
  opblock_get_boundingbox,
  opblock_draw,
  opblock_destroy};

static Block *opblock_create(const gchar **str)
{
  Block *block;
  gunichar c;

  c = g_utf8_get_char(*str);
  *str = g_utf8_next_char(*str);

  block = g_new0(Block,1);
  block->type = BLOCK_OPERATOR;
  block->ops = &operator_block_ops;
  switch(c) {
  case '&':
  case '.':
    block->d.operator = OP_AND;
    break;
  case '|':
  case '+':
    block->d.operator = OP_OR;
    break;
  case '^':
  case '*':
    block->d.operator = OP_XOR;
    break;
  case '=':
    block->d.operator = OP_EQUAL;
    break;
  case '<':
    block->d.operator = OP_LT;
    break;
  case '>':
    block->d.operator = OP_GT;
    break;
  case '{':
    block->d.operator = OP_RISE;
    break;
  case '}':
    block->d.operator = OP_FALL;
    break;
  default:
    g_assert_not_reached();
  }
  return block;
}

/* Overlineblock : */
static void
overlineblock_get_boundingbox(Block *block, Point *relpos,
			      Boolequation *booleq, DiaRectangle *rect)
{
  g_assert(block); g_assert(block->type == BLOCK_OVERLINE);

  block->d.inside->ops->get_boundingbox(block->d.inside,relpos,booleq,rect);

  block->bl = block->d.inside->bl;
  block->ur.x = block->d.inside->ur.x;
  block->ur.y =
    block->d.inside->ur.y - (3.0 * OVERLINE_RATIO * booleq->fontheight);

  /*rect->left = bl.x; */
  rect->top = block->ur.y;
  /*rect->bottom = bl.y;
    rect->right = ur.x;*/
}

static void
overlineblock_draw (Block *block, Boolequation *booleq, DiaRenderer *renderer)
{
  Point ul,ur;
  g_assert(block); g_assert(block->type == BLOCK_OVERLINE);
  block->d.inside->ops->draw (block->d.inside,booleq,renderer);
  dia_renderer_set_linestyle (renderer, DIA_LINE_STYLE_SOLID, 0.0);
  dia_renderer_set_linewidth (renderer,booleq->fontheight * OVERLINE_RATIO);
  ul.x = block->bl.x;
  ur.y = ul.y = block->ur.y;

      /* FIXME: try to get the actual block width */
  ur.x = block->ur.x -
      (dia_font_string_width ("_", booleq->font,booleq->fontheight) / 2);

  dia_renderer_draw_line (renderer,&ul,&ur,&booleq->color);
}


static void
overlineblock_destroy (Block *block)
{
  if (!block) return;

  g_return_if_fail(block->type == BLOCK_OVERLINE);

  block->d.inside->ops->destroy(block->d.inside);

  g_free (block);
}


static BlockOps overline_block_ops = {
  overlineblock_get_boundingbox,
  overlineblock_draw,
  overlineblock_destroy};

static Block *overlineblock_create(Block *inside)
{
  Block *block;
  /* *str is already in the inside */
  block = g_new0(Block,1);
  block->type = BLOCK_OVERLINE;
  block->ops = &overline_block_ops;
  block->d.inside = inside;

  return block;
}

/* Parensblock : */
static void
parensblock_get_boundingbox(Block *block, Point *relpos,
			    Boolequation *booleq, DiaRectangle *rect)
{
  real pheight,pwidth;
  Point temppos;
  g_assert(block); g_assert(block->type == BLOCK_PARENS);

  temppos = block->pos = *relpos;
  block->d.inside->ops->get_boundingbox(block->d.inside,&temppos,booleq,rect);
  pheight = 1.1 * (block->d.inside->bl.y - block->d.inside->ur.y);
  pwidth = dia_font_string_width("()",booleq->font,pheight) / 2;
  temppos.x += pwidth;
  block->d.inside->ops->get_boundingbox(block->d.inside,&temppos,booleq,rect);

  block->bl.x = block->pos.x;
  block->bl.y = block->pos.y + dia_font_descent("()",booleq->font,pheight);
  block->ur.x = block->d.inside->ur.x + pwidth;
  block->ur.y = block->bl.y - pheight;

  rect->left = block->bl.x;
  rect->top = block->ur.y;
  rect->bottom = block->bl.y;
  rect->right = block->ur.x;
}

static void
parensblock_draw (Block *block, Boolequation *booleq, DiaRenderer *renderer)
{
  Point pt;
  real pheight;

  g_assert(block); g_assert(block->type == BLOCK_PARENS);

  pheight = block->d.inside->bl.y - block->d.inside->ur.y;
  block->d.inside->ops->draw (block->d.inside,booleq,renderer);

  dia_renderer_set_font (renderer,booleq->font,pheight);
  pt.y = block->pos.y;
  pt.x = block->d.inside->ur.x;

  dia_renderer_draw_string (renderer,
                            "(",
                            &block->pos,
                            DIA_ALIGN_LEFT,
                            &booleq->color);
  dia_renderer_draw_string (renderer,
                            ")",
                            &pt,
                            DIA_ALIGN_LEFT,
                            &booleq->color);
}


static void
parensblock_destroy (Block *block)
{
  if (!block) return;

  g_return_if_fail (block->type == BLOCK_PARENS);

  block->d.inside->ops->destroy (block->d.inside);

  g_free (block);
}


static BlockOps parens_block_ops = {
  parensblock_get_boundingbox,
  parensblock_draw,
  parensblock_destroy};

static Block *parensblock_create(Block *inside)
{
  Block *block;
  /* *str is already in the inside */
  block = g_new0(Block,1);
  block->type = BLOCK_PARENS;
  block->ops = &parens_block_ops;
  block->d.inside = inside;

  return block;
}

/* Compoundblock : */
static void
compoundblock_get_boundingbox(Block *block, Point *relpos,
			      Boolequation *booleq, DiaRectangle *rect)
{
  GSList *elem;
  Block *inblk;
  Point pos;
  DiaRectangle inrect;

  g_assert(block); g_assert(block->type == BLOCK_COMPOUND);

  pos = block->pos = *relpos;

  inrect.right = inrect.left = pos.x;
  inrect.top = inrect.bottom = pos.y;

  *rect = inrect;

  elem = block->d.contained;
  while (elem) {
    inblk = (Block *)(elem->data);
    if (!inblk) break;

    inblk->ops->get_boundingbox(inblk,&pos,booleq,&inrect);
    rectangle_union(rect,&inrect);

    pos.x = inblk->ur.x;

    elem = g_slist_next(elem);
  }
  block->ur.x = rect->right;
  block->ur.y = rect->top;
  block->bl.x = rect->left;
  block->bl.y = rect->bottom;
}

static void compoundblock_draw(Block *block,
			       Boolequation *booleq,
			       DiaRenderer *renderer)
{
  GSList *elem;
  Block *inblk;
  g_assert(block); g_assert(block->type == BLOCK_COMPOUND);

  elem = block->d.contained;
  while (elem) {
    inblk = (Block *)(elem->data);
    if (!inblk) break;

    inblk->ops->draw(inblk,booleq,renderer);

    elem = g_slist_next(elem);
  }
}

static void
compoundblock_destroy(Block *block)
{
  GSList *elem;
  Block *inblk;

  if (!block) return;
  g_assert(block->type == BLOCK_COMPOUND);

  elem = block->d.contained;
  while (elem) {
    inblk = (Block *)(elem->data);
    if (!inblk) break;

    inblk->ops->destroy(inblk);
    elem->data = NULL;

    elem = g_slist_next(elem);
  }

  g_slist_free(block->d.contained);
  g_clear_pointer (&block, g_free);
}


static BlockOps compound_block_ops = {
  compoundblock_get_boundingbox,
  compoundblock_draw,
  compoundblock_destroy,
};


static Block *
compoundblock_create (const char **str)
{
  Block *block, *inblk;

  block = g_new0 (Block, 1);
  block->type = BLOCK_COMPOUND;
  block->ops = &compound_block_ops;
  block->d.contained = NULL;

  while (*str && **str) {
    gunichar c = g_utf8_get_char (*str);
    const char *p = g_utf8_next_char (*str);

    inblk = NULL;
    switch (c) {
    case '&':
    case '^':
    case '|':
    case '+':
    case '.':
    case '*':
    case '=':
    case '>':
    case '<':
    case '}':
    case '{':
      inblk = opblock_create(str);
      break;
    case '(':
      *str = p;
      inblk = parensblock_create(compoundblock_create(str));
      break;
    case '!':
      *str = p;
      c = g_utf8_get_char (*str);
      p = g_utf8_next_char (*str);
      if (c == '(') {
        *str = p;
        inblk = overlineblock_create (compoundblock_create (str));
      } else { /* single identifier "not" */
        inblk = overlineblock_create (textblock_create (str));
      }
      break;
    case ')': /* Whooops ! outta here ! */
      *str = p;
      return block;
    default:
      inblk = textblock_create(str);
    }
    if (inblk) {
      block->d.contained = g_slist_append(block->d.contained,inblk);
    }
  }
  /* We should fall here exactly once : at the end of the root block. */
  return block;
}


/* Boolequation : */
void
boolequation_set_value (Boolequation *booleq, const char *value)
{
  g_return_if_fail (booleq);
  g_clear_pointer (&booleq->value, g_free);

  if (booleq->rootblock) {
    booleq->rootblock->ops->destroy (booleq->rootblock);
  }

  booleq->value = g_strdup (value);
  booleq->rootblock = compoundblock_create (&value);
  /* a good bounding box recalc here would be nice. */
}


Boolequation *
boolequation_create (const gchar *value,
                     DiaFont     *font,
                     real         fontheight,
                     Color       *color)
{
  Boolequation *booleq;

  booleq = g_new0 (Boolequation,1);
  booleq->font = g_object_ref (font);
  booleq->fontheight = fontheight;
  booleq->color = *color;
  boolequation_set_value (booleq,value);

  return booleq;
}


void
boolequation_destroy(Boolequation *booleq)
{
  g_return_if_fail (booleq);
  g_clear_object (&booleq->font);
  g_clear_pointer (&booleq->value, g_free);

  if (booleq->rootblock) booleq->rootblock->ops->destroy (booleq->rootblock);

  g_free (booleq);
}


void
save_boolequation(ObjectNode obj_node, const gchar *attrname,
		  Boolequation *booleq, DiaContext *ctx)
{
  data_add_string(new_attribute(obj_node,attrname),(gchar *)booleq->value, ctx);
}

void
boolequation_draw(Boolequation *booleq, DiaRenderer *renderer)
{
  if (booleq->rootblock) {
    booleq->rootblock->ops->draw(booleq->rootblock,booleq,renderer);
  }
}

void
boolequation_calc_boundingbox(Boolequation *booleq, DiaRectangle *box)
{
        /*
          booleq->ascent = dia_font_ascent(booleq->font,booleq->fontheight);
          booleq->descent = dia_font_descent(booleq->font,booleq->fontheight);
        */

  box->left = box->right = booleq->pos.x;
  box->top = box->bottom = booleq->pos.y;

  if (booleq->rootblock) {
    booleq->rootblock->ops->get_boundingbox(booleq->rootblock,
                                            &booleq->pos,booleq,box);
  }

  booleq->width = box->right - box->left;
  booleq->height = box->bottom - box->top;
}

