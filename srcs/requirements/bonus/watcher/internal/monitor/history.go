package monitor

import (
	"time"
	"watcher/internal/probe"
)

type history struct {
	count        int
	healthyCount int
	latencySum   map[string]time.Duration
	lastStatus   probe.Status
}

type Summary struct {
	UptimePercent float64
	AvgLatency    map[string]time.Duration
	LastStatus    probe.Status
}
