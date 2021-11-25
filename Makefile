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

tests/test: tests/tests.o $(PRG).o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

test: tests/test
	$<

clean:
	rm -f *.o *.a *.so tests/*.o tests/test
