/*
  pbar: small C library for easy and flexible progress bar display in terminal.
  Features:
  * very low processor usage
  * progress bar
  * percent progress
  * wheel animation
  * elapsed time
  * remaining time
  * user defined text and variables in the progress line
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
#include <string.h>
#include <math.h>

#include <sys/ioctl.h>
#include <unistd.h>
#include <time.h>

#include <stdarg.h>

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


/** Print functions ***********************************************************/

static void bar(pbar *p, int cols_taken) {
  int i,
    /* We subtract 1 from the available columns to avoid a line break
       when the full terminal line is used */
    c = termwidth(fileno(p->output)) - cols_taken - 1,
    n = c * p->progress;

  /* In case of over 100%, just print a full bar */
  if(n > c) n = c;

  for(i = 0; i < n; i++) fputc(p->bar_fill, p->output);
  for(i = n; i < c; i++) fputc(' ', p->output);
}

static int wheel(pbar *p) {
  int w;
  if((w = p->wheel[++p->wheel_count_]) != '\0') return w;
  return p->wheel[p->wheel_count_ = 0];
}

typedef struct fields {
  char percent[MAXFIELD], elapsed[MAXFIELD], remain[MAXFIELD];
  int cols_taken;
} fields;

static void fill_fields(pbar *p, fields *t, char *format, va_list ap) {
  char *f;
  va_list ap2;
  
  va_copy(ap2, ap);
  t->cols_taken = 0;
  for(f = format; *f; f++) {
    if(*f == '%') {
      f++;
      if(*f == '\0') break;
      switch(*f) {
      case 'p':
	t->cols_taken += sprn(t->percent, "%3.0f%%", 100 * p->progress);
	break;
      case 'e':
	t->cols_taken += timestr(t->elapsed, p->time_elapsed);
	break;
      case 'r':
	t->cols_taken += timestr(t->remain, p->time_remain);
	break;
      case 'w':
	t->cols_taken += 1;
	break;
      case 's':
	t->cols_taken += strlen(va_arg(ap2, char *));
	break;
      case 'b':
	break;
      default:
	t->cols_taken++;
      }
    } else {
      t->cols_taken++;
    }
  }
  va_end(ap2);
}

static void print_fields(pbar *p, fields *t, char *format, va_list ap) {
  char *f;
  va_list ap2;
  
  va_copy(ap2, ap);
  for(f = format; *f; f++) {
    if(*f == '%') {
      f++;
      if(*f == '\0') break;
      switch(*f) {
      case 'p':
	fputs(t->percent, p->output);
	break;
      case 'e':
	fputs(t->elapsed, p->output);
	break;
      case 'r':
	fputs(t->remain, p->output);
	break;
      case 'w':
	fputc(wheel(p), p->output);
	break;
      case 's':
	fputs(va_arg(ap2, char *), p->output);
	break;
      case 'b':
	bar(p, t->cols_taken);
	break;
      default:
	fputc(*f, p->output);
      }
    } else {
      fputc(*f, p->output);
    }
  }
  va_end(ap2);
}

/*********************************************************** Print functions **/


/** Public functions **********************************************************/

void pbar_init(pbar *p, double parm_start, double parm_end) {
  /* These are members of struct pbar and can be modified after
     pbar_new(). They are not modified by the pbar library after
     pbar_new(). */
  p->update = 0.2;
  p->wheel = "|/-\\"; /* Other possibilities: ".oOo", "+x" */
  p->bar_fill = '#';
  p->output = stderr;

  /* These members of struct pbar should not be modified. They are
     used by inline functions in pbar.h */
  p->mark_ = parm_start;
  p->inc_ = parm_end > parm_start;

  /* The other members of struct pbar have useful data that can be
     read by the user but should not be modified. They change across
     calls to pbar_update_() and pbar_print_() */

  /* These are private members not available outside the pbar library */
  p->parm_start_ = parm_start;
  p->parm_end_ = parm_end;
  p->parm_delta_ = p->parm_end_ - p->parm_start_;
  p->time_start_ = gettimef();
  p->wheel_count_ = 0;
  p->parm_lupd_ = parm_start;
  p->time_lupd_ = p->time_start_;

  fputs("\033[s", p->output);
}

int pbar_update_(pbar *p, double parm) {
  double now = gettimef();
  double dp = parm - p->parm_lupd_;
  double dt = now - p->time_lupd_;
  double oldmark = p->mark_;

  p->parm_rate = dp / dt;
  p->time_elapsed = now - p->time_start_;
  p->time_remain = (p->parm_end_ - parm) / p->parm_rate;

  if(parm == p->parm_end_) {
    p->progress = 1;
  } else {
    /* fabs() is needed because if p->parm_end_ - p->parm_start_ < 0,
       then 0 / p->parm_delta_ == -0 */
    p->progress = fabs((parm - p->parm_start_) / p->parm_delta_);
  }

  /* Calculation of the next mark after p->update */
  p->mark_ = parm + p->update * p->parm_rate;

  /* Ensure that p->parm_end_ will be a mark */
  if((p->inc_ && oldmark < p->parm_end_ && p->mark_ > p->parm_end_)
     || (!p->inc_ && oldmark > p->parm_end_ && p->mark_ < p->parm_end_)) {
    p->mark_ = p->parm_end_;
  }

  p->parm_lupd_ = parm;
  p->time_lupd_ = now;

  /* If dt is too discrepant from p->update, reject this
     iteration. This will happen in the first 2 or 3 calls to
     pbar_update(). */
  if(fabs(dt/p->update - 1) > 0.5 && p->progress != 1) return 0;

  if(!isatty(fileno(p->output))) return 0;
  fputc('\r', p->output);
  return 1;
}

void pbar_vshow(pbar *p, char *format, va_list ap) {
  fields t;
  fill_fields(p, &t, format, ap);
  print_fields(p, &t, format, ap);
  /* DEBUG
     fputc('\n', p->output); */
  fflush(p->output);
}
  
void pbar_show(pbar *p, char *format, ...) {
  va_list ap;
  va_start(ap, format);
  pbar_vshow(p, format, ap);
  va_end(ap);
}

/********************************************************** Public functions **/
