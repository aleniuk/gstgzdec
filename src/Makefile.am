plugin_LTLIBRARIES = libgstgzdec.la


# sources used to compile this plug-in
libgstgzdec_la_SOURCES = gstgzdec.c zip-dec-wrapper.c

# compiler and linker flags used to compile this plugin, set in configure.ac
libgstgzdec_la_CFLAGS = $(GST_BASE_CFLAGS) $(GST_CFLAGS) $(ZL_CFLAGS)
libgstgzdec_la_LIBADD = $(GST_BASE_LIBS) $(GST_LIBS) $(ZL_LIBS) -lbz2
libgstgzdec_la_LDFLAGS = $(GST_PLUGIN_LDFLAGS)
libgstgzdec_la_LIBTOOLFLAGS = --tag=disable-static

# headers we need but don't want installed
noinst_HEADERS = zip-dec-wrapper.h
