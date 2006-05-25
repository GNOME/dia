% Metapost TeX macro
% Title: Zigzaglines.dia
% Creator: Dia v0.95+cvs
% CreationDate: Mon May 15 07:49:23 2006
% For: larsrc


beginfig(1);
verbatimtex
%&latex
\documentclass{minimal}
\begin{document}
etex
  % picture(3.350000,-12.400000)(17.900000,-3.835000)
  x = 1.000000cm; y = -1.000000cm;

% set_line_color 0.000000, 0.000000, 0.000000
% set_line_color 1.000000, 1.000000, 1.000000
% set_linewidth 0.230000
%metapost_arc
% center->x = 3.980055% center->y = 4.819945% width = 0.260111% height = 0.260111% angle1 = 180.000000% angle2 = 270.000000% set_line_color 1.000000, 0.647059, 0.000000
  draw (3.850000x,4.819945y)..(3.888092x,4.911908y)..(3.980055x,4.950000y)
    withpen pencircle scaled 0.2300x
    withcolor (1.0000, 0.6471, 0.0000)
    dashed dashpattern (on 1.000000x off 1.000000x);
% set_line_color 1.000000, 0.647059, 0.000000
  draw (3.850000x,4.582148y)--(3.850000x,4.819945y)
    withpen pencircle scaled 0.2300x
    withcolor (1.0000, 0.6471, 0.0000)
    dashed dashpattern (on 1.000000x off 1.000000x);
%metapost_arc
% center->x = 13.350000% center->y = 5.550000% width = 1.200000% height = 1.200000% angle1 = 0.000000% angle2 = 90.000000% set_line_color 1.000000, 0.647059, 0.000000
  draw (13.950000x,5.550000y)..(13.774264x,5.125736y)..(13.350000x,4.950000y)
    withpen pencircle scaled 0.2300x
    withcolor (1.0000, 0.6471, 0.0000)
    dashed dashpattern (on 1.000000x off 1.000000x);
% set_line_color 1.000000, 0.647059, 0.000000
  draw (3.980055x,4.950000y)--(13.350000x,4.950000y)
    withpen pencircle scaled 0.2300x
    withcolor (1.0000, 0.6471, 0.0000)
    dashed dashpattern (on 1.000000x off 1.000000x);
% set_line_color 1.000000, 0.647059, 0.000000
  draw (13.950000x,5.550000y)--(13.950000x,11.750000y)
    withpen pencircle scaled 0.2300x
    withcolor (1.0000, 0.6471, 0.0000)
    dashed dashpattern (on 1.000000x off 1.000000x);
% set_line_color 1.000000, 0.647059, 0.000000
% fill_polygon
  path p;
  p = (3.850000x,4.207148y)--(4.100000x,4.707148y)--(3.850000x,4.582148y)--(3.600000x,4.707148y)--cycle;
  fill p 
    withpen pencircle scaled 0.2300x
    withcolor (1.0000, 0.6471, 0.0000);
% set_line_color 1.000000, 0.647059, 0.000000
  draw (3.850000x,4.207148y)--(4.100000x,4.707148y)--(3.850000x,4.582148y)--(3.600000x,4.707148y)--cycle
    withpen pencircle scaled 0.2300x
    withcolor (1.0000, 0.6471, 0.0000);
% set_linewidth 0.000000
% set_line_color 0.000000, 0.000000, 0.000000
  draw (7.900000x,7.150000y)--(7.900000x,9.775000y)--(16.900000x,9.775000y)--(16.900000x,12.400000y)
    withpen pencircle scaled 0.0000x;
% set_linewidth 0.000000
% set_line_color 0.000000, 0.000000, 0.000000
  draw (8.150000x,7.650000y)--(7.900000x,7.150000y)--(7.650000x,7.650000y)
    withpen pencircle scaled 0.0000x;
% set_line_color 0.000000, 0.000000, 0.000000
% fill_polygon
  path p;
  p = (16.400000x,11.400000y)--(16.900000x,12.400000y)--(17.400000x,11.400000y)--cycle;
  fill p 
    withpen pencircle scaled 0.0000x;
endfig;
end;
