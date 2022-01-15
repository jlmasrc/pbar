/*
  pbar: simple example advanced. Compile with
  gcc -O3 -o pbar_example_advanced pbar_example_advanced.c pbar.c -lm

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
#include "pbar.h"

/* Change this value for program duration */
#define END 2000000000l

int main(int argc, char **argv) {
  long int n;
  pbar p;
  double s;

  /* The second and third arguments are the initial and final values
     for the progress parameter (in this case, the variable n). They
     can be positive or negative, integer or floating point and the
     initial value may be greater than the final value (in case of a
     decreasing progress parameter). */
  pbar_init(&p, 1, END);

  /* The behavior can be changed after pbar_init():
     p->update = <value>: change how often the progress bar is updated
       in seconds. Default value is 0.2.
     p->output = <stream>: change the output of pbar. Default value is stderr.
       Other possibility: stdout. pbar will only produce output if <stream> is
       a terminal, so program output can be safely redirected to a file.
     p->bar_fill = <char>: change the character that fills the progress bar.
       Default value is '#'.
     p->wheel = <string>: change the wheel animation. Default value is "|/-\\",
       other possibility: ".oOo". */

  s = 0;
  for(n = 1; n <= END; n++) {
    /* Work */
    s += 1.0 / n;

    /* pbar_mark() will return nonzero every p->update seconds
       approximately. Its impact on execution time should be
       negligible. */
    if(pbar_mark(&p, n)) {
      char s1[50], s2[50];
      /* Create strings with the extra fields in the progress bar */
      snprintf(s1, 50, "%ld/%ld ", n, END);
      snprintf(s2, 50, "s = %f ", s);
      /* Format string:
	 %p: percent progress
	 %b: progress bar
	 %w: animated wheel
	 %e: elapsed time
	 %r: remaining time
	 %s: any string passed after the format string */
      pbar_show(&p, "%s %p [%b] %w (ET: %e, RT: %r) Partial sum: %s", s1, s2);
    }
  }

  printf("\nFinal sum: s = %f\n", s);

  return 0;
}
