package main

import (
	"crypto/tls"
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
	srv := &http.Server{
		Addr:      ":8082",
		Handler:   nil,
		TLSConfig: &tls.Config{MinVersion: tls.VersionTLS12},
	}
	log.Fatal(srv.ListenAndServeTLS("/etc/watcher/ssl/watcher.crt", "/etc/watcher/ssl/watcher.key"))
}
