#!/bin/env bash

#	/* QuasarFoks Systemd-rc 
#    	/* Fake systemd-logind 
#    	/* This is use Elogind (fork systemd-logind) 

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
PURPLE='\033[0;35m'
CYAN='\033[0;36m'
NC='\033[0m'

info() {
    echo -e "${BLUE}[info]${NC} $1"
}

error() {
    echo -e "${RED}[error]${NC} $1"
}

warning() {
    echo -e "${YELLOW}[warning]${NC} $1"
}

success() {
    echo -e "${GREEN}[success]${NC} $1"
}

dependencies_list() {
	echo "Devuan Linux:"
	echo "sudo apt install elogind polkit dbus"
	echo "-------------------------------------"
	echo "Artix/Quasar Linux:"
	echo "sudo pacman -S elogind polkit dbus"
	echo "-------------------------------------"
	echo "Void Linux: "
	echo "sudo xbps-install elogind polkit dbus"
	echo "-------------------------------------"
	echo "Alpine Linux: "
	echo "sudo apk add elogind polkit dbus"
	exit 0
}
dependencies_check() {
	info "Check dependencies"
	# check dbus
	if which dbus-launch; then
		info "Dbus is found"
	else
		error "Please install dbus"
		dependencies_list
	fi
	# check elogind
	if ls /usr/lib/elogind/elogind; then
		info "elogind is found"
	else
		error "Please install elogind"
		dependencies_list
	fi
}
install_fake_systemd_logind() {
	ln -sf /usr/lib/elogind /usr/lib/systemd-logind
	ln -sf /usr/lib/elogind/libelogind.so.0 /usr/lib/libsystemd.so.0
	ln -sf /usr/lib/elogind/libelogind.so.0 /usr/lib/libsystemd-logind.so.0
	info "[Systemd-rc]: Good ! "
}
main() {
	dependencies_check
	install_fake_systemd_logind
}
main
