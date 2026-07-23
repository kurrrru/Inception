FROM alpine:3.23 AS builder

RUN apk add --no-cache go

WORKDIR /app
COPY go.mod ./
COPY cmd/ ./cmd/
RUN CGO_ENABLED=0 go build -o watcher ./cmd/watcher

FROM alpine:3.23

COPY --from=builder /app/watcher /usr/local/bin/watcher

CMD ["watcher"]
