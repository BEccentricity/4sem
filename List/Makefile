HDIRS = headers
SRC = main.c List.c test.c

OBJ=$(patsubst %.c,%.o,$(SRC))

VPATH=../

CFLAGS=--coverage -Wall -Wextra -MMD $(addprefix -I,$(HDIRS))

PHONY: test valgrind coverage clean

test: code
	./code

valgrind: code
	valgrind ./code

code: $(OBJ)
	gcc -o code --coverage $^

coverage: test
	gcov $(patsubst %.gcda,%.c,$(SRC))

clean:
	rm *.o *.d *.gcno *.gcda *gcov code
	
include $(wildcard *.d)