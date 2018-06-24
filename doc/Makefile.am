BUILT_MANS = \
	oidentd.8

man_MANS = \
	$(BUILT_MANS) \
	oidentd.conf.5 \
	oidentd_masq.conf.5

man_SOURCES = \
	oidentd.8.adoc

DISTCLEANFILES = \
	oidentd.8

EXTRA_DIST = \
	$(man_MANS) \
	$(man_SOURCES) \
	rfc1413

SUFFIXES = .adoc

oidentd.8: oidentd.8.adoc

.adoc:
	a2x --doctype manpage --format manpage -a "sysconfdir=\"$(sysconfdir)\"" $<
