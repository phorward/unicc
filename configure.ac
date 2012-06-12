AC_PREREQ([2.69])
AC_INIT([UniCC], [1.0.0])
AC_PREFIX_DEFAULT([/usr])
AC_CONFIG_SRCDIR([p_main.c])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])

# Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_MAKE_SET

# Check for min_lalr1.
AC_CHECK_PROG([MIN_LALR1], [min_lalr1], [Y], [N])
if test $MIN_LALR1 = N ; then
	AC_MSG_ERROR([Cannot build without min_lalr1 parser generator;
	Install Phorward Foundation Libraries first, or check your PATH.])
fi

# Checks for libraries.
AC_CHECK_LIB([rt], [sprintf])
AC_CHECK_LIB([phorward], [pmalloc])

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.
AC_CHECK_FUNCS([memset strrchr])

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
