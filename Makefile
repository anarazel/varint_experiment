CFLAGS = -O2 -Wall -Wextra -Werror
LDFLAGS =  -O2 -Wall -Wextra -Werror

all: varint_test

varint_test: varint.o varint_test.o

clean:
	rm -f  varint.o varint_test.o varint_test

test: varint_test
	./varint_test
	./varint_test -u

bench: varint_test
	time ./varint_test -b
	time ./varint_test -b -u
