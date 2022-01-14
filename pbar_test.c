/*
  pbar: example program. Compile with
  gcc -O3 -o pbar_test pbar_test.c pbar.c -lm

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
#include "pbar.h"

/* Change this value for program duration */
#define END 10000000000

int main(int argc, char **argv) {
  long int n;
  pbar *p;
  double s;

  /* Format string:
     %a: absolute progress
     %p: percent progress
     %b: progress bar
     %w: animated wheel
     %e: elapsed time
     %r: remaining time */

  /* The first and second arguments are the initial and final values
     for the progress parameter (in this case, the variable n
     below). They can be positive or negative, integer or floating
     point and the initial value may be greater than the final value
     (in case of a decreasing progress parameter).

     The third argument is the format string (here, a full featured
     one). */
  p = pbar_new(1, END, "%a %p [%b] %w ET: %e, RT: %r");

  /* The behavior can be changed after pbar_new():
     p->print_format = <string>: change the format string
     p->wheel = <string>: change the wheel animation. Default value is "|/-\\",
       other possibility: ".oOo".
     p->bar_fill = <char>: change the character that fills the progress bar.
       Default value is '#'.
     p->update_period = <value>: change how often the progress bar is updated
       in seconds. Default value is 0.2.
     p->output = <stream>: change the output of pbar. Default value is stderr.
       Other possibility: stdout. pbar will only produce output if <stream> is
       a terminal, so program output can be safely redirected to a file. */

  s = 0;
  for(n = 1; n <= END; n++) {
    /* Real work */
    s += n;

    /* pbar_print() will only do something every p->update_period
       seconds. During execution, the variables p->elapsed_time,
       p->remaining_time and p->work_done can be read.

       pbar_print() will impact only around 15% more execution time in
       this very thin workload per cycle (s += n) with gcc with -O3 in
       amd64. For more intense workloads per cycle, the impact is
       negligible (try s += 1.0 / n instead of s += n). */
    pbar_print(p, n);
  }

  /* This function must be called after the loop for ensuring a 100% progress
     display and free memory allocated in p. */
  pbar_close(p);

  printf("\ns = %.0f\n", s);

  return 0;
}
