# The autoconf initilization.
AC_INIT(pidgin-osd, 0.1.0, [mbroemme@plusserver.de], [pidgin-osd])

# Detect the canonical host and target build environment
AC_CANONICAL_SYSTEM

AM_INIT_AUTOMAKE([no-dependencies])
AC_CONFIG_HEADERS([config.h:config.h.in])

# Define some common variables.
AC_DEFINE_UNQUOTED(AUTHOR, "Maik Broemme <$PACKAGE_BUGREPORT>", [Package author])

# Notices
AC_PREREQ(2.53)
AC_REVISION($Revision: 1.6 $)

# Checking for programs.
AC_DISABLE_STATIC
AC_PROG_LIBTOOL
AC_PROG_MAKE_SET
AC_PROG_CC

# Checking for pidgin.
PKG_CHECK_MODULES(PIDGIN, [pidgin >= 2.0.0], HAVE_PIDGIN="yes", HAVE_PIDGIN="no")

# Checking for pidgin flags.
AC_SUBST(PIDGIN_CFLAGS)
AC_SUBST(PIDGIN_LDFLAGS)
PIDGIN_LIBDIR=`pkg-config --variable=libdir pidgin`
AC_SUBST(PIDGIN_LIBDIR)

# Checking for glib.
PKG_CHECK_MODULES(GLIB, [glib-2.0], HAVE_GLIB="yes", HAVE_GLIB="no")

# Checking for glib flags.
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LDFLAGS)

# Checking for gtk.
PKG_CHECK_MODULES(GTK, [gtk+-2.0], HAVE_GTK="yes", HAVE_GTK="no")

# Checking for gtk flags.
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LDFLAGS)

# Checking for xosd-config.
AC_PATH_PROG([xosdconfigpath], [xosd-config], [no])
if test "$xosdconfigpath" = "no"; then
	AC_MSG_ERROR([xosd-config binary is required])
fi

# Checking for xosd and flags.
AC_MSG_CHECKING([for xosd])
XOSD_VERSION=`$xosdconfigpath --version`
if test "$XOSD_VERSION" \< "2.2.14" ; then
	AC_MSG_ERROR([*** xosd version below 2.2.14 is incompatible with recent glibc (found $XOSD_VERSION)])
fi
XOSD_CFLAGS=`$xosdconfigpath --cflags`
if test -z "$XOSD_CFLAGS"; then
	AC_MSG_RESULT([not found.])
	exit
fi
XOSD_LDFLAGS=`$xosdconfigpath --libs`
if test -z "$XOSD_LDFLAGS"; then
	AC_MSG_RESULT([not found.])
	exit
fi
AC_MSG_RESULT([yes (found $XOSD_VERSION)])
AC_SUBST(XOSD_CFLAGS)
AC_SUBST(XOSD_LDFLAGS)

# Creating files.
AC_OUTPUT([
Makefile
src/Makefile
])
