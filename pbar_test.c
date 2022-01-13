/*
  pbar: example program. Compile with
  gcc -o pbar_test pbar_test.c pbar.c -lm

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
#define END 2000000000

int main(int argc, char **argv) {
  long int n;
  pbar *p;

  /* Format string:
     %a: absolute progress
     %p: percent progress
     %b: progress bar
     %w: animated wheel
     %e: elapsed time
     %r: remaining time */

  /* Full featured format string. */
  p = pbar_new(0, END - 1, "%a %p [%b] %w Elapsed: %e, Remaining: %r");

  /* The behavior can be changed after pbar_new():
     p->print_format = <string>: change the format string
     p->wheel = <string>: change the wheel animation. Default value is "|/-\\",
       other possibility: ".oOo".
     p->bar_fill = <char>: change the character that fills the progress bar.
       Default value is '#'.
     p->update_period = <value>: change how often the progress bar is updated
       in seconds. Default value is 0.2. */

  for(n = 0; n < END; n++) {
    /* Do something useful here */

    /* pbar_print() will only do something every p->update_period
       seconds. During execution, the variables p->elapsed_time,
       p->remaining_time and p->work_done can be read. */
    pbar_print(p, n);
  }

  /* This function must be called after the loop for ensuring a 100% progress
     display and free memory allocated in p. */
  pbar_close(p);

  return 0;
}
