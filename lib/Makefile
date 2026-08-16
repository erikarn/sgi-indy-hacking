SUBDIRS=libbres libbres_sub libftri libpoint libscanline

all:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir all; \
	done

clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done

default:
	all
