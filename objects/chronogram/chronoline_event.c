/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * Chronogram objects support
 * Copyright (C) 2000 Cyrille Chepelov <chepelov@calixo.net>
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

#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "chronoline_event.h"
#include "message.h"

#define DUMP_PARSED_CLELIST 0

inline static CLEvent *
new_cle(CLEventType type, real time)
{
  CLEvent *cle;
  cle = g_new0(CLEvent,1);
  cle->type = type;
  cle->time = time;
  cle->x = 0.0;
  return cle;
}


static gint
compare_cle (gconstpointer a, gconstpointer b)
{
  CLEvent *ca = (CLEvent *)a;
  CLEvent *cb = (CLEvent *)b;

  g_return_val_if_fail (ca, 1);
  g_return_val_if_fail (cb, 1);

  if (ca->time == cb->time) return 0;
  if (ca->time < cb->time) return -1;
  /* else */ return 1;
}


void
destroy_cle (gpointer data, gpointer user_data)
{
  CLEvent *cle = (CLEvent *) data;
  g_clear_pointer (&cle, g_free);
}


void
destroy_clevent_list(CLEventList *clel)
{
  g_slist_foreach(clel,destroy_cle,NULL);
  g_slist_free(clel);
}

#if DUMP_PARSED_CLELIST
#include <stdio.h>

static void dump_parsed_clelist(CLEventList *clel)
{
  int i = 0;
  const char *s = NULL;

  printf("ChronoLine Event List dump follows:\n");
  while (clel) {
    CLEvent *evt = (CLEvent *)(clel->data);
    switch (evt->type) {
    case CLE_OFF: s = "off"; break;
    case CLE_ON: s = "on"; break;
    case CLE_UNKNOWN: s = "unknown"; break;
    case CLE_START: s = "start"; break;
    default:
      g_assert_not_reached();
    }
    printf("%3d  t=%7.3f %s\n",i++,evt->time,s);

    clel = g_slist_next(clel);
  }
  printf("ChronoLine Event List dump finished.\n");
}
#endif

#define CHEAT_CST (1E-7)


static void
add_event (CLEventList **clel,
           real         *t,
           real         *dt,
           CLEventType  *oet,
           CLEventType  *et,
           real          rise,
           real          fall)
{
  if (*et == CLE_START) {
    *t = *dt; *dt = 0.0;
    /* special case */
    return;
  } else {
    while (*oet != *et) {
      *clel = g_slist_insert_sorted (*clel, new_cle (*oet, *t), compare_cle);
      switch (*oet) {
        case CLE_UNKNOWN:
          *t += fall;
          *dt -= CHEAT_CST;
          *oet = CLE_OFF;
          break;
        case CLE_OFF:
          *t += rise;
          *dt -= CHEAT_CST;
          *oet = *et;
          break;
        case CLE_ON:
          *t += fall;
          *dt -= CHEAT_CST;
          *oet = CLE_OFF;
          break;
        case CLE_START:
          break;
        default:
          g_assert_not_reached();
      }
    }

    /* insert (at last) the desired state. */
    *clel = g_slist_insert_sorted (*clel, new_cle (*et, *t), compare_cle);
    *t += *dt;
    *dt = 0.0;
    *oet = *et;
  }
}


static CLEventList *
parse_clevent (const gchar *events, real rise, real fall)
{
  real t = -1E10;
  double dt;
  const gchar *p, *p1, *np;
  gunichar uc;
  CLEventType et = CLE_UNKNOWN;
  CLEventType oet = CLE_UNKNOWN;
  CLEventList *clel = NULL;
  enum { EVENT, LENGTH } waitfor = EVENT;

  p1 = p = events;

  /* if we don't cheat like this, g_slist_insert_sorted won't insert the
     right way. */
  if (rise <= 0.0) {
    rise = 0.0;
  }
  rise += CHEAT_CST;
  if (fall <= 0.0) {
    fall = 0.0;
  }
  fall += CHEAT_CST;

  while (*p) {
    uc = g_utf8_get_char (p);
    np = g_utf8_next_char (p);
    switch (uc) { /* skip spaces */
      case ' ':
      case '\t':
      case '\n':
        p = np; continue;
      default:
        break;
    }
    if (waitfor == EVENT) {
      switch (uc) {
        case 'u':
        case 'U':
          et = CLE_UNKNOWN;
          waitfor = LENGTH;
          p = np;
          break;
        case '@':
          et = CLE_START;
          waitfor = LENGTH;
          p = np;
          break;
        case '(':
          et = CLE_ON;
          waitfor = LENGTH;
          p = np;
          break;
        case ')':
          et = CLE_OFF;
          waitfor = LENGTH;
          p = np;
          break;
        default:
          message_warning ("Syntax error in event string; waiting one of "
                           "\"()@u\". string=%s",p);
          return clel;
      }
    } else { /* waitfor == LENGTH */
      dt = g_ascii_strtod (p, (char **) &p1);
      if (p1 == p) {
        /* We were ready for a length argument, we got nothing.
          Maybe the user entered a zero-length argument ? */
        switch (uc) {
          case 'u':
          case 'U':
          case '@':
          case '(':
          case ')':
            dt = 0.0;
            break;
          default:
            message_warning ("Syntax error in event string; waiting a floating "
                             "point value. string=%s", p);
            return clel;
        }
      }
      /* dt contains a duration value.
         p1 points to the rest of the string. */
      p = p1;

      add_event (&clel, &t, &dt, &oet, &et, rise, fall);
      waitfor = EVENT;
    }
  }

  if (waitfor == LENGTH) {
    /* late fix */
    if (oet == CLE_START) {
      oet = et;
    }
    dt = 0.0;
    add_event (&clel, &t, &dt, &oet, &et, rise, fall);
  }

  #if DUMP_PARSED_CLELIST
  dump_parsed_clelist(clel);
  #endif

  return clel;
}


inline static int
__forward_checksum_i (int chk, int value)
{
  return (0xFFFFFFFF & (((chk << 1) | ((chk & 0x80000000)?1:0)) ^ value));
}


inline static int
__forward_checksum_r (int chk, real value)
{
  int ival = (int)value;
  return __forward_checksum_i (chk,ival);
}


static int
__chksum (const char *str, real rise, real fall, real time_end)
{
  const char *p = str;
  int i = 1;

  i = __forward_checksum_r (i, rise);
  i = __forward_checksum_r (i, fall);
  i = __forward_checksum_r (i, time_end);

  if (!p) return i;

  while (*p) {
    i = __forward_checksum_i (i,*p);
    p++;
  }
  /* printf("chksum[%s] = %d\n",str,i); */
  return i;
}


void
reparse_clevent(const gchar *events, CLEventList **lst,
		int *chksum,real rise, real fall,real time_end)
{
  int newsum;
  gchar *ps;

  /* XXX: it might be better to simply drop this checksumming ? */
  newsum = __chksum(events,rise,fall,time_end);
  if ((newsum == *chksum) && (*lst)) return;

  /* the string might contain ',' as a decimal separtor, fix it on the fly */
  if (events && strchr(events, ',') != NULL) {
    gchar *p;

    ps = g_strdup(events);
    for (p = ps; *p != '\0'; ++p) {
      if (*p == ',')
        *p = '.';
    }
  } else {
    /* must not be freed below */
    ps = (gchar *)events;
  }
  destroy_clevent_list(*lst);
  *lst = parse_clevent(ps,rise,fall);
  if (ps != events)
    g_clear_pointer (&ps, g_free);
  *chksum = newsum;
}







