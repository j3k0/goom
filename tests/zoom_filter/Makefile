# Standalone conformance + stress test for zoom_filter_neon vs zoom_filter_c.
# No framework; builds against the real Goom/filters.c reference.

TOP    = ../..
GOOM   = $(TOP)/Goom

CC     = cc
CFLAGS = -O2 -Wall -I$(GOOM)

OBJS = zoom_filter_test.o filters.o graphic.o config_param.o zoom_filter_neon.o

zoom_filter_test: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm

zoom_filter_test.o: zoom_filter_test.c
	$(CC) $(CFLAGS) -c $< -o $@

filters.o: $(GOOM)/filters.c
	$(CC) $(CFLAGS) -c $< -o $@

graphic.o: $(GOOM)/graphic.c
	$(CC) $(CFLAGS) -c $< -o $@

config_param.o: $(GOOM)/config_param.c
	$(CC) $(CFLAGS) -c $< -o $@

zoom_filter_neon.o: $(GOOM)/zoom_filter_neon.c
	$(CC) $(CFLAGS) -c $< -o $@

test: zoom_filter_test
	./zoom_filter_test

bench: zoom_filter_test
	./zoom_filter_test --bench

clean:
	rm -f zoom_filter_test *.o

.PHONY: test bench clean
