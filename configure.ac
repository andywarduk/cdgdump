AC_INIT(cdgdump, 0.1)
AC_CONFIG_SRCDIR([configure.ac])

AM_INIT_AUTOMAKE

AC_LANG([C++])
AC_PROG_CXX

dnl -----------------------
dnl -- SDL configuration --
dnl -----------------------

dnl Check if the user hasn't set the variable $SDL_CONFIG
AC_MSG_CHECKING([for SDL config])
if test -z "$SDL_CONFIG"; then
	SDL_CONFIG=`which sdl-config`
fi
if test -n "$SDL_CONFIG" && test -x "$SDL_CONFIG"; then
	AC_MSG_RESULT([$SDL_CONFIG])
	LIBSDL_LIBS=`$SDL_CONFIG --libs`
	LIBSDL_CFLAGS=`$SDL_CONFIG --cflags`
else
	AC_MSG_RESULT([missing])
	AC_MSG_ERROR([SDL not found])
fi

dnl ------------------------
dnl -- zlib configuration --
dnl ------------------------

AC_CHECK_HEADER(zlib.h, libz_ok=yes, libz_ok=no)
if test "x$libz_ok" = "xyes"; then
	old_libs="$LIBS"
	AC_CHECK_LIB(z, zlibVersion, libz_ok=yes, libz_ok=no)
	LIBS="$old_libs"
fi
if test "x$libz_ok" = xyes; then
	LIBZ_LIBS="-lz"
else
	AC_MSG_ERROR([Zlib not found])
fi

dnl --------------------------
dnl -- libpng configuration --
dnl --------------------------

AC_MSG_CHECKING([for libpng-config])
dnl Check if the user hasn't set the variable $PNG_CONFIG
if test -z "$PNG_CONFIG"; then
	PNG_CONFIG=`which libpng-config`
fi

dnl Check if the libpng-config script was found and is executable
if test -n "$PNG_CONFIG" && test -x "$PNG_CONFIG"; then
	AC_MSG_RESULT([$PNG_CONFIG])
    png_ok="yes"
else
	AC_MSG_RESULT([missing])
	png_ok="no"
fi

if test "x$png_ok" = "xyes"; then
	LIBPNG_CFLAGS=`$PNG_CONFIG --cflags`
	LIBPNG_LIBS=`$PNG_CONFIG --ldflags`
else
	AC_MSG_ERROR([libpng not found])
fi

AC_SUBST(LIBSDL_LIBS)
AC_SUBST(LIBSDL_CFLAGS)

AC_SUBST(LIBZ_LIBS)

AC_SUBST(LIBPNG_LIBS)
AC_SUBST(LIBPNG_CFLAGS)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

