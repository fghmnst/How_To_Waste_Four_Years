CC = gcc
CFLAGS = -Wall -Wextra -std=c17 -g

note_stats: note_stats.o
	$(CC) $(CFLAGS) note_stats.o -o note_stats

note_stats.o: note_stats.c
	$(CC) $(CFLAGS) -c note_stats.c -o note_stats.o

.PHONY: clean

clean:
	rm -f *.o note_stats