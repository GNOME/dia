/* Dia -- an diagram creation/manipulation program
 * Copyright (C) 1998 Alexander Larsson
 *
 * 
 * W   W   A   RRR  N   N I N   N  GGG
 * W   W  A A  R  R NN  N I NN  N G
 * W   W AAAAA RRR  N N N I N N N G  GG
 * W W W A   A R R  N  NN I N  NN G   G
 *  W W  A   A R  R N   N I N   N  GGG
 *
 *
 * THIS FILE IS ENCODED IN UTF-8. 
 *
 * IF YOUR EDITOR DOES NOT SUPPORT FULLY UTF-8 INCLUDING CHARACTERS IT DOES
 * NOT KNOW HOW TO DISPLAY, GET THE HELL OUT OF THIS FILE ** NOW **. 
 *
 * (well, if you plan to generate a patch or make a CVS commit)
 *
 *
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
 
/* Wouldn't it be neat to display (where applicable) the names
   of the authors in their original (non-Roman) scripts, where
   applicable ?  -- cc */

/* All names below should be UTF-8 encoded. The consensus is that
   it's the responsibility of the user to ensure their terminal
   can display this if necessary.

   The way the following list is done is a bit messy, but it
   works. All the people are 'authors'. The first
   NUMBER_OF_ORIG_AUTHORS are original authors. The next
   NUMBER_OF_MAINTAINERS are _current_ maintainers. The rest
   are just 'authors'. Documentors are listed in a separate
   array below. 

   --- Andrew Ferrier
*/

static const int NUMBER_OF_ORIG_AUTHORS = 1;
static const int NUMBER_OF_MAINTAINERS = 1;

static const gchar *authors[] =	{

    /* original author(s) */

    "Alexander Larsson <alexl@redhat.com>",

    /* current maintainer */
    "Zander Brown <zbrown@gnome.org>",

    /* "out for lunch" maintainer */
    "Hans Breuer <hans@breuer.org>",

    /* other author(s) */
    "Eduard Nicodei <eddnicodei@gmail.com>",
    "Lars R. Clausen <lrclause@cs.uiuc.edu>",
    "Cyrille Chépélov <cyrille@chepelov.org>",
    "James Henstridge <james@daa.com.au>",
    "Jerome Abela <abela@solsoft.fr>",
    "Emmanuel Briot <briot@volga.gnat.com>",
    "Fredrik Hallenberg <hallon@debian.org>",
    "Sameer D. Sahasrabuddhe <sameerds@gmail.com>",
    "Patrick Hallinan <patrick.j.hallinan@gmail.com>",
    "Luc Cessieux <cessieux@hotmail.com>",
    "Francis J. Lacoste <francis@contre.com>",
    "Steffen Macke <sdteffen@gmail.com>",
    "Jacek Pliszka <Jacek@Pliszka.fuw.edu.pl>",
    "Henk Jan Priester <hj@justcroft.com>",
    "Alejandro Aguilar Sierra <asierra@servidor.unam.mx>",
    "Hubert Figuière <hfiguiere@teaser.fr>",
    "Alexey Novodvorsky <aen@logic.ru>",
    "Patric Sung <phsung@ualberta.ca>",
    "Robert Young <robert@young.dsto.defence.gov.au>",
    "田郷 明 (Akira TAGOH) <tagoh@redhat.com>",
    "Richard Rowell <rwrowell@bellsouth.net>",
    "Frank Gevaerts <frank.gevaerts@fks.be>",
    "M. C. Nelson <mcn@kobayashimaru.org>",
    "Matthieu Sozeau <mattam@netcourrier.com>",
    "Xing Wang <quixon@gnuchina.org>",
    "Andrew Ferrier <andrew@new-destiny.co.uk>",
    "Angus Ainslie <angusa@deltatee.com>",
    "Alan Horkan <horkana@tcd.ie>",
    "Martin Hans <opfinder@gmx.net>",
    "Vadim Berezniker <vadim@berezniker.com>",
    "Krzysztof Foltman <kfoltman@onet.pl>",
    "W. Borgert <debacle@debian.org>",
    "Luc Pionchon <luc@handhelds.org>",
    "Maurizio Tidei <tidei@compal.de>",
    "Sampo Kellomaki <sampo@iki.fi>",
    "Grégoire Dooms <dooms@info.ucl.ac.be>",
    "Moritz Kirmse <momomaniac@gmail.com>",
    "Robert Staudinger <robert.staudinger@gmail.com>",
    "Caolan McNamara <caolanm@redhat.com>",
    "Josef Moellers <josef.moellers@gmx.de>",
    "Jason Childs <oblivian@users.sourceforge.net>",
    "Frank Endres <frank.endres@ac-nantes.fr>",
    "Steven Garcia <webwhammy@gmail.com>",
    NULL
};

static const gchar *documentors[] =	{
    "Henry House <hajhouse@houseag.com>",
    "Judith Samson <judith@samsonsource.com>",
    "Kevin Breit <battery841@mypad.com>",
    "Alan Horkan <horkana@tcd.ie>",
    NULL
};
