FROM alpine:3.23

RUN apk add --no-cache vsftpd openssl

RUN mkdir -p /etc/vsftpd/ssl && \
    openssl req -x509 -nodes -days 365 \
        -newkey rsa:2048 \
        -keyout /etc/vsftpd/ssl/ftp.key \
        -out /etc/vsftpd/ssl/ftp.crt \
        -subj "/C=JP/ST=Tokyo/L=Tokyo/O=42/OU=42/CN=ftp"

COPY ./conf/vsftpd.conf /etc/vsftpd/vsftpd.conf

CMD ["vsftpd", "/etc/vsftpd/vsftpd.conf"]
