AC_PREREQ([2.69])

AC_INIT([Bitcoin Core],[29.0.0],[https://github.com/bitcoin/bitcoin/issues],[bitcoin],[https://bitcoincore.org/])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIR([build-aux/m4])

AC_PROG_CXX

echo "=================== BEFORE"
AX_PTHREAD([echo FOUND], [echo NOT_FOUND])
echo "=================== AFTER"

AC_OUTPUT

echo
echo "  PTHREAD_LIBS    = $PTHREAD_LIBS"
echo "  PTHREAD_CFLAGS  = $PTHREAD_CFLAGS"
echo "  PTHREAD_CC      = $PTHREAD_CC"
echo "  PTHREAD_CXX     = $PTHREAD_CXX"
echo
