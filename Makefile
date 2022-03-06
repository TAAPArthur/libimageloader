-include config.mk

PREFIX ?= /usr
CFLAGS += -fPIC
PRG=img_loader
LIB=libimgloader.so

libimgloader.so: $(PRG).o
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)

libimgloader.a: $(PRG).o
	ar rcs $@ $^

install: $(LIB)
	install -Dt $(DESTDIR)$(PREFIX)/lib $(LIB)
	install -Dt $(DESTDIR)$(PREFIX)/include/$(PRG) $(PRG).h

install-single-header:
	./extra/make_single_header.sh
	install -Dt $(DESTDIR)$(PREFIX)/include/$(PRG)_single_header.h single_header.h

examples/example: examples/example.o $(PRG).o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

CFLAGS += -g
tests/test: tests/tests.o $(PRG).o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: tests/test examples/example
	$^
	examples/example tests/test_image.png >/dev/null

clean:
	rm -f *.o *.a *.so tests/*.o tests/test
