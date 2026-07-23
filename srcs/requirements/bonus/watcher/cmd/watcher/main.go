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
	log.Println("listening on :8082")
	log.Fatal(http.ListenAndServe(":8082", nil))
}
