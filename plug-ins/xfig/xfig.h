#ifndef XFIG_H
#define XFIG_H

extern Color fig_default_colors[];
extern char *fig_fonts[];

int num_fig_fonts (void);

#define FIG_MAX_DEFAULT_COLORS 32
#define FIG_MAX_USER_COLORS 512
#define FIG_MAX_DEPTHS 1000
/* 1200 PPI */
#define FIG_UNIT 472.440944881889763779527559055118
/* 1/80 inch */
#define FIG_ALT_UNIT 31.496062992125984251968503937007

#endif
