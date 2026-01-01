#!/usr/bin/env sh

############################################################
# скрипт для использования в QuasarInstall и QuasarBuilder #
############################################################

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_FILE="$SCRIPT_DIR/src/systemctl/openrc/systemctl.go"
go build -o systemctl "$SRC_FILE"
