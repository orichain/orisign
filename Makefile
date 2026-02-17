all:
	bear -- clang -O3 -march=native -fwrapv -fno-strict-aliasing orisign.c globals.c fips202.c -o orisign -lm
	@rm -rf *.o
clean:
	@rm -rf *.o
