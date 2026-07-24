package probe

type Status int

const (
	Healthy Status = iota
	Unhealthy
	Down
)

func (s Status) Label() string {
	switch s {
	case Healthy:
		return "正常に稼働中"
	case Unhealthy:
		return "異常あり"
	case Down:
		return "停止中"
	default:
		return "不明"
	}
}

func (s Status) CSSClass() string {
	switch s {
	case Healthy:
		return "healthy"
	case Unhealthy:
		return "unhealthy"
	case Down:
		return "down"
	default:
		return "unknown"
	}
}
