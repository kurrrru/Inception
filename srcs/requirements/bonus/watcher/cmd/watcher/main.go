package main

import (
	"crypto/tls"
	"log"
	"net/http"
	"os"
	"strings"
	"time"

	"watcher/internal/alert"
	"watcher/internal/config"
	"watcher/internal/monitor"
	"watcher/internal/probe"
	"watcher/internal/server"
)

func main() {
	cfg, err := config.Load("/etc/watcher/watcher.yml")
	if err != nil {
		log.Fatalf("failed to load config: %v", err)
	}

	interval := time.Duration(cfg.Monitor.Interval) * time.Second
	targets := make([]monitor.Target, len(cfg.Targets))
	for i, target := range cfg.Targets {
		targets[i] = monitor.Target{
			Name:     target.Name,
			Checkers: make([]probe.Checker, len(target.Checkers)),
		}
		for j, checker := range target.Checkers {
			if checker.Timeout == 0 {
				checker.Timeout = cfg.Default.Targets.Timeout
			}
			switch checker.Type {
			case "tcp":
				targets[i].Checkers[j] = probe.TCPChecker{
					Name:    targets[i].Name,
					Addr:    checker.Address,
					Timeout: time.Duration(checker.Timeout) * time.Second,
				}
			case "http":
				targets[i].Checkers[j] = probe.HTTPChecker{
					Name:    targets[i].Name,
					URL:     checker.Address,
					Timeout: time.Duration(checker.Timeout) * time.Second,
				}
			}
		}
	}

	store := &monitor.Store{
		OnTransition: func(name string, oldStatus, newStatus probe.Status) {
			alert.Notify(name, oldStatus, newStatus, cfg.Alert.Webhook)
		},
	}
	go monitor.Start(store, targets, interval)

	var authPassword string
	if cfg.Auth.Enabled {
		b, err := os.ReadFile("/run/secrets/watcher_password")
		if err != nil {
			log.Fatalf("failed to read watcher password: %v", err)
		}
		authPassword = strings.TrimSpace(string(b))
	}

	protect := func(h http.HandlerFunc) http.HandlerFunc {
		if !cfg.Auth.Enabled {
			return h
		}
		return server.BasicAuth(cfg.Auth.Username, authPassword, h)
	}

	http.HandleFunc("/", protect(server.IndexHandler(store)))
	http.HandleFunc("/api/status", protect(server.StatusAPIHandler(store)))
	http.HandleFunc("/health", server.HealthCheckHandler())

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
