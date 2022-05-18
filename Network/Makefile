CC = gcc
CFLAGS = -c -MD -g -Wall -std=gnu99
THREADFLAG = -pthread

INC_DIR = ./includes
SRC_DIR = ./src

INC = -I$(INC_DIR)

SRC = main.c $(SRC_DIR)/DistributionNetwork.c $(SRC_DIR)/IntegralCalculator.c $(SRC_DIR)/ParseInfo.c
OBJ = $(SRC:.c=.o)
EXEC = code

all: $(EXEC)

$(EXEC): $(OBJ)
	$(CC) $(THREADFLAG) $(INC) -o $@ $^

%.o: %.c
	$(CC) $(THREADFLAG) $(CFLAGS) $(INC) -o $@ $<

clean:
	rm -rf $(OBJ) $(EXEC) $(OBJ:.o=.d)

-include $(OBJ:.o=.d)

.PHONY: clean clean_trash