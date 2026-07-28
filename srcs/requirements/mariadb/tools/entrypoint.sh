#!/bin/sh
set -e

DATADIR=/var/lib/mysql
INIT_FILE=/run/mysqld/init.sql

mkdir -p /run/mysqld
chown mysql:mysql /run/mysqld

DB_PASSWORD=$(cat /run/secrets/db_password)
DB_ROOT_PASSWORD=$(cat /run/secrets/db_root_password)

if [ ! -d "$DATADIR/mysql" ]; then
    echo "[entrypoint] First run: initializing data directory..."
    mariadb-install-db --user=mysql --datadir="$DATADIR" --skip-test-db
    echo "[entrypoint] Data directory initialized."
fi

# 初期化SQLは --init-file で本体の起動時に渡す
cat > "$INIT_FILE" <<-EOSQL
	CREATE DATABASE IF NOT EXISTS \`${MYSQL_DATABASE}\`;
	CREATE USER IF NOT EXISTS '${MYSQL_USER}'@'%' IDENTIFIED BY '${DB_PASSWORD}';
	GRANT ALL PRIVILEGES ON \`${MYSQL_DATABASE}\`.* TO '${MYSQL_USER}'@'%';
	ALTER USER 'root'@'localhost' IDENTIFIED BY '${DB_ROOT_PASSWORD}';
	FLUSH PRIVILEGES;
EOSQL
chown mysql:mysql "$INIT_FILE"
chmod 600 "$INIT_FILE"

echo "[entrypoint] Starting mariadbd (foreground)..."
exec mariadbd --user=mysql --datadir="$DATADIR" --init-file="$INIT_FILE"
