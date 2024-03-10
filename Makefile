# This is just a wrapper for make.sh, which is the actual build script

all:
	./make.sh all

test:
	./make.sh test_all

stats:
	./make.sh stats

clean:
	./make.sh clean

.PHONY: all test stats clean 
