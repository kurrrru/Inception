#!/bin/sh
set -e

REDIS_PASSWORD=$(cat /run/secrets/redis_password)
echo "requirepass $REDIS_PASSWORD" > /run/redis_requirepass.conf

exec redis-server /etc/redis/redis.conf
