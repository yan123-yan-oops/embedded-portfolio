CC      ?= gcc
CFLAGS  ?= -std=c11 -O2 -g -Wall -Wextra
SRCS     = src/nmea_parser.c src/nmea_stream.c

.PHONY: all clean test

all: demo stream_test dump_csv

demo: $(SRCS) src/main.c src/nmea_parser.h src/nmea_stream.h
	$(CC) $(CFLAGS) $(SRCS) src/main.c -o $@

stream_test: $(SRCS) src/stream_test.c src/nmea_parser.h src/nmea_stream.h
	$(CC) $(CFLAGS) $(SRCS) src/stream_test.c -o $@

dump_csv: $(SRCS) src/dump_csv.c src/nmea_parser.h
	$(CC) $(CFLAGS) $(SRCS) src/dump_csv.c -o $@

test: stream_test
	./stream_test test/sample_track.nmea

clean:
	rm -f demo stream_test dump_csv
