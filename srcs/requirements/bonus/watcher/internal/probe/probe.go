package probe

import (
	"crypto/tls"
	"net"
	"net/http"
	"time"
)

type RawValue struct {
	Kind    string
	Latency time.Duration
}

type Detail struct {
	Label string
	Value string
}

type Result struct {
	Name      string
	Status    Status
	Details   []Detail
	RawValues []RawValue
}

type Checker interface {
	Check() Result
}

type TCPChecker struct {
	Name    string
	Addr    string
	Timeout time.Duration
}

func (c TCPChecker) Check() Result {
	start := time.Now()
	conn, err := net.DialTimeout("tcp", c.Addr, c.Timeout)
	latency := time.Since(start)
	if err != nil {
		return Result{Name: c.Name, Status: Unhealthy}
	}
	conn.Close()
	return Result{
		Name:   c.Name,
		Status: Healthy,
		Details: []Detail{
			{Label: "TCP応答時間", Value: latency.String()},
		},
		RawValues: []RawValue{
			{Kind: "tcp", Latency: latency},
		},
	}
}

type HTTPChecker struct {
	Name    string
	URL     string
	Timeout time.Duration
}

func (c HTTPChecker) Check() Result {
	client := http.Client{
		Timeout: c.Timeout,
		Transport: &http.Transport{
			// inceptionネットワーク内部の自己署名証明書向け。外部への接続はしないため許容
			TLSClientConfig: &tls.Config{InsecureSkipVerify: true},
		},
	}
	start := time.Now()
	resp, err := client.Get(c.URL)
	latency := time.Since(start)
	if err != nil {
		return Result{Name: c.Name, Status: Unhealthy}
	}
	defer resp.Body.Close()
	return Result{
		Name: c.Name,
		Status: func() Status {
			if resp.StatusCode < 500 {
				return Healthy
			}
			return Unhealthy
		}(),
		Details: []Detail{
			{Label: "HTTP応答時間", Value: latency.String()},
			{Label: "HTTPステータス", Value: http.StatusText(resp.StatusCode)},
		},
		RawValues: []RawValue{
			{Kind: "http", Latency: latency},
		},
	}
}
