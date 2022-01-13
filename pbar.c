/*
  pbar: small C library for easy and flexible progress bar display in terminal.
  Features: progress bar, percent progress, absolute progress, wheel animation,
  elapsed time, remaining time.

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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>

#include "pbar.h"

/** Support functions *********************************************************/

static double gettimef(void) {
  struct timespec t;
  clock_gettime(CLOCK_MONOTONIC, &t);
  return t.tv_sec + 1E-9 * t.tv_nsec;
}

static int termwidth(void) {
  struct winsize w;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
  return w.ws_col;
}

#define MAXFIELD 100
typedef char field[MAXFIELD];

#define sprn(s, ...) snprintf(s, MAXFIELD, __VA_ARGS__)

static int timestr(char *s, time_t t) {
  int r;
  if(t < 60) return sprn(s, "%ds", (int) t);
  r = t % 60;
  t /= 60;
  if(t < 60) return sprn(s, "%dm%ds", (int) t, r);
  r = t % 60;
  t /= 60;
  if(t < 60) return sprn(s, "%dh%dm", (int) t, r);
  r = t % 24;
  t /= 24;
  return sprn(s, "%dd%dh", (int) t, r);
}

/********************************************************* Support functions **/


/** State *********************************************************************/

/* (pbar *) and (state *) can be casted one into another */
typedef struct state {
  pbar public;

  double work_start;
  double work_end;
  double work_mark;
  double work_lastupdate;
  double time_start;
  double time_lastupdate;
  double time_usage;
  int wheel_counter;

  int (*print)(struct state *st, double work);
  int (*update)(struct state *st, double work);
} state;

static int update(state *st, double work) {
  pbar *p = &(st->public);
  double delta = st->work_end - st->work_start;
  double now = gettimef();
  double dt = now - st->time_lastupdate;
  
  p->elapsed_time = now - st->time_start;
  p->remaining_time = (st->work_end - work) * dt / (work - st->work_lastupdate);

  if((delta > 0 && work >= st->work_end)||(delta < 0 && work <= st->work_end)) {
    p->work_done = 1;
  } else {
    /* fabs() is needed because if delta < 0, then 0 / delta == -0 */
    p->work_done = fabs((work - st->work_start) / delta);
  }
  
  st->time_lastupdate = now;
  st->work_lastupdate = work;

  st->work_mark = work
    + p->update_period * (st->work_mark - st->work_start) / p->elapsed_time;

  p->pbar_load = 100 * st->time_usage / p->elapsed_time;

  st->time_usage += gettimef() - st->time_lastupdate;

  /* If dt is too discrepant from p->update_period, reject this iteration. */
  return fabs((1 - dt/p->update_period)) < 0.5;
}

static int inc_update(state *st, double work) {
  return work >= st->work_mark && update(st, work);
}

static int dec_update(state *st, double work) {
  return work <= st->work_mark && update(st, work);
}

/********************************************************************* State **/


/** Terminal printing *********************************************************/

static void print_bar(state *st, int cols_taken) {
  int i,
    c = termwidth() - cols_taken - 1,
    n = c * st->public.work_done;

  for(i = 0; i < n; i++) fputc(st->public.bar_fill, stdout);
  for(i = n; i < c; i++) fputc(' ', stdout);
}

static int wheel(state *st) {
  int w;
  if((w = st->public.wheel[++st->wheel_counter]) != '\0') return w;
  return st->public.wheel[st->wheel_counter = 0];
}

static void print(state *st, double work) {
  char *f;
  int cols_taken = 0;
  field str_absolute, str_percent, str_elapsed, str_remain, str_load;

  /* First format pass: fill fields and calculate the number of columns */
  for(f = st->public.print_format; *f; f++) {
    if(*f == '%') {
      f++;
      if(*f == '\0') break;
      switch(*f) {
      case 'a':
	cols_taken += sprn(str_absolute, "%.0f/%.0f", work, st->work_end);
	break;
      case 'p':
	cols_taken += sprn(str_percent, "%3.0f%%", 100 * st->public.work_done);
	break;
      case 'e':
	cols_taken += timestr(str_elapsed, st->public.elapsed_time);
	break;
      case 'r':
	cols_taken += timestr(str_remain, st->public.remaining_time);
	break;
      case 'w':
	cols_taken += 1;
      case 'L':
	cols_taken += sprn(str_load, "%.2f%%", st->public.pbar_load);
	break;
      case 'b':
	break;
      default:
	cols_taken++;
      }
    } else {
      cols_taken++;
    }
  }

  /* Second format pass: print */
  fputc('\r', stdout);
  for(f = st->public.print_format; *f; f++) {
    if(*f == '%') {
      f++;
      if(*f == '\0') break;
      switch(*f) {
      case 'a':
	fputs(str_absolute, stdout);
	break;
      case 'p':
	fputs(str_percent, stdout);
	break;
      case 'e':
	fputs(str_elapsed, stdout);
	break;
      case 'r':
	fputs(str_remain, stdout);
	break;
      case 'w':
	fputc(wheel(st), stdout);
	break;
      case 'L':
	fputs(str_load, stdout);
	break;
      case 'b':
	print_bar(st, cols_taken);
	break;
      default:
	fputc(*f, stdout);
      }
    } else {
      fputc(*f, stdout);
    }
  }
  fflush(stdout);

  /* Recalculate st->time_usage (calculated before in pbar_update) */
  st->time_usage += gettimef() - st->time_lastupdate;
}

static int inc_print(state *st, double work) {
  if(work < st->work_mark || !update(st, work)) return 0;
  print(st, work);
  return 1;
}

static int dec_print(state *st, double work) {
  if(work > st->work_mark || !update(st, work)) return 0;
  print(st, work);
  return 1;
}

/********************************************************* Terminal printing **/


/** Public functions **********************************************************/

pbar *pbar_new(double work_start, double work_end, char *print_format) {
  state *st = malloc(sizeof(*st));

  st->work_start = work_start;
  st->work_end = work_end;
  st->work_mark = work_start;
  st->time_start = gettimef();
  st->time_usage = 0;
  st->wheel_counter = 0;

  st->work_lastupdate = st->work_start;
  st->time_lastupdate = st->time_start;

  st->public.print_format = print_format;
  st->public.update_period = 0.2;
  /* Other possibilities: ".oOo", "+x" */
  st->public.wheel = "|/-\\";
  st->public.bar_fill = '#';

  st->print = work_end > work_start ? inc_print : dec_print;
  st->update = work_end > work_start ? inc_update : dec_update;

  return &(st->public);
}

void pbar_close(pbar *p) {
  state *st = (state *)p;
  update(st, st->work_end);
  print(st, st->work_end);
  fputc('\n', stdout);
  fflush(stdout);
  free(st);
}

int pbar_update(pbar *p, double work) {
  return ((state *)p)->update((state *)p, work);
}

int pbar_print(pbar *p, double work) {
  return ((state *)p)->print((state *)p, work);
}

/********************************************************** Public functions **/
