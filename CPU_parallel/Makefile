CC = gcc
CFLAGS = -c -MD -g -Wall
PTHREADFLAG = -pthread
SRC = main.c Integral.c CPU_info.c
OBJ = $(SRC:.c=.o)

all: Integrate

Integrate: $(OBJ)
	$(CC) $(PTHREADFLAG) -o $@ $^

#%.o: %.c
#	$(CC) $(THREADFLAG) $(CFLAGS) -o $@ $<

clean:
	rm -rf $(OBJ) $(EXEC) $(OBJ:.o=.d)
	rm Integrate

-include $(OBJ:.o=.d)

.PHONY: clean run