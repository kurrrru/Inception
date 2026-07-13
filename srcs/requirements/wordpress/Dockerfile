FROM alpine:3.23

# WordPressの推奨環境に合わせて、PHPと必要な拡張モジュールをインストールする
# 参照: https://make.wordpress.org/hosting/handbook/server-environment/
RUN apk add --no-cache \
        php83 php83-fpm php83-mysqli \
        php83-curl php83-dom php83-exif php83-fileinfo php83-intl php83-mbstring \
        php83-openssl php83-xml php83-zip \
        php83-pecl-igbinary php83-pecl-imagick imagemagick \
        php83-opcache php83-pecl-redis \
        php83-phar curl tar mariadb-client && \
    ln -sf /usr/bin/php83 /usr/bin/php

# ↓ 仮置き。後で置き換える
CMD ["tail", "-f", "/dev/null"]
