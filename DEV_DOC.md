# Developer Documentation

## Prerequisites

- Docker Engine + Docker Compose plugin
- A Linux OS on which sudo can be used

## Configuration Files and Secrets

A plain `git clone` is not enough to run the project. The following two kinds of files are not included in Git (`.gitignore`) and have to be created by hand.

**`srcs/.env`** (environment variable settings):
```
MYSQL_DATABASE=wordpress
MYSQL_USER=wp_user
WP_TITLE=Inception
WORDPRESS_DB_HOST=mariadb
WORDPRESS_DB_NAME=wordpress
WORDPRESS_DB_USER=wp_user
WP_ADMIN_USER=<user name of the administrator>
WP_ADMIN_EMAIL=<email address of the administrator>
WP_USER=<user name of the regular user>
WP_USER_EMAIL=<email address of the regular user>
```
`DOMAIN_NAME` and `DATA_PATH` are derived automatically by the Makefile from `whoami`, so they are not written in `.env`.

**`secrets/*.txt`** (passwords, saved without a trailing newline):

The following passwords have to be set:
- `secrets/db_root_password.txt`: password of the MariaDB root user
- `secrets/db_password.txt`: password of wp_user, the WordPress-specific database account inside MariaDB
- `secrets/wp_admin_password.txt`: password of the administrator account of the WordPress site
- `secrets/wp_user_password.txt`: password of the regular user of the WordPress site

## Building and Launching the Project (Makefile & Docker Compose)

```bash
make          # create_data_dir → build → up (starts the core stack only)
make clean    # remove containers, images and volume definitions
make fclean   # clean + also remove the real data on the host (/home/<login>/data)
make re       # fclean + all
```

Internally the `Makefile` computes and `export`s `LOGIN` / `DATA_PATH` / `DOMAIN_NAME`, and then invokes `docker compose -f srcs/docker-compose.yml build/up`.

## Managing Containers and Volumes

```bash
docker compose -f srcs/docker-compose.yml ps        # status
docker exec -it wordpress sh                        # get a shell inside a container
docker exec wordpress wp user list --path=/var/www/html --allow-root   # list WordPress users
docker volume ls                                     # list volumes
```

## Data Storage and Persistence

The WordPress files and the MariaDB data are managed as named volumes (`wordpress_vol`, `mariadb_vol`), but `driver_opts` pins their actual location on the host to `/home/<login>/data/wordpress_vol` and `/home/<login>/data/mariadb_vol`.

**Note**: `docker compose down -v` only removes the volume definitions from Docker; it does not delete the contents of those host directories. This is intentional — the data survives recreating the containers. To really start over from scratch, the host directories have to be removed explicitly.

```bash
sudo rm -rf /home/$(whoami)/data/wordpress_vol
sudo rm -rf /home/$(whoami)/data/mariadb_vol
```

## Bonus Services

Each bonus service is placed under `srcs/requirements/bonus/<service>/` and defined in `srcs/docker-compose.bonus.yml`. Keeping it in a file separate from the core setup means it has no effect whatsoever on the behaviour of `make` (core stack only).

```bash
make bonus       # build and start everything, core + bonus
make down        # stop everything, core + bonus (safe to use when the bonus stack is running)
```

The `Makefile` keeps `COMPOSE_FILE` (for the core stack) and `COMPOSE_FILE_BONUS` (for the bonus differences) separate, and the bonus targets invoke `docker compose` with both stacked using `-f`.

### Publishing Scheme

Switching the `NGINX_CONF` build argument of the core nginx container makes it load `nginx.bonus.conf`, a configuration that contains the additional `server` blocks for the bonus services. Adminer (port 8443) and reversi (port 8081) are published this way. Redis (internal only, no published port) and Watcher (published directly on port 8082) are deliberate exceptions to this scheme.

### Redis

- Location: `srcs/requirements/bonus/redis/`
- No dedicated volume (cached data can be regenerated from the source of truth)
- `bind` is relaxed and, in its place, password authentication is used via `requirepass` (`secrets/redis_password.txt`)
- On the WordPress side, `entrypoint.sh` branches on the `USE_REDIS` environment variable and runs `wp redis enable` and related commands only when the bonus stack is started (`make bonus`)

### Adminer

- Location: `srcs/requirements/bonus/adminer/` (php-fpm only, no web server bundled)
- Forwarded from nginx with `fastcgi_pass adminer:9000`. `try_files` is unnecessary because the tool is a single PHP file
- No secrets are used (the connection details are typed into the browser each time by design)

### reversi

- Location: `srcs/requirements/bonus/reversi/`
- Multi-stage build: the builder stage installs emsdk by hand on Alpine and compiles the C++ sources to WebAssembly. The final stage contains only the generated `.wasm` / `.js` and the static files
- Serving is done by `python3 -m http.server`. External publishing goes through nginx's `proxy_pass http://reversi:8000;`

### FTP Server

- Location: `srcs/requirements/bonus/ftp-server/`
- Uses vsftpd. `port_enable=NO` disables active mode (the root cause of the bounce attack) and allows passive mode only
- `ssl_enable=YES` provides FTPS (self-signed certificate, generated with the same pattern as the core nginx)
- The only user is `ftp_user` (UID/GID 65534, matching `nobody`, so that the access rights to `/var/www/html` line up)
- The password comes from `secrets/ftp_password.txt` and is applied by `entrypoint.sh` when the container starts

### Watcher

- Location: `srcs/requirements/bonus/watcher/` (written in Go)
- Multi-stage build: the builder stage produces a static binary with `go build`, and only that binary is copied into the final stage
- Monitoring targets, check interval, timeouts and notification settings are managed in `config/watcher.yml` (YAML) and read by the `internal/config` package
- Liveness is determined solely by network probing — a TCP connection check or an HTTP GET. Access to the Docker API through `docker.sock` is not used (see the "Bonus" section of the README for the reasoning)
- The uptime ratio (cumulative %) of each target and the average response time per checker type are aggregated and displayed
- When a status changes, a notification is sent to a Discord Webhook (enabled, together with the destination URL, in `alert.webhook` of `config/watcher.yml`)
- Authentication is not implemented (as the "service of choice" of the bonus part, the other work — configuration-driven targets, uptime aggregation and alert notification — was prioritized instead)
