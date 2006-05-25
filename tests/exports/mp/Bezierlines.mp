% Metapost TeX macro
% Title: Bezierlines.dia
% Creator: Dia v0.95+cvs
% CreationDate: Mon May 15 07:49:07 2006
% For: larsrc


beginfig(1);
verbatimtex
%&latex
\documentclass{minimal}
\begin{document}
etex
  % picture(5.200000,-14.650000)(18.350000,-1.703910)
  x = 1.000000cm; y = -1.000000cm;

% set_line_color 0.000000, 0.000000, 0.000000
% set_line_color 1.000000, 1.000000, 1.000000
% set_linewidth 0.100000
% set_line_color 1.000000, 0.647059, 0.000000
  draw (10.489107x,2.164374y)..controls (17.140704x,1.656266y) and (5.950000x,11.450000y)
    ..(10.650000x,8.150000y)..controls (15.350000x,4.850000y) and (14.465400x,12.250000y)
    ..(16.388197x,12.250000y)
    withpen pencircle scaled 0.1000x
    withcolor (1.0000, 0.6471, 0.0000);
% set_line_color 1.000000, 0.647059, 0.000000
% fill_polygon
  path p;
  p = (10.470065x,1.915100y)--(9.990559x,2.202457y)--(10.508148x,2.413648y)--cycle;
  fill p 
    withpen pencircle scaled 0.1000x
    withcolor (1.0000, 0.6471, 0.0000);
% set_line_color 1.000000, 0.647059, 0.000000
% fill_polygon
  path p;
  p = (18.188197x,12.250000y)--(15.788197x,13.450000y)--(16.388197x,12.250000y)--(15.788197x,11.050000y)--cycle;
  fill p 
    withpen pencircle scaled 0.1000x
    withcolor (1.0000, 0.6471, 0.0000);
% set_line_color 1.000000, 0.647059, 0.000000
  draw (18.188197x,12.250000y)--(15.788197x,13.450000y)--(16.388197x,12.250000y)--(15.788197x,11.050000y)--cycle
    withpen pencircle scaled 0.1000x
    withcolor (1.0000, 0.6471, 0.0000);
% set_linewidth 0.000000
% set_line_color 0.000000, 1.000000, 0.000000
  draw (4.700000x,4.350000y)..controls (9.348000x,4.350000y) and (11.200000x,10.650000y)
    ..(8.700000x,7.000000y)..controls (6.200000x,3.350000y) and (11.350000x,10.950000y)
    ..(11.700000x,6.900000y)..controls (12.050000x,2.850000y) and (14.052000x,9.250000y)
    ..(18.700000x,9.250000y)
    withpen pencircle scaled 0.0000x
    withcolor (0.0000, 1.0000, 0.0000)
    dashed dashpattern (on 1.000000x off 0.475000x on 0.050000x off 0.475000x);
endfig;
end;
