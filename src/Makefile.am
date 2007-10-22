bin_PROGRAMS = cdgdump
cdgdump_SOURCES = CdgDump.cpp CDG.cpp SDLWin.cpp PNGOut.cpp

# set the include path found by configure
AM_CPPFLAGS = $(LIBSDL_CFLAGS) $(LIBPNG_CFLAGS)

# the library search path.
cdgdump_LDFLAGS = 
cdgdump_LDADD = $(LIBSDL_LIBS) $(LIBZ_LIBS) $(LIBPNG_LIBS)
