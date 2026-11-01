CC ?= cc
CFLAGS ?= -O2 -Wall -Wextra -std=c99 -Iupstream
LDFLAGS ?= -lm

UPSTREAM = upstream/reciter.c upstream/sam.c upstream/render.c upstream/debug.c
OBJS = sam.o $(UPSTREAM:.c=.o)

sam: $(OBJS)
	$(CC) $(CFLAGS) -o sam $(OBJS) $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

upstream/%.o: upstream/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f sam sam.o upstream/*.o
