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

#include "receptivity.h"

#define OVERLINE_RATIO .1

static Font *symbol;
static void init_symbolfont() {
  if (!symbol) symbol = font_getfont("Symbol");
}

typedef enum {BLOCK_COMPOUND, BLOCK_OPERATOR, BLOCK_OVERLINE,
	      BLOCK_PARENS, BLOCK_TEXT} BlockType;
typedef enum {OP_AND, OP_OR, OP_XOR, OP_RISE, OP_FALL } OperatorType;

typedef void (*BlockGetBBFunc)(Block *block, Point *relpos, Receptivity *rcep,
			       Rectangle *rect);
typedef void (*BlockDrawFunc)(Block *block, 
			      Receptivity *rcep, 
			      Renderer *render);
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
    const gchar *text;
    GSList *contained;
    OperatorType operator;
  } d;
};


/* utility */
inline static gboolean isspecial(char c) 
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
textblock_get_boundingbox(Block *block, Point *relpos, 
			  Receptivity *rcep, Rectangle *rect)
{
  g_assert(block); g_assert(block->type == BLOCK_TEXT);

  block->pos = *relpos;
  block->bl.x = block->pos.x;
  block->bl.y = block->pos.y + rcep->descent;
  block->ur.y = block->bl.y - rcep->fontheight;
  block->ur.x = block->bl.x + font_string_width(block->d.text,
						rcep->font,
						rcep->fontheight);
  rect->left = block->bl.x;
  rect->top = block->ur.y;
  rect->bottom = block->bl.y;
  rect->right = block->ur.x;
}

static void 
textblock_draw(Block *block,Receptivity *rcep,Renderer *renderer)
{
  g_assert(block); g_assert(block->type == BLOCK_TEXT);
  renderer->ops->set_font(renderer,rcep->font,rcep->fontheight);
  renderer->ops->draw_string(renderer,block->d.text,
			     &block->pos,ALIGN_LEFT,&rcep->color);
}

static void 
textblock_destroy(Block *block)
{
  if (!block) return;
  g_assert(block->type == BLOCK_TEXT);
  g_free((void *)block->d.text);
  g_free(block);
}

static BlockOps text_block_ops = {
  textblock_get_boundingbox,
  textblock_draw,
  textblock_destroy};
  
static Block *textblock_create(const char **str) 
{
  const char *p = *str;
  Block *block;

  g_assert(!isspecial(**str));
  while (**str && (!isspecial(**str))) (*str)++;
  
  block = g_new0(Block,1);
  block->type = BLOCK_TEXT;
  block->ops = &text_block_ops;
  block->d.text = g_strndup(p,*str-p);

  return block;
}

/* Operator block */
static const gchar xor_symbol[] = { 197, 0 };
static const gchar or_symbol[] = { 43, 0 };
static const gchar and_symbol[] = { 215,0 };
static const gchar rise_symbol[] = {173,0};
static const gchar fall_symbol[] = {175,0};

static const gchar *opstring(OperatorType optype)
{
  switch (optype) {
  case OP_AND: return and_symbol;
  case OP_OR: return or_symbol;
  case OP_XOR: return xor_symbol;
  case OP_RISE: return rise_symbol;
  case OP_FALL: return fall_symbol;
  default:
    break;
  }
  g_assert_not_reached();
  return NULL; /* never */
}

static void
opblock_get_boundingbox(Block *block, Point *relpos,
			Receptivity *rcep, Rectangle *rect)
{
  g_assert(block); g_assert(block->type == BLOCK_OPERATOR);
  
  
  block->pos = *relpos;
  block->bl.x = block->pos.x;
  block->bl.y = block->pos.y + font_descent(symbol,rcep->fontheight);
  block->ur.y = block->bl.y - rcep->fontheight;
  block->ur.x = block->bl.x + font_string_width(opstring(block->d.operator),
						symbol,
						rcep->fontheight);
  rect->left = block->bl.x;
  rect->top = block->ur.y;
  rect->bottom = block->bl.y;
  rect->right = block->ur.x;
}

static void 
opblock_draw(Block *block, Receptivity *rcep,Renderer *renderer)
{
  g_assert(block); g_assert(block->type == BLOCK_OPERATOR);
  renderer->ops->set_font(renderer,symbol,rcep->fontheight);
  renderer->ops->draw_string(renderer,opstring(block->d.operator),&block->pos,
			     ALIGN_LEFT,&rcep->color);
}
    
