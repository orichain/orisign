CC = clang

CFLAGS = -O3 -march=native -fomit-frame-pointer -funroll-loops -fno-tree-vectorize -fwrapv -fno-strict-aliasing -fPIC -I/usr/local/include
LDFLAGS = -L/usr/local/lib -lm -lgmp
LIBNAME = liborisign.a

SRCS = globals.c fips202.c
OBJS = $(SRCS:.c=.o)

all: $(LIBNAME)

$(LIBNAME): $(OBJS)
	ar rcs $@ $^
	@echo "Library $(LIBNAME) created."

orisign: orisign.c $(LIBNAME)
	bear -- $(CC) $(CFLAGS) orisign.c -L. -lorisign $(LDFLAGS) -o orisign

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	@rm -rf *.o $(LIBNAME) orisign
	@echo "Clean!"
