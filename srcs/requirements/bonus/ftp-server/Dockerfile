FROM alpine:3.23

ARG DOMAIN_NAME

RUN apk add --no-cache vsftpd openssl

RUN mkdir -p /etc/vsftpd/ssl && \
    openssl req -x509 -nodes -days 365 \
        -newkey rsa:2048 \
        -keyout /etc/vsftpd/ssl/ftp.key \
        -out /etc/vsftpd/ssl/ftp.crt \
        -subj "/C=JP/ST=Tokyo/L=Tokyo/O=42/OU=42/CN=${DOMAIN_NAME}"

RUN adduser -D -H -h /var/www/html -s /bin/sh ftp_user && \
    sed -i 's/^ftp_user:x:[0-9]*:[0-9]*:/ftp_user:x:65534:65534:/' /etc/passwd

COPY ./conf/vsftpd.conf /etc/vsftpd/vsftpd.conf
COPY ./tools/entrypoint.sh /usr/local/bin/entrypoint.sh
COPY ./conf/pam.d/vsftpd /etc/pam.d/vsftpd
RUN chmod +x /usr/local/bin/entrypoint.sh

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
