package main

import (
	"flag"
	"fmt"
	"os"
	"os/exec"
)

func getLogFile() string {
	paths := []string{
		"/var/log/everything.log",
		"/var/log/messages",
		"/var/log/syslog",
		"/var/log/rc.log",
	}
	for _, p := range paths {
		if _, err := os.Stat(p); err == nil {
			return p
		}
	}
	return "/var/log/messages"
}

func main() {
	var (
		unit   string
		follow bool
		kernel bool
	)

	flag.StringVar(&unit, "u", "", "Show logs for the specified service")
	flag.BoolVar(&follow, "f", false, "Follow the log output")
	flag.BoolVar(&kernel, "k", false, "Show kernel messages")
	flag.Parse()

	if kernel {
		exec.Command("dmesg").Run()
		return
	}

	logFile := getLogFile()

	if unit != "" {
		// Без -f — просто grep
		cmd := exec.Command("grep", "-i", unit, logFile)
		cmd.Stdout = os.Stdout
		cmd.Stderr = os.Stderr
		cmd.Run()
		return
	}

	if follow {
		exec.Command("tail", "-F", logFile).Run()
	} else {
		exec.Command("cat", logFile).Run()
	}
}
