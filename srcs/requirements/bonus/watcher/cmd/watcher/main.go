package main

import (
	"fmt"
	"log"
	"net/http"
)

func main() {
	http.HandleFunc("/", func(w http.ResponseWriter, r *http.Request) {
		fmt.Fprintln(w, "watcher: ok")
	})
	log.Println("listening on :8082 (TLS)")
	log.Fatal(http.ListenAndServeTLS(":8082", "/etc/watcher/ssl/watcher.crt", "/etc/watcher/ssl/watcher.key", nil))
}
