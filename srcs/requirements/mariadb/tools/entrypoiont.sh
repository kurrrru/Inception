#!/bin/sh
set -e

DATADIR=/var/lib/mysql
SOCKET=/run/mysqld/mysqld.sock

mkdir -p /run/mysqld
chown mysql:mysql /run/mysqld

DB_PASSWORD=$(cat /run/secrets/db_password)
DB_ROOT_PASSWORD=$(cat /run/secrets/db_root_password)

if [ ! -d "$DATADIR/mysql" ]; then
    echo "[entrypoint] First run: initializing data directory..."
    mariadb-install-db --user=mysql --datadir="$DATADIR" --skip-test-db

    echo "[entrypoint] Starting temporary server for setup..."
    mariadbd --user=mysql --datadir="$DATADIR" --skip-networking --socket="$SOCKET" &
    TMP_PID=$!

    until mariadb-admin --socket="$SOCKET" ping >/dev/null 2>&1; do
        sleep 1
    done

    mariadb --socket="$SOCKET" -u root <<-EOSQL
        CREATE DATABASE IF NOT EXISTS \`${MYSQL_DATABASE}\`;
        CREATE USER IF NOT EXISTS '${MYSQL_USER}'@'%' IDENTIFIED BY '${DB_PASSWORD}';
        GRANT ALL PRIVILEGES ON \`${MYSQL_DATABASE}\`.* TO '${MYSQL_USER}'@'%';
        ALTER USER 'root'@'localhost' IDENTIFIED BY '${DB_ROOT_PASSWORD}';
        FLUSH PRIVILEGES;
EOSQL

    echo "[entrypoint] Stopping temporary server..."
    mariadb-admin --socket="$SOCKET" -uroot -p"${DB_ROOT_PASSWORD}" shutdown
    wait "$TMP_PID"
    echo "[entrypoint] Setup complete."
fi

echo "[entrypoint] Starting mariadbd (foreground)..."
exec mariadbd --user=mysql --datadir="$DATADIR"
