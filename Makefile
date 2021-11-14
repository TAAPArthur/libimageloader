-include auto_detect.mk

CFLAGS += -fPIC
PRG=img_loader
LIB=libimgloader.so

libimgloader.so: $(PRG).o
	$(CC) $(CFLAGS) -shared -o $@ $^ $(LDFLAGS)

libimgloader.a: $(PRG).o
	ar rcs $@ $^


install: $(LIB)
	install -Dt $(DESTDIR)/usr/lib $(LIB)

clean:
	rm -f *.o *.a *.so
