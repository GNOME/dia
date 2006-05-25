% Metapost TeX macro
% Title: Lines.dia
% Creator: Dia v0.95+cvs
% CreationDate: Mon May 15 07:49:14 2006
% For: larsrc


beginfig(1);
verbatimtex
%&latex
\documentclass{minimal}
\begin{document}
etex
  % picture(3.281138,-11.700000)(14.206506,-2.598666)
  x = 1.000000cm; y = -1.000000cm;

% set_line_color 0.000000, 0.000000, 0.000000
% set_line_color 1.000000, 1.000000, 1.000000
% set_linewidth 0.110000
% set_line_color 1.000000, 0.000000, 0.000000
  draw (3.595873x,3.106830y)--(14.150000x,3.400000y)
    withpen pencircle scaled 0.1100x
    withcolor (1.0000, 0.0000, 0.0000)
    dashed dashpattern (on 1.000000x off 1.000000x);
% set_linewidth 0.110000
% set_line_color 1.000000, 0.000000, 0.000000
  draw (3.979685x,2.867395y)--(3.472936x,3.103415y)--(3.965802x,3.367202y)
    withpen pencircle scaled 0.1100x
    withcolor (1.0000, 0.0000, 0.0000);
% set_linewidth 0.000000
% set_line_color 0.000000, 0.000000, 0.000000
  draw (9.800000x,11.700000y)--(8.811656x,3.746184y)
    withpen pencircle scaled 0.0000x;
% set_line_color 0.000000, 0.000000, 0.000000
% fill_polygon
  path p;
  p = (8.811656x,3.746184y)--(9.678950x,5.049181y)--(8.941134x,4.788170y)--(8.289635x,5.221818y)--cycle;
  fill p 
    withpen pencircle scaled 0.0000x;
% set_line_color 0.000000, 0.000000, 0.000000
  draw (8.811656x,3.746184y)--(9.678950x,5.049181y)--(8.941134x,4.788170y)--(8.289635x,5.221818y)--cycle
    withpen pencircle scaled 0.0000x;
endfig;
end;
