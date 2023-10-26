CC=g++
OPT=-Wall -Wextra -O2
OBJS=

all: img-search

img-search: main.cpp $(OBJS)
	$(CC) $(OPT) $(OPT) main.cpp -o img-search $(OBJS)

%.o: %.cpp %.h
	$(CC) $(OPT) $(DBG_OPT) -c $< -o $@
