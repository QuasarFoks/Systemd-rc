package main

import (
	"fmt"
	"os"
	"os/exec"
	"path/filepath"
	"strings"
        "time"
        "syscall"
)
const (
        green = "\033[32m"
        red   = "\033[31m"
        reset = "\033[0m"
)
// runCommand — запускает команду и выводит stdout/stderr
func runCommand(name string, arg ...string) error {
	cmd := exec.Command(name, arg...)
	cmd.Stdout = os.Stdout
	cmd.Stderr = os.Stderr
	return cmd.Run()
}
func runCommandF(name string, arg ...string) error {
	cmd := exec.Command(name, arg...)
	cmd.Stdout = nil
	cmd.Stderr = nil
	return cmd.Run()
}
func isRoot() bool {
        return os.Geteuid() == 0
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

func Help() {
        fmt.Println("")
        fmt.Println("FLAGS                   Experimental flag ")
        fmt.Println("============Service management============")
        fmt.Println("enable       {SERVICE}     --now/--user   ")
        fmt.Println("disable      {SERVICE}     --now/--user   ")
        fmt.Println("status       {SERVICE}                    ")
        fmt.Println("restart      {SERVICE}       --user       ")
        fmt.Println("reload       {SERVICE}       --user       ")
        fmt.Println("stop         {SERVICE}       --user       ")
        fmt.Println("start        {SERVICE}       --user       ")
        fmt.Println("is-enabled   {SERVICE}       --user       ")
        fmt.Println("daemon-reload 			       ")
        fmt.Println("list-unit 			               ")
        fmt.Println("list-unit-files              --user       ")
        fmt.Println("=============Power management=============")
        fmt.Println("halt			               ")
        fmt.Println("poweroff 			               ")
        fmt.Println("reboot 		                       ")
        fmt.Println("suspend 			               ")
        fmt.Println("hibernate                                 ")
        fmt.Println("==========================================")
}
func getServiceStartTime(svc string) string {
        // 1. Читаем PID из pidfile
        pidfile := fmt.Sprintf("/run/%s.pid", svc)
        data, err := os.ReadFile(pidfile)
        if err != nil {
                return ""
        }

        pid := strings.TrimSpace(string(data))
        if pid == "" {
                return ""
        }

        // 2. Получаем время создания процесса из /proc
        procPath := fmt.Sprintf("/proc/%s", pid)
        var stat syscall.Stat_t
        if err := syscall.Stat(procPath, &stat); err != nil {
                return ""
        }

        // 3. Преобразуем timestamp в читаемый формат
        startTime := time.Unix(stat.Ctim.Sec, stat.Ctim.Nsec)
        return startTime.Format("2006-01-02 15:04:05")
}
func getServiceRunlevel(svc string) string {
        // Проверяем наличие симлинка в /etc/runlevels/default/
        defaultPath := "/etc/runlevels/default/" + svc
                if _, err := os.Stat(defaultPath); err == nil {
                        return "default"
                }

                // Можно проверить и другие runlevel-ы, если нужно
                bootPath := "/etc/runlevels/boot/" + svc
                if _, err := os.Stat(bootPath); err == nil {
                        return "boot"
                }

                return ""
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
				cmd := buildCmd([]string{"/usr/bin/rc-update", "add", svc, "default"})
				runCommand(cmd[0], cmd[1:]...)
			}
			if nowFlag {
				for _, svc := range services {
					cmd := buildCmd([]string{"/usr/bin/rc-service", svc, "start"})
					runCommand(cmd[0], cmd[1:]...)
				}
			}
			if !isRoot() {
                                for _, svc := range services {
                                        cmd := buildCmd([]string{"pkexec", "/usr/bin/rc-update", "add", svc, "default"})
                                        runCommand(cmd[0], cmd[1:]...)
                                }
                        }

		case "disable":
			if len(services) == 0 {
				fmt.Fprintln(os.Stderr, "error: service name required")
				os.Exit(2)
			}
			for _, svc := range services {
				cmd := buildCmd([]string{"/usr/bin/rc-update", "del", svc})
				runCommand(cmd[0], cmd[1:]...)
			}
			if nowFlag {
				for _, svc := range services {
					cmd := buildCmd([]string{"/usr/bin/rc-service", svc,})
					runCommand(cmd[0], cmd[1:]...)
				}
			}
			if !isRoot() {
                                for _, svc := range services {
                                        cmd := buildCmd([]string{"pkexec", "/usr/bin/rc-update", "del", svc})
                                        runCommand(cmd[0], cmd[1:]...)
                                }
                        }

		case "start", "stop", "restart", "reload":
			if len(services) == 0 {
				fmt.Fprintln(os.Stderr, "error: service name required")
				os.Exit(2)
			}
			for _, svc := range services {
				cmd := buildCmd([]string{"/usr/bin/rc-service", svc, command})
				runCommandF(cmd[0], cmd[1:]...)
			}
			if !isRoot() {
                                for _, svc := range services {
                                        cmd := buildCmd([]string{"pkexec", "/usr/bin/rc-service", svc, command})
                                        runCommandF(cmd[0], cmd[1:]...)
                                }
                        }
		case "status":
                        if len(services) == 0 {
                                fmt.Fprintln(os.Stderr, "error: service name required")
                                os.Exit(2)
                        }
                        for _, svc := range services {
                                enabled := isServiceEnabled(svc, userFlag)

                                statusis := "disabled"
                                runlevel := ""
                                if enabled {
                                        statusis = "enabled"
                                        runlevel = getServiceRunlevel(svc)  // получаем runlevel
                                }

                                cmd := exec.Command("/usr/bin/rc-service", svc, "status")
                                err := cmd.Run()
                                description := "temporary description"
                                startTime := getServiceStartTime(svc)



                                if err != nil {
                                        fmt.Println(red + "○ " + reset, svc+".service -", description)
                                        fmt.Printf("       Loaded: loaded (%s; %s; Runlevel: %s; vendor preset: Plug)\n",
                                                   "/etc/init.d/"+svc, statusis, runlevel)

                                } else {
                                        fmt.Println(green + "● " + reset, svc+".service -", description)
                                        fmt.Printf("       Loaded: loaded (%s; %s; Runlevel: %s; vendor preset: Plug)\n",
                                                   "/etc/init.d/"+svc, statusis, runlevel)
                                         fmt.Printf("       Active: active (running) since %s\n", startTime)


                                }
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
			runCommand("/usr/bin/rc-status", "-a")
		case "daemon-reload":
			runCommand("/usr/bin/rc-update", "-u")

		case "list-unit-files":
			if userFlag {
				runCommand("/usr/bin/rc-update", "--user", "show")
			} else {
				runCommand("/usr/bin/rc-update", "show")
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
		case "help":
			Help()
		default:
			fmt.Fprintf(os.Stderr, "Unknown command: %s\n", command)
			os.Exit(2)
	}
}
