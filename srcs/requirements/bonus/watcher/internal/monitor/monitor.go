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

func (t Target) Check() probe.Result {
	overallUp := true
	var details []probe.Detail
	for _, c := range t.Checkers {
		r := c.Check()
		if !r.Up {
			overallUp = false
		}
		details = append(details, r.Details...)
	}
	return probe.Result{Name: t.Name, Up: overallUp, Details: details}
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
