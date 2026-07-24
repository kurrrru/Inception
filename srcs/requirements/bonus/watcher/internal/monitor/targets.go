package monitor

import (
	"time"

	"watcher/internal/probe"
)

var Targets = []Target{
	{Name: "mariadb", Checkers: []probe.Checker{
		probe.TCPChecker{Name: "mariadb", Addr: "mariadb:3306", Timeout: 2 * time.Second},
	}},
	{Name: "redis", Checkers: []probe.Checker{
		probe.TCPChecker{Name: "redis", Addr: "redis:6379", Timeout: 2 * time.Second},
	}},
	{Name: "nginx", Checkers: []probe.Checker{
		probe.HTTPChecker{Name: "nginx", URL: "https://nginx/", Timeout: 2 * time.Second},
	}},
	{Name: "wordpress", Checkers: []probe.Checker{
		probe.TCPChecker{Name: "wordpress", Addr: "wordpress:9000", Timeout: 2 * time.Second},
	}},
}
