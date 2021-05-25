#DEFS += -DDEBUG
#DEFS += -DPSPE

PBP = EBOOT.PBP
BINARY = out
SRCS = $(wildcard *.c) $(wildcard psp/*.c) 
OBJS = psp/startup.o $(addsuffix .o,$(basename $(SRCS)))
LIBS = -lc

all: $(PBP)

$(PBP): $(BINARY)
	outpatch
	elf2pbp outp "InfoNES"

$(BINARY): $(OBJS)
	$(LD) $(LDFLAGS) $(CFLAGS) $(OBJS) $(LIBS) -o $@
	$(STRIP) $(BINARY)

%.o: %.c
	$(CC) $(DEFS) $(INCLUDES) $(CFLAGS) -c $< -o $@

%.o: %.S
	$(CC) $(DEFS) $(ARCHFLAGS) -c $< -o $@

clean:
	rm *.o *.map out outp psp/*.o

include psp/Makefile.psp
