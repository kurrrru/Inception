package server

import (
	"encoding/json"
	"html/template"
	"net/http"

	"watcher/internal/monitor"
	"watcher/internal/probe"
)

type PageData struct {
	Title   string
	Results []probe.Result
}

var tmpl = template.Must(template.ParseFiles("web/templates/index.html"))

func IndexHandler(store *monitor.Store) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		tmpl.Execute(w, PageData{Title: "Watcher", Results: store.Get()})
	}
}

func StatusAPIHandler(store *monitor.Store) http.HandlerFunc {
	return func(w http.ResponseWriter, r *http.Request) {
		w.Header().Set("Content-Type", "application/json")
		json.NewEncoder(w).Encode(store.Get())
	}
}
