FROM alpine:3.23

RUN apk add --no-cache mariadb mariadb-client

# ↓ 仮置き。Step 4 で entrypoint スクリプト経由の起動に置き換える
CMD ["tail", "-f", "/dev/null"]
