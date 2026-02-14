all:
	bear -- clang -O3 -march=native orisign.c fips202.c -o orisign -lm
	@rm -rf *.o
clean:
	@rm -rf *.o
