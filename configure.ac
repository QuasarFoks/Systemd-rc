AC_INIT([Systemd-rc], [0.3], [quasarfoks@proton.me])
AM_INIT_AUTOMAKE([-Wall foreign subdir-objects])
AC_PROG_CXX

AC_ARG_WITH([init],
  [AS_HELP_STRING([--with-init=NAME], [openrc, runit, dinit, s6, bsd])],
  [init=$withval], [init=openrc])
AS_CASE([$init], [openrc|runit|dinit|s6|bsd], [],
  [AC_MSG_ERROR([unknown init: $init])])
AM_CONDITIONAL([INIT_OPENRC], [test "x$init" = "xopenrc"])
AM_CONDITIONAL([INIT_RUNIT],  [test "x$init" = "xrunit"])
AM_CONDITIONAL([INIT_DINIT],  [test "x$init" = "xdinit"])
AM_CONDITIONAL([INIT_S6],     [test "x$init" = "xs6"])
AM_CONDITIONAL([INIT_BSD],    [test "x$init" = "xbsd"])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
