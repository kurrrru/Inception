FROM alpine:3.23 AS builder

RUN apk add --no-cache go

WORKDIR /app
COPY go.mod ./
COPY cmd/ ./cmd/
COPY internal/ ./internal/
RUN CGO_ENABLED=0 go build -o watcher ./cmd/watcher

FROM alpine:3.23

ARG DOMAIN_NAME

RUN apk add --no-cache openssl

RUN mkdir -p /etc/watcher/ssl && \
    openssl req -x509 -nodes -days 365 \
        -newkey rsa:2048 \
        -keyout /etc/watcher/ssl/watcher.key \
        -out /etc/watcher/ssl/watcher.crt \
        -subj "/C=JP/ST=Tokyo/L=Tokyo/O=42/OU=42/CN=${DOMAIN_NAME}"

COPY --from=builder /app/watcher /usr/local/bin/watcher
COPY web/ /app/web/
WORKDIR /app

CMD ["watcher"]
