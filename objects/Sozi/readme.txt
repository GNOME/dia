This is a front end for the integration of the Sozi player (see [1])
to the Dia's SVG exports. Sozi is a zooming UI for presentation, a
bit like Prezi. Some examples of Sozi presentations are available on
the page [2].

This implementation supports Sozi strictly less than 14. The old
(<=13) "API" can be found at [3], but will probably be obsolete when
you will read this doc.

Additional information can be found on the Sozi web site, particularly
helps about the usage of the player [4].

The implementation splits in several files :

sozi-object : the base object that control placement of the object.
sozi-frame : the frame that represent a view of the presentation.
sozi-media : a video or audio to insert in the presentation
sozi-player : the minified javascript/css code of the Sozi player,
              generated from the Sozi source code, taken from the
              commit of the Sep 22, 2014 [5].

As an alternative to the embedded Sozi player version, you can provide
your own Sozi player with the "--with-sozi-path" configure option that
should point to a path containing the Sozi player files (sozi.version,
sozi.js, sozi.css, ...).

Some issues still need to be addressed to improve this module:
- the problem of the frame title/sequence display :
  - should we display something ?
  - what : sequence number, title, both ?
  - should it be editable in interactive rendering ?
  - placement : inside, outside ?
  - size : dynamic re-sizing ?
- the problem of the frame title/sequence management :
  - the automatic sequence numbering is screwed after frame
    deletion or undo command.
  - is the automatic sequence management the good solution ?
  - how to redraw all objects after changing sequence numbering ?
- display a preview of the media ?


[1] http://sozi.baierouge.fr/
[2] http://sozi.wikidot.com/presentations 
[3] https://web.archive.org/web/20130527164221/http://sozi.baierouge.fr/wiki/en:format
[4] http://sozi.baierouge.fr/pages/40-play.html
[5] https://github.com/senshu/Sozi/commit/e6598642ae6f06e87b689839fb0229ccf1f1130c




