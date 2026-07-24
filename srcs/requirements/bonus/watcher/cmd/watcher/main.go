package main

import (
	"log"
	"net/http"
	"time"

	"watcher/internal/monitor"
	"watcher/internal/server"
)

func main() {
	store := &monitor.Store{}
	go monitor.Start(store, monitor.Targets, 5*time.Second)

	http.HandleFunc("/", server.IndexHandler(store))
	http.HandleFunc("/api/status", server.StatusAPIHandler(store))

	fs := http.FileServer(http.Dir("web/static"))
	http.Handle("/static/", http.StripPrefix("/static/", fs))

	log.Println("listening on :8082 (TLS)")
	log.Fatal(http.ListenAndServeTLS(":8082", "/etc/watcher/ssl/watcher.crt", "/etc/watcher/ssl/watcher.key", nil))
}
