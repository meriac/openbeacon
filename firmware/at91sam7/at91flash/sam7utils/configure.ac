AC_INIT([sam7utils], [0.2.1])
LT_INIT

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_MAKE_SET

case "${host}" in
     *-*-darwin* )
       LIBS="$LIBS -framework IOKIT -framework CoreFoundation"
       IOTYPE="iokit"
     ;;	    
     *-*-cygwin* )
       LIBS="$LIBS -lsetupapi"
       IOTYPE="win32"
     ;;	    
     * )
       AC_CHECK_LIB(usb,usb_init,IOTYPE="libusb";LIBS="$LIBS -lusb",IOTYPE="posix")
       
     ;;
esac


AC_HEADER_STDC



AC_CHECK_LIB(readline, readline,,AC_MSG_ERROR(readline not found))
AC_CHECK_LIB(curses, tgoto,,AC_MSG_ERROR(curses not found))
AC_CHECK_HEADERS(stdint.h,,AC_MSG_WARN(can't find stdint.h))
AC_CHECK_HEADERS(ctype.h,,)
AC_CHECK_HEADERS(endian.h,,)




AC_SUBST(IOTYPE)

AC_OUTPUT([Makefile
	lib/Makefile])
