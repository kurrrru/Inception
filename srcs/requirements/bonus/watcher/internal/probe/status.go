package probe

type Status string

const (
	Healthy   Status = "healthy"
	Unhealthy Status = "unhealthy"
	Down      Status = "down"
	Unknown   Status = "unknown"
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
	case Healthy, Unhealthy, Down:
		return string(s)
	default:
		return "unknown"
	}
}
