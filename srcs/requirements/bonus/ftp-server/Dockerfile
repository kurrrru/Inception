FROM alpine:3.23

RUN apk add --no-cache vsftpd openssl

COPY ./conf/vsftpd.conf /etc/vsftpd/vsftpd.conf

CMD ["vsftpd", "/etc/vsftpd/vsftpd.conf"]
