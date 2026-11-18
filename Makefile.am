PROGRAM = Systemd-rc
systemctl_SOURCE = src/systemctl
bin_PROGRAMS = systemctl
if INIT_OPENRC
systemctl_SOURCES = src/systemctl/openrc/systemctl.cpp
endif
if INIT_RUNIT
systemctl_SOURCES = src/systemctl/runit/systemctl.cpp
endif
if INIT_DINIT
systemctl_SOURCES = src/systemctl/dinit/systemctl.cpp
endif
if INIT_S6
systemctl_SOURCES = src/systemctl/s6/main.cpp
endif
if INIT_BSD
systemctl_SOURCES = src/systemctl/bsd/main.cpp
endif
MAINTAINERCLEANFILES = Makefile.in configure aclocal.m4 \
  depcomp install-sh missing configure~