static void opblock_destroy(Block *block)
{
  if (!block) return;
  g_assert(block->type == BLOCK_OPERATOR);
  g_free(block);
}

static BlockOps operator_block_ops = {
  opblock_get_boundingbox,
  opblock_draw,
  opblock_destroy};

static Block *opblock_create(const char **str) 
{
  const char *p = *str;
  Block *block;

  (*str)++;
  block = g_new0(Block,1);
  block->type = BLOCK_OPERATOR;
  block->ops = &operator_block_ops;
  switch(*p) {
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
			      Receptivity *rcep, Rectangle *rect)
{
  g_assert(block); g_assert(block->type == BLOCK_OVERLINE);
  
  block->d.inside->ops->get_boundingbox(block->d.inside,relpos,rcep,rect);

  block->bl = block->d.inside->bl;
  block->ur.x = block->d.inside->ur.x;
  block->ur.y = 
    block->d.inside->ur.y - (3.0 * OVERLINE_RATIO * rcep->fontheight);

  /*rect->left = bl.x; */
  rect->top = block->ur.y;
  /*rect->bottom = bl.y;
    rect->right = ur.x;*/
}

static void 
overlineblock_draw(Block *block,Receptivity *rcep,Renderer *renderer)
{
  Point ul;
  g_assert(block); g_assert(block->type == BLOCK_OVERLINE);
  block->d.inside->ops->draw(block->d.inside,rcep,renderer);
  renderer->ops->set_linestyle(renderer,LINESTYLE_SOLID);
  renderer->ops->set_linewidth(renderer,rcep->fontheight * OVERLINE_RATIO);
  ul.x = block->bl.x;
  ul.y = block->ur.y;
  renderer->ops->draw_line(renderer,&ul,&block->ur,&rcep->color);
}

static void
overlineblock_destroy(Block *block)
{
  if (!block) return;
  g_assert(block->type == BLOCK_OVERLINE);
  block->d.inside->ops->destroy(block->d.inside);
  g_free(block);
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
			    Receptivity *rcep, Rectangle *rect)
{
  real pheight,pwidth;
  Point temppos;
  g_assert(block); g_assert(block->type == BLOCK_PARENS);

  temppos = block->pos = *relpos;
  block->d.inside->ops->get_boundingbox(block->d.inside,&temppos,rcep,rect);
  pheight = 1.1 * (block->d.inside->bl.y - block->d.inside->ur.y);
  pwidth = font_string_width("()",rcep->font,pheight) / 2;
  temppos.x += pwidth;
  block->d.inside->ops->get_boundingbox(block->d.inside,&temppos,rcep,rect);
  
  block->bl.x = block->pos.x;
  block->bl.y = block->pos.y + font_descent(rcep->font,pheight);
  block->ur.x = block->d.inside->ur.x + pwidth;
  block->ur.y = block->bl.y - pheight;

  rect->left = block->bl.x;
  rect->top = block->ur.y;
  rect->bottom = block->bl.y;
  rect->right = block->ur.x;
}

static void 
parensblock_draw(Block *block,Receptivity *rcep,Renderer *renderer)
{
  Point pt;
  real pheight;

  g_assert(block); g_assert(block->type == BLOCK_PARENS);

  pheight = block->d.inside->bl.y - block->d.inside->ur.y;
  block->d.inside->ops->draw(block->d.inside,rcep,renderer);

  renderer->ops->set_font(renderer,rcep->font,pheight);
  pt.y = block->pos.y;
  pt.x = block->d.inside->ur.x;

  renderer->ops->draw_string(renderer,"(",&block->pos,ALIGN_LEFT,&rcep->color);
  renderer->ops->draw_string(renderer,")",&pt,ALIGN_LEFT,&rcep->color);
}
 
static void
parensblock_destroy(Block *block)
{
  if (!block) return;
  g_assert(block->type == BLOCK_PARENS);
  block->d.inside->ops->destroy(block->d.inside);
  g_free(block);
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
			      Receptivity *rcep, Rectangle *rect)
{
  GSList *elem;
  Block *inblk;
  Point pos;
  Rectangle inrect;

  g_assert(block); g_assert(block->type == BLOCK_COMPOUND);

  pos = block->pos = *relpos;

  inrect.right = inrect.left = pos.x;
  inrect.top = inrect.bottom = pos.y;

  *rect = inrect;

  elem = block->d.contained;
  while (elem) {
    inblk = (Block *)(elem->data);
    if (!inblk) break;
    
    inblk->ops->get_boundingbox(inblk,&pos,rcep,&inrect);
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
			       Receptivity *rcep,
			       Renderer *renderer)
{
  GSList *elem;
  Block *inblk;
  g_assert(block); g_assert(block->type == BLOCK_COMPOUND);

  elem = block->d.contained;
  while (elem) {
    inblk = (Block *)(elem->data);
    if (!inblk) break;
    
    inblk->ops->draw(inblk,rcep,renderer);
    
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
  }

  g_slist_free(block->d.contained);
  g_free(block);
}

static BlockOps compound_block_ops = {
  compoundblock_get_boundingbox,
  compoundblock_draw,
  compoundblock_destroy};

static Block *
compoundblock_create(const char **str) 
{
  Block *block, *inblk;
  
  block = g_new0(Block,1);
  block->type = BLOCK_COMPOUND;
  block->ops = &compound_block_ops;
  block->d.contained = NULL;

  while (*str && **str) {
    inblk = NULL;
    switch (**str) {
    case '&':
    case '^':
    case '|':
    case '+':
    case '.':
    case '*':
    case '}':
    case '{':
      inblk = opblock_create(str);
      break;
    case '(':
      (*str)++;
      inblk = parensblock_create(compoundblock_create(str));
      break;
    case '!':
      (*str)++;
      if ((**str) == '(') {
	(*str)++;
	inblk = overlineblock_create(compoundblock_create(str));
      } else { /* single identifier "not" */
	inblk = overlineblock_create(textblock_create(str));
      }
      break;
    case ')': /* Whooops ! outta here ! */
      (*str)++;
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


/* Receptivity : */
void 
receptivity_set_value(Receptivity *rcep, const gchar *value)
{
  if (rcep->value) g_free((char *)rcep->value);
  if (rcep->rootblock) rcep->rootblock->ops->destroy(rcep->rootblock);

  rcep->value = g_strdup(value);
  rcep->rootblock = compoundblock_create(&value);
  /* a good bounding box recalc here would be nice. */
}


Receptivity *
receptivity_create(const gchar *value, Font *font, real fontheight,
		   Color *color)
{
  Receptivity *rcep;

  init_symbolfont();

  rcep = g_new0(Receptivity,1);
  rcep->font = font;
  rcep->fontheight = fontheight;
  rcep->color = *color;
  receptivity_set_value(rcep,value);

  return rcep;
}

void 
receptivity_destroy(Receptivity *rcep)
{
  if (rcep->value) g_free((char *)rcep->value);
  if (rcep->rootblock) rcep->rootblock->ops->destroy(rcep->rootblock);
  g_free(rcep);
}

extern void save_receptivity(ObjectNode *obj_node, const gchar *attrname,
			     Receptivity *rcep)
{
  save_string(obj_node,attrname,(char *)rcep->value);
}

Receptivity *
load_receptivity(ObjectNode *obj_node,
		 const gchar *attrname,
		 const gchar *defaultvalue,
		 Font *font,
		 real fontheight, Color *color)
{
  const gchar *value = NULL;
  Receptivity *rcep;

  init_symbolfont();

  rcep = receptivity_create(NULL,font,fontheight,color);
  value = load_string(obj_node,attrname,(char *)defaultvalue);
  if (value) receptivity_set_value(rcep,value);
  g_free((char *)value);
  return rcep;
}
 
void 
receptivity_draw(Receptivity *rcep, Renderer *renderer)
{
  if (rcep->rootblock) {
    rcep->rootblock->ops->draw(rcep->rootblock,rcep,renderer);
  }
}

void receptivity_calc_boundingbox(Receptivity *rcep, Rectangle *box)
{
  rcep->ascent = font_ascent(rcep->font,rcep->fontheight);
  rcep->descent = font_descent(rcep->font,rcep->fontheight);

  box->left = box->right = rcep->pos.x;
  box->top = box->bottom = rcep->pos.y;

  if (rcep->rootblock) {
    rcep->rootblock->ops->get_boundingbox(rcep->rootblock,&rcep->pos,rcep,box);
  }  

  rcep->width = box->right - box->left;
  rcep->height = box->bottom - box->top;
}


