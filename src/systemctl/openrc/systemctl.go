package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
)

// runCommand — запускает команду и выводит stdout/stderr
func runCommand(name string, arg ...string) error {
	cmd := exec.Command(name, arg...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}

// isServiceEnabled проверяет, включена ли служба в автозагрузку (runlevel 'default')
func isServiceEnabled(service string, userMode bool) bool {
	if service == "" || strings.ContainsAny(service, "/\\") || service == "." || service == ".." {
		return false
	}

	var path string
	if userMode {
		path = filepath.Join(os.Getenv("HOME"), ".local/share/openrc/runlevels/default", service)
	} else {
		path = filepath.Join("/etc/runlevels/default", service)
	}

	info, err := os.Lstat(path)
	return err == nil && info.Mode()&os.ModeSymlink != 0
}

func main() {
	if len(os.Args) < 2 {
		fmt.Fprintln(os.Stderr, "Usage: systemctl [OPTIONS...] COMMAND [SERVICE...]")
		os.Exit(2)
	}

	var nowFlag, userFlag bool
	var pos int

	// Находим позицию первой НЕ-флаговой команды
	for pos = 1; pos < len(os.Args); pos++ {
		arg := os.Args[pos]
		if arg == "--now" {
			nowFlag = true
		} else if arg == "--user" {
			userFlag = true
		} else if arg == "-q" || arg == "--quiet" || arg == "-v" || arg == "--verbose" {
			// known flags — skip
			continue
		} else if strings.HasPrefix(arg, "-") {
			// unknown flag — skip (like systemd)
			continue
		} else {
			// Это команда — выходим
			break
		}
	}

	if pos >= len(os.Args) {
		fmt.Fprintln(os.Stderr, "error: no command specified")
		os.Exit(2)
	}

	rest := os.Args[pos:]
	command := rest[0]
	services := rest[1:]



	// Системные команды — не поддерживают --user
	systemOnly := map[string]bool{
		"halt":      true,
		"poweroff":  true,
		"reboot":    true,
		"suspend":   true,
		"hibernate": true,
		"list-units": true,
		"list-unit-files": true,
	}

	if userFlag && systemOnly[command] {
		fmt.Fprintf(os.Stderr, "Warning: --user ignored for system command '%s'\n", command)
		userFlag = false
	}

	// Вспомогательная функция для построения команды с --user
	buildCmd := func(baseCmd []string) []string {
		if userFlag {
			// Вставляем --user сразу после имени утилиты
			return append([]string{baseCmd[0], "--user"}, baseCmd[1:]...)
		}
		return baseCmd
	}

	switch command {
		case "enable":
			if len(services) == 0 {
				fmt.Fprintln(os.Stderr, "error: service name required")
				os.Exit(2)
			}
			for _, svc := range services {
				cmd := buildCmd([]string{"rc-update", "add", svc, "default"})
				runCommand(cmd[0], cmd[1:]...)
			}
			if nowFlag {
				for _, svc := range services {
					cmd := buildCmd([]string{"rc-service", svc, "start"})
					runCommand(cmd[0], cmd[1:]...)
				}
			}

		case "disable":
			if len(services) == 0 {
				fmt.Fprintln(os.Stderr, "error: service name required")
				os.Exit(2)
			}
			for _, svc := range services {
				cmd := buildCmd([]string{"rc-update", "del", svc})
				runCommand(cmd[0], cmd[1:]...)
			}
			if nowFlag {
				for _, svc := range services {
					cmd := buildCmd([]string{"rc-service", svc, "stop"})
					runCommand(cmd[0], cmd[1:]...)
				}
			}

		case "start", "stop", "restart", "reload", "status":
			if len(services) == 0 {
				fmt.Fprintln(os.Stderr, "error: service name required")
				os.Exit(2)
			}
			for _, svc := range services {
				cmd := buildCmd([]string{"rc-service", svc, command})
				runCommand(cmd[0], cmd[1:]...)
			}

		case "is-enabled":
			if len(services) == 0 {
				fmt.Fprintln(os.Stderr, "error: service name required")
				os.Exit(2)
			}
			allEnabled := true
			for _, svc := range services {
				// Для --user нужно проверять ~/.local/share/openrc/init.d/
				enabled := isServiceEnabled(svc, userFlag)
				if enabled {
					fmt.Printf("%s enabled\n", svc)
				} else {
					fmt.Printf("%s disabled\n", svc)
					allEnabled = false
				}
			}
			if !allEnabled {
				os.Exit(1)
			}

		case "list-units":
			runCommand("rc-status", "-a")

		case "list-unit-files":
			if userFlag {
				runCommand("rc-update", "--user", "show")
			} else {
				runCommand("rc-update", "show")
			}

		case "halt":
			runCommand("halt")
		case "poweroff":
			runCommand("poweroff")
		case "reboot":
			runCommand("reboot")
		case "suspend":
			runCommand("loginctl", "suspend")
		case "hibernate":
			runCommand("loginctl", "hibernate")

		default:
			fmt.Fprintf(os.Stderr, "Unknown command: %s\n", command)
			os.Exit(2)
	}
}
