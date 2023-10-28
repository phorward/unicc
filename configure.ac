AC_PREREQ([2.69])
AC_INIT([UniCC], [1.9.0])
AC_PREFIX_DEFAULT([/usr])
AC_CONFIG_SRCDIR([src/main.c])
AM_INIT_AUTOMAKE([-Wall -Werror foreign subdir-objects])

# Enable Trace facilities?
AC_MSG_CHECKING([whether Phorward trace facilities will be integrated])
AC_ARG_WITH([trace],
	[AS_HELP_STRING([--with-trace],
		[enable trace macro facilities (huger binary files)])],
		[with_trace="yes" AC_DEFINE([DEBUG])],
		[with_trace="no"])
AC_MSG_RESULT([$with_trace])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MAKE_SET

# Checks for libraries.
AC_CHECK_LIB([rt], [sprintf])

AC_DEFINE([UNICODE],[1])
AC_DEFINE([UTF8],[1])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset strrchr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
