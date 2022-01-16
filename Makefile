all: pbar_example_simple pbar_example_advanced

pbar_example_simple: pbar_example_simple.c pbar.o
	gcc -Wall -O3 -o pbar_example_simple pbar_example_simple.c pbar.o -lm

pbar_example_advanced: pbar_example_advanced.c pbar.o
	gcc -Wall -O3 -o pbar_example_advanced pbar_example_advanced.c pbar.o -lm

pbar.o: pbar.c pbar.h
	gcc -Wall -O3 -c pbar.c

clean:
	rm -f pbar_example_simple pbar_example_advanced pbar.o *~

.PHONY: all
