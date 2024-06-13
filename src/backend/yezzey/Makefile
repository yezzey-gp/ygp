#-------------------------------------------------------------------------
#
# Makefile--
#    Makefile for yezzey
#
# IDENTIFICATION
#    src/backend/yezzey/Makefile
#
#-------------------------------------------------------------------------

subdir = src/backend/yezzey
top_builddir = ../../..
include $(top_builddir)/src/Makefile.global
override CPPFLAGS := -I$(libpq_srcdir) $(CPPFLAGS)


OBJS = yezzey.o

include $(top_srcdir)/src/backend/common.mk
