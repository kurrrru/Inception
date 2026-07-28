FROM alpine:3.23

RUN apk add --no-cache redis

COPY conf/redis.conf /etc/redis/redis.conf
COPY tools/entryscript.sh /usr/local/bin/entryscript.sh
RUN chmod +x /usr/local/bin/entryscript.sh

ENTRYPOINT ["/usr/local/bin/entryscript.sh"]
