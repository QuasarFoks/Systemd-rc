package main

import (
	"fmt"
	"os"
	"os/exec"
)
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
	err := cmd.Run()
        if err != nil {
                if exitErr, ok := err.(*exec.ExitError); ok {
                        return exitErr.ExitCode()
                }
                return -1
        }
        return 0
}

func isRoot() bool {
	return os.Geteuid() == 0
}

func main() {
	if len(os.Args) < 2 {
		fmt.Println("Usage: systemd-rc <command> [service]")
		os.Exit(1)
	}

	command := os.Args[1]
	service := ""
	if len(os.Args) > 2 {
		service = os.Args[2]
	}
 
	switch command {
	case "enable":
		if (!isRoot) {
			runCommandF("pkexec", "/usr/bin/sv", "enable", service)
                        err := runCommandF()
                        if err == 0 {
                                fmt.Println("Created symlink", "/etc/service/" + service, "--> /etc/sv/" + service)
                        } else {
                                os.exit(0)
                        }
		} else {
			runCommandF("/usr/bin/sv", "enable", service)
                        err := runCommandF()
                        if err == 0 {
                                fmt.Println("Created symlink", "/etc/service/" + service, "--> /etc/sv/" + service)
                        } else {
                                os.exit(0)
                        }
		}

	case "disable":
                if (!isRoot) {
                        runCommandF("pkexec", "sv", "disable", service)
                        err := runCommandF()
                        if err == 0 {
                                fmt.Println("Removed /etc/service/" + service)
                        } else {
                               os.exit(0)
                        }
                } else {
                        runCommandF("sv", "disable", service)
                        err := runCommandF()
                        if err == 0 {
                                fmt.Println("Removed /etc/service/" + service)
                        } else {
                                os.exit(0)
                        }
                }

	case "status":
                if (!isRoot) {
		runCommand("sv", "status", service)
	case "start":
                if (!isRoot) {
		runCommandF("sv", "up", service)
	case "stop":
                if (!isRoot) {
		runCommandF("sv", "down", service)
	case "reload":
                if (!isRoot) {

                }else {

                }
		runCommandF("sv", "hup", service)
	case "restart":
                if (!isRoot) {

                }
		runCommandF("sv", "restart", service)
	case "list-units":
		runCommand("sv", "-l")
	case "halt":
		runCommand("halt")
	case "poweroff":
		runCommand("poweroff")
	case "reboot":
		runCommand("reboot")
	}
}
