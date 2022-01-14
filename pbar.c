/*
  pbar: small C library for easy and flexible progress bar display in terminal.
  Features:
  * very low processor usage
  * progress bar
  * percent progress
  * absolute progress
  * wheel animation
  * elapsed time
  * remaining time
  * terminal width autodetection

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

static int termwidth(int fd) {
  struct winsize w;
  ioctl(fd, TIOCGWINSZ, &w);
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
  double work_lastupdate;
  double time_start;
  double time_lastupdate;
  double time_usage;
  int wheel_counter;
} state;

static int update(state *st, double work, double now) {
  pbar *p = &(st->public);
  double delta = st->work_end - st->work_start;
  double dt = now - st->time_lastupdate;
  
  p->elapsed_time = now - st->time_start;
  p->remaining_time = (st->work_end - work) * dt / (work - st->work_lastupdate);

  if((delta > 0 && work >= st->work_end)||(delta < 0 && work <= st->work_end)) {
    p->work_done = 1;
  } else {
    /* fabs() is needed because if delta < 0, then 0 / delta == -0 */
    p->work_done = fabs((work - st->work_start) / delta);
  }
  
  p->work_mark = work
    + p->update_period * (p->work_mark - st->work_start) / p->elapsed_time;

  p->pbar_load = 100 * st->time_usage / p->elapsed_time;

  st->work_lastupdate = work;
  st->time_lastupdate = now;

  /* If dt is too discrepant from p->update_period, reject this
     iteration. This will happen in the first 2 or 3 calls to
     update() */
  return fabs((1 - dt/p->update_period)) < 0.5;
}

/********************************************************************* State **/


/** Terminal printing *********************************************************/

static void print_bar(state *st, int cols_taken) {
  int i,
    c = termwidth(fileno(st->public.output)) - cols_taken - 1,
    n = c * st->public.work_done;

  for(i = 0; i < n; i++) fputc(st->public.bar_fill, st->public.output);
  for(i = n; i < c; i++) fputc(' ', st->public.output);
}

static int wheel(state *st) {
  int w;
  if((w = st->public.wheel[++st->wheel_counter]) != '\0') return w;
  return st->public.wheel[st->wheel_counter = 0];
}

static void print(state *st, double work) {
  char *f;
  int cols_taken;
  field str_absolute, str_percent, str_elapsed, str_remain, str_load;

  /* First format pass: fill fields and calculate the number of columns */
  cols_taken = 0;
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
	break;
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
  for(f = st->public.print_format; *f; f++) {
    if(*f == '%') {
      f++;
      if(*f == '\0') break;
      switch(*f) {
      case 'a':
	fputs(str_absolute, st->public.output);
	break;
      case 'p':
	fputs(str_percent, st->public.output);
	break;
      case 'e':
	fputs(str_elapsed, st->public.output);
	break;
      case 'r':
	fputs(str_remain, st->public.output);
	break;
      case 'w':
	fputc(wheel(st), st->public.output);
	break;
      case 'L':
	fputs(str_load, st->public.output);
	break;
      case 'b':
	print_bar(st, cols_taken);
	break;
      default:
	fputc(*f, st->public.output);
      }
    } else {
      fputc(*f, st->public.output);
    }
  }
}

/********************************************************* Terminal printing **/


/** Public functions **********************************************************/

pbar *pbar_new(double work_start, double work_end, char *print_format) {
  state *st = malloc(sizeof(*st));

  /* These are members of struct pbar and can be modified after
     pbar_new(). They are not modified by the pbar library after
     pbar_new(). */
  st->public.print_format = print_format;
  st->public.update_period = 0.2;
  st->public.wheel = "|/-\\"; /* Other possibilities: ".oOo", "+x" */
  st->public.bar_fill = '#';
  st->public.output = stderr;

  /* These members of struct pbar should not be modified. They are
     used by inline functions in pbar.h */
  st->public.work_mark = work_start;
  st->public.increasing = work_end > work_start;

  /* The other members of struct pbar have useful data that can be
     read by the user but should not be modified. They change across
     calls to pbar_update_() and pbar_print_() */

  /* These are private members not available outside the pbar library */
  st->work_start = work_start;
  st->work_end = work_end;
  st->time_start = gettimef();
  st->time_usage = 0;
  st->wheel_counter = 0;
  st->work_lastupdate = st->work_start;
  st->time_lastupdate = st->time_start;
  
  return &(st->public);
}

void pbar_close(pbar *p) {
  double now = gettimef();
  state *st = (state *)p;
  update(st, st->work_end, now);
  if(isatty(fileno(p->output))) {
    fputc('\r', st->public.output);
    print(st, st->work_end);
    fputc('\n', st->public.output);
    fflush(st->public.output);
  }
  free(st);
  st->time_usage += gettimef() - now;
}

int pbar_update_(pbar *p, double work) {
  double now = gettimef();
  state *st = (state *)p;
  int r = update(st, work, now);
  st->time_usage += gettimef() - now;
  return r;
}

int pbar_print_(pbar *p, double work) {
  double now = gettimef();
  state *st = (state *)p;
  int r = update(st, work, now);
  if(r && isatty(fileno(p->output))) {
    fputc('\r', st->public.output);
    print((state *)p, work);
    fflush(st->public.output);
  }
  st->time_usage += gettimef() - now;
  return r;
}

/********************************************************** Public functions **/
