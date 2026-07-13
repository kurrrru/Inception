FROM alpine:3.23

RUN apk add --no-cache mariadb mariadb-client

COPY tools/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

# MariaDBの設定ファイルを編集して、TCP/IP接続を有効にする
RUN sed -i 's/^skip-networking/#skip-networking/' /etc/my.cnf.d/mariadb-server.cnf

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
