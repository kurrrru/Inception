FROM alpine:3.23 AS builder

RUN apk add --no-cache go

WORKDIR /app
COPY go.mod ./
COPY cmd/ ./cmd/
RUN CGO_ENABLED=0 go build -o watcher ./cmd/watcher

FROM alpine:3.23

RUN apk add --no-cache openssl

RUN mkdir -p /etc/watcher/ssl && \
    openssl req -x509 -nodes -days 365 \
        -newkey rsa:2048 \
        -keyout /etc/watcher/ssl/watcher.key \
        -out /etc/watcher/ssl/watcher.crt \
        -subj "/C=JP/ST=Tokyo/L=Tokyo/O=42/OU=42/CN=watcher"

COPY --from=builder /app/watcher /usr/local/bin/watcher

CMD ["watcher"]
