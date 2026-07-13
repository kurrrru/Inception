#!/bin/sh
set -e

DB_PASSWORD=$(cat /run/secrets/db_password)

until mariadb-admin -h "${WORDPRESS_DB_HOST}" -u "${MYSQL_USER}" -p"${DB_PASSWORD}" ping >/dev/null 2>&1; do
    echo "[entrypoint] Waiting for MariaDB..."
    sleep 1
done

echo "[entrypoint] MariaDB is ready."

if [ ! -f /var/www/html/wp-load.php ]; then
    echo "[entrypoint] First run: copying WordPress core to /var/www/html..."
    cp -a /usr/src/wordpress/. /var/www/html/
    chown -R nobody:nobody /var/www/html
    echo "[entrypoint] Core files copied."

    echo "[entrypoint] Creating wp-config.php..."
    wp config create \
        --path=/var/www/html \
        --dbname="${WORDPRESS_DB_NAME}" \
        --dbuser="${WORDPRESS_DB_USER}" \
        --dbpass="${DB_PASSWORD}" \
        --dbhost="${WORDPRESS_DB_HOST}" \
        --allow-root
    echo "[entrypoint] wp-config.php created."

    WP_ADMIN_PASSWORD=$(cat /run/secrets/wp_admin_password)
    echo "[entrypoint] Installing WordPress core..."
    wp core install \
        --path=/var/www/html \
        --url="https://${DOMAIN_NAME}" \
        --title="${WP_TITLE}" \
        --admin_user="${WP_ADMIN_USER}" \
        --admin_password="${WP_ADMIN_PASSWORD}" \
        --admin_email="${WP_ADMIN_EMAIL}" \
        --skip-email \
        --allow-root
    echo "[entrypoint] WordPress core installed."

    WP_USER_PASSWORD=$(cat /run/secrets/wp_user_password)
    echo "[entrypoint] Creating second (non-admin) user..."
    wp user create \
        "${WP_USER}" "${WP_USER_EMAIL}" \
        --path=/var/www/html \
        --role=author \
        --user_pass="${WP_USER_PASSWORD}" \
        --allow-root
    echo "[entrypoint] Second user created."
else
    echo "[entrypoint] WordPress already present, skipping copy."
fi

echo "[entrypoint] Starting php-fpm (foreground)..."
exec php-fpm83 -F
