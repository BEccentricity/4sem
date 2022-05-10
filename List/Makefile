HDIRS = headers
SRC = main.c List.c test.c

OBJ=$(notdir $(patsubst %.c,%.o,$(SRC)))

VPATH=../

CFLAGS=--coverage -Wall -Wextra -MMD $(addprefix -I,$(HDIRS))

PHONY: clean test valgrind coverage

test: test-bin
	./test-bin

valgrind: test-bin
	valgrind ./test-bin

test-bin: $(OBJ)
	gcc -o test-bin --coverage $^

coverage: test
	gcov $(notdir $(patsubst %.gcda,%.c,$(SRC)))

clean:
	rm -f *.o *.d *.gcno *.gcda *.gcov test-bin
	
include $(wildcard *.d)