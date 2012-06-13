AC_PREREQ([2.69])
AC_INIT([UniCC], [1.0.0])
AC_PREFIX_DEFAULT([/usr])
AC_CONFIG_SRCDIR([p_main.c])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

#Bootstrap toolchain should be built?
AC_MSG_CHECKING([if UniCC bootstrap toolchain is wanted])
AC_ARG_WITH([bootstrap],
	[AS_HELP_STRING([--with-bootstrap],
		[configures to compile UniCC parser bootstrap toolchain])],
		[with_bootstrap="yes"],
		[with_bootstrap="no"])

AM_CONDITIONAL([WITH_BOOTSTRAP],[test "$with_bootstrap" = "yes"])
AC_MSG_RESULT([$with_bootstrap])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MAKE_SET

# Check for min_lalr1.
AS_IF([test "$with_bootstrap" = "yes"],
	[AC_CHECK_PROG([MIN_LALR1], [min_lalr1], "yes", "no")
	if test $MIN_LALR1 = "no" ; then
		AC_MSG_ERROR([Cannot build without min_lalr1 parser generator; Install Phorward Foundation Toolkit first, or check your PATH.])
	fi],[])

# Checks for libraries.
AC_CHECK_LIB([rt], [sprintf])
AC_CHECK_LIB([phorward], [pmalloc], [], [AC_MSG_ERROR([Cannot build without libphorward; Install Phorward Foundation Toolkit first, or set your LDFLAGS correct.])])


# Checks for header files.
AC_CHECK_HEADER([phorward.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset strrchr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
