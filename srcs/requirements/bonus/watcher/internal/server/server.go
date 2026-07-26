package server

import (
	"encoding/json"
	"html/template"
	"net/http"

	"watcher/internal/monitor"
	"watcher/internal/probe"
)

type TargetView struct {
	Name          string
	Status        probe.Status
	Details       []probe.Detail
	UptimePercent float64
	AvgLatency    map[string]string
}

type PageData struct {
	Title   string
	Results []TargetView
}

var tmpl = template.Must(template.ParseFiles("web/templates/index.html"))

func buildViews(store *monitor.Store) []TargetView {
	results := store.Get()
	summaries := store.Summaries()

	views := make([]TargetView, len(results))
	for i, r := range results {
		s := summaries[r.Name]
		avgLatency := make(map[string]string)
		for kind, d := range s.AvgLatency {
			avgLatency[kind] = d.String()
		}
		views[i] = TargetView{
			Name:          r.Name,
			Status:        r.Status,
			Details:       r.Details,
			UptimePercent: s.UptimePercent,
			AvgLatency:    avgLatency,
		}
	}
	return views
}

func IndexHandler(store *monitor.Store) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		tmpl.Execute(w, PageData{Title: "Watcher", Results: buildViews(store)})
	}
}

func StatusAPIHandler(store *monitor.Store) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(buildViews(store))
	}
}

func HealthCheckHandler() http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.WriteHeader(http.StatusOK) // 200 OK
		w.Write([]byte("OK"))
	}
}
