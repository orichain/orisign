CC = clang
CFLAGS = -O3 -march=native
INCLUDES = -I..
LDFLAGS = -L.. -lorisign -lm

all: test_orisign test_attack

test_orisign: test_orisign.c
	$(CC) $(CFLAGS) $(INCLUDES) test_orisign.c $(LDFLAGS) -o test_orisign
	@echo "\nRun './test_orisign' to execute the integration tests."

test_attack: test_attack.c
	$(CC) $(CFLAGS) $(INCLUDES) test_attack.c $(LDFLAGS) -o test_attack
	@echo "\nRun './test_attack' to test Key Substitution Attack."

clean:
	rm -f test_orisign test_attack

