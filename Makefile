CFLAGS = -g3 -Wall -Wextra -Wconversion -Wcast-qual -Wcast-align -g -D_GNU_SOURCE -std=gnu99
CFLAGS += -Winline -Wfloat-equal -Wnested-externs
CFLAGS += -pedantic -std=gnu99 -Werror

PROMPT = -DPROMPT
# variable declarations
CC = gcc
HEADERS = jobs.h
OBJS = sh.c jobs.c
EXECS = 33sh 33noprompt

.PHONY: all clean
all: $(EXECS)
33sh: $(OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -o 33sh $(PROMPT) $(OBJS)
33noprompt: $(OBJS) $(HEADERS)
	$(CC) $(CFLAGS) -o 33noprompt $(OBJS)
clean:
	rm -f $(EXECS)
