dnl required version of autoconf
AC_PREREQ([2.53])

AC_INIT([gstgzdec],[0.0.0])

AC_CONFIG_SRCDIR([src/gstgzdec.c])
AC_CONFIG_HEADERS([config.h])

dnl required version of automake
AM_INIT_AUTOMAKE([1.10])

dnl arg --enable-version-1
AC_ARG_ENABLE([version-1], "builds with gstreamer-1.x API", [
	GST_REQUIRED=1.0
	GSTPB_REQUIRED=1.0
	GST_POSTFIX=1.0
] , [
	GST_POSTFIX=0.10
	GST_REQUIRED=0.10.16
	GSTPB_REQUIRED=0.10.16
])

dnl enable mainainer mode by default
AM_MAINTAINER_MODE([enable])

dnl check for tools (compiler etc.)
AC_PROG_CC

dnl required version of libtool
LT_PREREQ([2.2.6])
LT_INIT

dnl give error and exit if we don't have pkgconfig
AC_CHECK_PROG(HAVE_PKGCONFIG, pkg-config, [ ], [
  AC_MSG_ERROR([You need to have pkg-config installed!])
])

dnl Check for the required version of GStreamer core (and gst-plugins-base)
dnl This will export GST_CFLAGS and GST_LIBS variables for use in Makefile.am
PKG_CHECK_MODULES(GST, [
  gstreamer-$GST_POSTFIX >= $GST_REQUIRED
  gstreamer-base-$GST_POSTFIX >= $GST_REQUIRED
  gstreamer-controller-$GST_POSTFIX >= $GST_REQUIRED
], [
  AC_SUBST(GST_CFLAGS)
  AC_SUBST(GST_LIBS)
], [
  AC_MSG_ERROR([
      You need to install or upgrade the GStreamer development
      packages on your system. On debian-based systems these are
      libgstreamer$GST_POSTFIX-dev and libgstreamer-plugins-base$GST_POSTFIX-dev.
      on RPM-based systems gstreamer$GST_POSTFIX-devel, libgstreamer$GST_POSTFIX-devel
      or similar. The minimum version required is $GST_REQUIRED.
  ])
])

PKG_CHECK_MODULES(ZL, zlib >= 1.0, [
  AC_SUBST(ZL_CFLAGS)
  AC_SUBST(ZL_LIBS)
], AC_MSG_ERROR([no zlib found]))




dnl check if compiler understands -Wall (if yes, add -Wall to GST_CFLAGS)
AC_MSG_CHECKING([to see if compiler understands -Wall])
save_CFLAGS="$CFLAGS"
CFLAGS="$CFLAGS -Wall"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([ ], [ ])], [
  GST_CFLAGS="$GST_CFLAGS -Wall"
  AC_MSG_RESULT([yes])
], [
  AC_MSG_RESULT([no])
])

dnl set the plugindir where plugins should be installed (for src/Makefile.am)
if test "x${prefix}" = "x$HOME"; then
  plugindir="$HOME/.gstreamer-$GST_POSTFIX/plugins"
else
  plugindir="\$(libdir)/gstreamer-$GST_POSTFIX"
fi
AC_SUBST(plugindir)

dnl set proper LDFLAGS for plugins
GST_PLUGIN_LDFLAGS='-module -avoid-version -export-symbols-regex [_]*\(gst_\|Gst\|GST_\).*'
AC_SUBST(GST_PLUGIN_LDFLAGS)

AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT

