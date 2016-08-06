dnl Process this file with autoconf to produce a configure script.
AC_INIT([Lpe], [1.2.8])
AC_CONFIG_SRCDIR([src/lpe.c])
AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])
AM_INIT_AUTOMAKE

AM_MAINTAINER_MODE
AC_USE_SYSTEM_EXTENSIONS

dnl Checks for programs.
AC_PROG_CC
dnl AC_PROG_GCC_TRADITIONAL
AM_DISABLE_STATIC
AM_PROG_LIBTOOL
AC_PROG_MAKE_SET

dnl GNU gettext
ALL_LINGUAS="bg cs de es fr pl ru hu"
AM_GNU_GETTEXT

if test "$USE_INCLUDED_LIBINTL" = "no" && test "$USE_NLS" = "yes"
then
  if ! test "$gt_cv_func_gettext_libc" = "yes"
  then
    AC_CHECK_LIB(intl,gettext)
  fi
fi

AM_CONDITIONAL(USE_INCLUDED_LIBINTL, test x$USE_INCLUDED_LIBINTL = xyes)

dnl Checking for command line arguments
#AC_ARG_WITH(language,
#[  --with-language=LL
#                          choose a language for the man page by specifying
#                          the ISO two-letter language code.],
#[
#  if test x$withval = xyes
#  then
#    withval=en
#  elif test x$withval = xno
#  then
#    withval=en
#  fi
#  $LANGUAGE=$withval
#])
#if test x$LANGUAGE = x
#then
#  LANGUAGE=en
#fi
#AC_SUBST(LANGUAGE)

AC_MSG_CHECKING(for modes to build)
AC_ARG_WITH(modes,
[  --with-modes={mode}[,...]
                          include mode modules for {mode}. {mode} can be
                          one of c, cpp, html, sgml, mail, perl, java or lisp.
  --with-modes=[all|yes]  include all modes. This is the default when building
                          with shared libraries.
  --without-modes         do not build any mode modules. This is the default
                          when building without shared libraries],
[
 if test "$withval" = "all" || test "$withval" = "yes"
 then
   AC_MSG_RESULT(all)
   MODES="all"
 elif test "$withval" = "no"
 then
   AC_MSG_RESULT(none)
   MODES=""
 else
   AC_MSG_RESULT(some)
   list="`echo $withval | tr , ' '`"
   for mode in ${list}
   do
     MODES="$MODES ${mode}mode.la"
   done
 fi
],
[if test "$enable_shared" = "yes"
 then
   MODES=all
   AC_MSG_RESULT(all)
 else
   MODES=""
   AC_MSG_RESULT(none)
 fi
])
if test "${MODES}" = "all"
then
  MODES="cmode.la cppmode.la perlmode.la mailmode.la"
  MODES="$MODES htmlmode.la sgmlmode.la javamode.la lispmode.la"
fi
if test "${MODES}" != ""
then
  if test "$enable_shared" = "no"
  then
    AC_MSG_WARN([Modes not yet supported without shared libraries.])
    MODES=""
  fi
fi
AC_SUBST(MODES)

dnl Checks for libraries.
AC_CHECK_FUNCS(dlopen,,[
AC_CHECK_LIB(dl,dlopen,[DL_LIB="-ldl"],[
AC_CHECK_LIB(dld,shl_load,[DL_LIB="-ldld"],[
DL_LIB=""
dnl AC_MSG_WARN("$PACKAGE depends on dynamic libraries.")
])])])
AC_SUBST(DL_LIB)
AC_CHECK_LIB(slang,SLang_init_tty,[SLANG_LIB="-lslang"],[
dnl reset the cache variable so configure will even bother to check -ltermcap
unset ac_cv_lib_slang_SLang_init_tty
AC_CHECK_LIB(slang,SLang_init_tty,[SLANG_LIB="-lslang -ltermcap"],[
AC_MSG_ERROR([no slang library found.  $PACKAGE depends on SLang.])
,-ltermcap])])
AC_SUBST(SLANG_LIB)

AC_CHECK_LIB(ncurses, beep, [NCURSES_LIB="-lncurses"],[AC_MSG_ERROR([no ncurses library found.])])
AC_SUBST(NCURSES_LIB)

dnl Checks for linked in modes (not functional at the moment)
lpe_LDFLAGS=""
if test "$enable_static" = "yes" && test "$enable_shared" = "no"
then
  for mode in $MODES
  do
    lpe_LDFLAGS="$lpe_LDFLAGS -dlopen $mode"
    lpe_DEPS="$lpe_DEPS $mode"
  done
  if test "$lpe_LDFLAGS" != ""
  then
    lpe_LDFLAGS="-export-dynamic $lpe_LDFLAGS"
  fi
else
  if test "$MODES" != ""
  then
    lpe_LDFLAGS="-export-dynamic"
  fi
fi
AC_SUBST(lpe_LDFLAGS)
AC_SUBST(lpe_DEPS)

dnl Checks for header files.
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(limits.h sys/syslimits.h sys/file.h sys/ioctl.h unistd.h)
AC_CHECK_HEADERS(fcntl.h)
AC_CHECK_HEADER([slang.h], [], [AC_MSG_ERROR([slang.h is required])])

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T

dnl Checks for library functions.
AC_FUNC_VPRINTF
AC_CHECK_FUNCS(strcspn strerror strstr strtol flock scandir strncasecmp)

AC_OUTPUT([
Makefile
data/Makefile
man/Makefile
po/Makefile.in
src/Makefile
src/modes/Makefile])
