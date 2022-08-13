PREFIX = /usr
PRG=img_loader
LIB=libimgloader.so

HAVE_LINUX ?= 1
CPPFLAGS_LINUX_1 = -DHAVE_LINUX
CPPFLAGS += $(CPPFLAGS_LINUX_$(HAVE_LINUX))

-include config.mk

libimgloader.so: $(PRG).o
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $^ $(LDFLAGS)

libimgloader.a: $(PRG).o
	ar rcs $@ $^

install: $(LIB)
	install -Dt $(DESTDIR)$(PREFIX)/lib $(LIB)
	install -Dt $(DESTDIR)$(PREFIX)/include/$(PRG) $(PRG).h

examples/example: examples/example.o $(PRG).o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

tests/test: CFLAGS += -g -DVERBOSE -UHAVE_LINUX
tests/test: tests/tests.o $(PRG).o
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

tests/posix_test: CFLAGS += -g -DVERBOSE -DHAVE_LINUX
tests/posix_test: tests/tests.c $(PRG).c
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

test: tests/test tests/posix_test examples/example
	./tests/test
	./tests/posix_test
	examples/example tests/test_image.png >/dev/null

clean:
	rm -f *.o *.a *.so tests/*.o tests/test tests/posix_test
