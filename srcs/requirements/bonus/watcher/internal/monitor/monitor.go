package monitor

import (
	"sync"
	"time"

	"watcher/internal/probe"
)

type Target struct {
	Name     string
	Checkers []probe.Checker
}

// severity はステータスの深刻さを表す。値が大きいほど深刻。
func severity(s probe.Status) int {
	switch s {
	case probe.Down:
		return 2
	case probe.Unhealthy:
		return 1
	case probe.Healthy:
		return 0
	default:
		return 3 // 不明(Unknown) は最も深刻として扱う
	}
}

func (t Target) Check() probe.Result {
	overall := probe.Healthy
	var details []probe.Detail
	for _, c := range t.Checkers {
		r := c.Check()
		if severity(r.Status) > severity(overall) {
			overall = r.Status
		}
		details = append(details, r.Details...)
	}
	return probe.Result{Name: t.Name, Status: overall, Details: details}
}

type Store struct {
	mu      sync.Mutex
	results []probe.Result
}

func (s *Store) Set(results []probe.Result) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.results = results
}

func (s *Store) Get() []probe.Result {
	s.mu.Lock()
	defer s.mu.Unlock()
	out := make([]probe.Result, len(s.results))
	copy(out, s.results)
	return out
}

func Start(store *Store, targets []Target, interval time.Duration) {
	ticker := time.NewTicker(interval)
	for {
		runOnce(store, targets)
		<-ticker.C
	}
}

func runOnce(store *Store, targets []Target) {
	var wg sync.WaitGroup
	results := make([]probe.Result, len(targets))
	for i, t := range targets {
		wg.Add(1)
		go func(i int, t Target) {
			defer wg.Done()
			results[i] = t.Check()
		}(i, t)
	}
	wg.Wait()
	store.Set(results)
}
