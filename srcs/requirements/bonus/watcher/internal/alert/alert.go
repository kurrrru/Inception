package alert

import (
	"bytes"
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"watcher/internal/config"
	"watcher/internal/probe"
)

type discordPayload struct {
	Content string `json:"content"`
}

func Notify(name string, oldStatus, newStatus probe.Status, webhook config.WebhookConfig) {
	if !webhook.Enabled {
		return
	}
	if oldStatus == newStatus {
		return
	}

	message := fmt.Sprintf("[watcher] %s: %s → %s", name, oldStatus.Label(), newStatus.Label())

	payload, err := json.Marshal(discordPayload{Content: message})
	if err != nil {
		log.Printf("alert: failed to build payload: %v", err)
		return
	}

	for _, url := range webhook.URLs {
		resp, err := http.Post(url, "application/json", bytes.NewReader(payload))
		if err != nil {
			log.Printf("alert: failed to send webhook to %s: %v", url, err)
			continue
		}
		resp.Body.Close()
	}
}
