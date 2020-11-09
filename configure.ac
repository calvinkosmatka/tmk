AC_INIT([tmk], [0.2], [calvin.kosmatka@gmail.com])
AM_INIT_AUTOMAKE([-Wall -Werror foreign])
AC_PROG_CC
AC_CONFIG_HEADERS([config.h])
AC_CHECK_LIB([asound], [snd_seq_event_input])
AC_CONFIG_FILES([Makefile
		 src/Makefile
		 ])
AC_OUTPUT
