CC=gcc
CFLAGS=-I.

speed_test: speed_test.o
	$(CC) -o speed_test speed_test.o -lmosquitto -lstdc++
	
speed_test.o.o: speed_test.cpp autotrain.h cred.h
	$(CC) -o speed_test.o -c speed_test.cpp
