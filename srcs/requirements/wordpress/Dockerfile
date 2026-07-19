FROM alpine:3.23

# WordPressの推奨環境に合わせて、PHPと必要な拡張モジュールをインストールする
# 参照: https://make.wordpress.org/hosting/handbook/server-environment/
RUN apk add --no-cache \
        php83 php83-fpm php83-mysqli \
        php83-curl php83-dom php83-exif php83-fileinfo php83-intl php83-mbstring \
        php83-openssl php83-xml php83-zip php83-tokenizer \
        php83-pecl-igbinary php83-pecl-imagick imagemagick \
        php83-opcache php83-pecl-redis \
        php83-phar curl tar mariadb-client redis && \
    ln -sf /usr/bin/php83 /usr/bin/php

# WordPress CLIをインストールする
# 参照: https://make.wordpress.org/cli/handbook/guides/installing/
RUN curl -fsSL -o /usr/local/bin/wp \
        https://raw.githubusercontent.com/wp-cli/builds/gh-pages/phar/wp-cli.phar && \
    chmod +x /usr/local/bin/wp

# WordPressのコアファイルをダウンロードする
# 参照: https://developer.wordpress.org/cli/commands/core/download/
RUN mkdir -p /usr/src/wordpress && \
    php -d memory_limit=-1 /usr/local/bin/wp --allow-root core download --path=/usr/src/wordpress

# 別コンテナのnginxから接続できるよう、php-fpmのlisten先をloopback限定から全interfaceに変更
RUN sed -i 's/^listen = 127.0.0.1:9000$/listen = 9000/' /etc/php83/php-fpm.d/www.conf

# entrypoint.shをコピーして実行権限を付与する
COPY tools/entrypoint.sh /usr/local/bin/entrypoint.sh
RUN chmod +x /usr/local/bin/entrypoint.sh

ENTRYPOINT ["/usr/local/bin/entrypoint.sh"]
