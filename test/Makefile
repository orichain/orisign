CC = clang
CFLAGS = -O3 -march=native
INCLUDES = -I..
LDFLAGS = -L.. -lorisign -lm

all: test_orisign

test_orisign: test_orisign.c
	$(CC) $(CFLAGS) $(INCLUDES) test_orisign.c $(LDFLAGS) -o test_orisign
	@echo "\nRun './test_orisign' to execute the integration tests."

clean:
	rm -f test_orisign
