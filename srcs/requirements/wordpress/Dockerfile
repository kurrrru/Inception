FROM alpine:3.23

RUN apk add --no-cache php83 php83-fpm && \
    ln -sf /usr/bin/php83 /usr/bin/php

# ↓ 仮置き。後で置き換える
CMD ["tail", "-f", "/dev/null"]
