SUBDIRS = src

subdirs: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

install: subdirs
	mkdir -p $(DESTDIR)/usr/sbin && cp src/irdc4cube $(DESTDIR)/usr/sbin
clean:
	for dir in $(SUBDIRS); do $(MAKE) -C $$dir clean; done

.PHONY: clean subdirs $(SUBDIRS)
