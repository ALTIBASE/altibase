# platform_freebsd_pthread.GNU,v 4.14 1999/05/26 06:59:33 nanbor Exp

# platform_freebsd_pthread.GNU

include $(PD_DIR)/makefiles/platform_freebsd.GNU

LDFLAGS		+= -pthread
CFLAGS          += -D_THREAD_SAFE
