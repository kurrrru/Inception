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

func (s *Store) Summaries() map[string]Summary {
	s.mu.Lock()
	defer s.mu.Unlock()
	summaries := make(map[string]Summary)
	for name, h := range s.history {
		if h.count == 0 {
			continue
		}
		avgLatency := make(map[string]time.Duration)
		for kind, sum := range h.latencySum {
			avgLatency[kind] = sum / time.Duration(h.count)
		}
		summaries[name] = Summary{
			UptimePercent: float64(h.healthyCount) / float64(h.count) * 100,
			AvgLatency:    avgLatency,
			LastStatus:    h.lastStatus,
		}
	}
	return summaries
}
