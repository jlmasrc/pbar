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

#include <stdio.h>

typedef struct pbar {
  char *print_format;
  char *wheel;
  char bar_fill;
  FILE *output;
  double update_period;
  double elapsed_time;
  double remaining_time;
  double work_done;
  double pbar_load;
  double work_mark;
  int increasing;
} pbar;

pbar *pbar_new(double work_start, double work_end, char *format);
void pbar_close(pbar *p);

int pbar_update_(pbar *p, double work);
int pbar_print_(pbar *p, double work);

static inline int pbar_update(pbar *p, double work) {
  return (p->increasing == (work >= p->work_mark)) && pbar_update_(p, work);
}

static inline int pbar_print(pbar *p, double work) {
  return (p->increasing == (work >= p->work_mark)) && pbar_print_(p, work);
}

#endif /* PBAR_H */
