/*
  pbar: small C library for easy and flexible progress bar display in terminal.
  Features: see pbar.c.

  Copyright (C) 2022  Joao Luis Meloni Assirati.

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.
  
  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.
  
  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef PBAR_H
#define PBAR_H

typedef struct pbar {
  /* These can be changed after pbar_init() */
  FILE *output;
  double update;
  char bar_fill;
  char *wheel;

  /* These may be of interest and can be read after pbar_mark() */
  double progress; /* between 0 and 1 */
  double parm_rate;
  double time_elapsed;
  double time_remain;

  /* These are private data used by lib pbar */
  double parm_start_;
  double parm_end_;
  double parm_delta_;
  double parm_lupd_;
  double time_start_;
  double time_lupd_;
  double mark_;
  int wheel_count_;
  int inc_;
} pbar;

int pbar_update_(pbar *p, double parm);

void pbar_init(pbar *p, double parm_start, double parm_end);
void pbar_vshow(pbar *p, char *format, va_list ap);
void pbar_show(pbar *p, char *format, ...);

static inline int pbar_mark(pbar *p, double parm) {
  return (p->inc_ == (parm > p->mark_) || parm == p->mark_)
    && pbar_update_(p, parm);
}

static inline void pbar_mark_show(pbar *p, double parm, char *format) {
  if(pbar_mark(p, parm)) pbar_show(p, format);
}

#endif /* PBAR_H */
