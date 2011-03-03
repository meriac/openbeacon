AC_INIT([estimator],[0.1],[meriac@bitmanufaktur.de])
AM_INIT_AUTOMAKE([-Wall -Werror])
AC_PROG_CC
AC_PROG_CXX
AC_LANG_CPLUSPLUS
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_FILES([
    Makefile
    src/Makefile
])
AC_OUTPUT

AC_CHECK_FUNCS([gettimeofday])
AC_CHECK_FUNCS([memset])
AC_CHECK_FUNCS([socket])
AC_CHECK_FUNCS([sqrt])

AC_CHECK_HEADERS([arpa/inet.h])
AC_CHECK_HEADERS([netinet/in.h])
AC_CHECK_HEADERS([stdint.h])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([string.h])
AC_CHECK_HEADERS([sys/socket.h])
AC_CHECK_HEADERS([sys/time.h])
AC_CHECK_HEADERS([unistd.h])
AC_CHECK_HEADERS([pthread.h])

AC_CHECK_LIB([pthread],[pthread_create],[LDFLAGS=-lpthread],[AC_MSG_ERROR([No pthread library is available])])

AC_C_INLINE
AC_FUNC_MALLOC
AC_HEADER_STDBOOL
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T
AC_TYPE_UINT8_T
