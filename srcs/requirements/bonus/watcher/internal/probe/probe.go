package probe

import (
	"crypto/tls"
	"net"
	"net/http"
	"time"
)

type Detail struct {
	Label string
	Value string
}

type Result struct {
	Name    string
	Up      bool
	Details []Detail
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
		return Result{Name: c.Name, Up: false}
	}
	conn.Close()
	return Result{
		Name: c.Name,
		Up:   true,
		Details: []Detail{
			{Label: "応答時間", Value: latency.String()},
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
		return Result{Name: c.Name, Up: false}
	}
	defer resp.Body.Close()
	return Result{
		Name: c.Name,
		Up:   resp.StatusCode < 500,
		Details: []Detail{
			{Label: "応答時間", Value: latency.String()},
			{Label: "HTTPステータス", Value: http.StatusText(resp.StatusCode)},
		},
	}
}
