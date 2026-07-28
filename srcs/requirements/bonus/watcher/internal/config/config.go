package config

import (
	"os"

	"gopkg.in/yaml.v3"
)

type Config struct {
	Monitor MonitorConfig  `yaml:"monitor"`
	Default DefaultConfig  `yaml:"default"`
	Targets []TargetConfig `yaml:"targets"`
	Alert   AlertConfig    `yaml:"alert"`
	Auth    AuthConfig     `yaml:"auth"`
}

type AuthConfig struct {
	Enabled  bool   `yaml:"enabled"`
	Username string `yaml:"username"`
}

type MonitorConfig struct {
	Interval int `yaml:"interval_sec"`
}

type DefaultConfig struct {
	Targets DefaultTargetsConfig `yaml:"targets"`
}

type DefaultTargetsConfig struct {
	Timeout int `yaml:"timeout_sec"`
}

type TargetConfig struct {
	Name     string          `yaml:"name"`
	Checkers []CheckerConfig `yaml:"checkers"`
}

type CheckerConfig struct {
	Type    string `yaml:"type"`
	Address string `yaml:"address"`
	Timeout int    `yaml:"timeout_sec"`
}

type AlertConfig struct {
	Webhook WebhookConfig `yaml:"webhook"`
}

type WebhookConfig struct {
	Enabled bool     `yaml:"enabled"`
	URLs    []string `yaml:"urls"`
}

func Load(path string) (Config, error) {
	data, err := os.ReadFile(path)
	if err != nil {
		return Config{}, err
	}

	var cfg Config
	if err := yaml.Unmarshal(data, &cfg); err != nil {
		return Config{}, err
	}

	return cfg, nil
}
