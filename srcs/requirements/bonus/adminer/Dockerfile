FROM alpine:3.23

RUN apk add --no-cache wget php83 php83-fpm php83-session php83-mysqli && \
    ln -sf /usr/bin/php83 /usr/bin/php
RUN mkdir -p /var/www/html
RUN wget https://www.adminer.org/latest-ja.php -O /var/www/html/adminer.php

RUN sed -i 's/^listen = 127.0.0.1:9000$/listen = 9000/' /etc/php83/php-fpm.d/www.conf

CMD ["php-fpm83", "-F"]
