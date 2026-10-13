# Standalone invariant test for the perceived-flow field and the
# gauge-aware kaleidoscope transitions. No framework; builds against the
# real Goom/filters.c reference, like tests/zoom_filter.

TOP    = ../..
GOOM   = $(TOP)/Goom

CC     = cc
CFLAGS = -O2 -Wall -I$(GOOM)

OBJS = kaleido_flow_test.o filters.o graphic.o config_param.o goom_tools.o

kaleido_flow_test: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm

kaleido_flow_test.o: kaleido_flow_test.c
	$(CC) $(CFLAGS) -c $< -o $@

filters.o: $(GOOM)/filters.c
	$(CC) $(CFLAGS) -c $< -o $@

graphic.o: $(GOOM)/graphic.c
	$(CC) $(CFLAGS) -c $< -o $@

config_param.o: $(GOOM)/config_param.c
	$(CC) $(CFLAGS) -c $< -o $@

goom_tools.o: $(GOOM)/goom_tools.c
	$(CC) $(CFLAGS) -c $< -o $@

test: kaleido_flow_test
	./kaleido_flow_test

clean:
	rm -f kaleido_flow_test *.o

.PHONY: test clean
