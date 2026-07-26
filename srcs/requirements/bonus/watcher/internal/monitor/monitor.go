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
	case probe.Unknown:
		return 3
	case probe.Down:
		return 2
	case probe.Unhealthy:
		return 1
	case probe.Healthy:
		return 0
	default:
		return 3 // ゼロ値や想定外の文字列も不明として扱う
	}
}

func (t Target) Check() probe.Result {
	overall := probe.Healthy
	var details []probe.Detail
	var rawValues []probe.RawValue
	for _, c := range t.Checkers {
		r := c.Check()
		if severity(r.Status) > severity(overall) {
			overall = r.Status
		}
		details = append(details, r.Details...)
		rawValues = append(rawValues, r.RawValues...)
	}
	return probe.Result{Name: t.Name, Status: overall, Details: details, RawValues: rawValues}
}

type Store struct {
	mu           sync.Mutex
	results      []probe.Result
	history      map[string]*history
	OnTransition func(name string, oldStatus, newStatus probe.Status)
}

func (s *Store) Set(results []probe.Result) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.results = results
	if s.history == nil {
		s.history = make(map[string]*history)
	}
	for _, r := range results {
		h, ok := s.history[r.Name]
		if !ok {
			h = &history{}
			s.history[r.Name] = h
			h.latencySum = make(map[string]time.Duration)
		}
		h.count++
		if r.Status == probe.Healthy {
			h.healthyCount++
		}
		for _, rv := range r.RawValues {
			h.latencySum[rv.Kind] += rv.Latency
		}
		// あとで、ここにalertingのための処理を追加する
		h.lastStatus = r.Status
	}
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
