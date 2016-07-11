CFLAGS := -Wall -O2 -g3
INCLUDES :=
LIBS := -lpthread

HEADERS := litmus.h parse.h olta.h affinity.h arm.h results.h log.h timing.h
OBJS := affinity.o parse.o results.o log.o arm-asm.o timing.o olta.o

olta: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $@ $(LIBS)

clean:
	rm -f $(OBJS) olta

%.o: %.c %.h $(HEADERS)
	$(CC) $(CFLAGS) $(INCLUDES) -c -o $@ $<
