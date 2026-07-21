#!/bin/sh
set -e

FTP_PASSWORD=$(cat /run/secrets/ftp_password)
echo "ftp_user:${FTP_PASSWORD}" | chpasswd

exec vsftpd /etc/vsftpd/vsftpd.conf
