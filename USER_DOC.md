# User Documentation

## Overview of Provided Services

The core setup consists of the following containers. For the bonus part, see "Bonus Services".

| Service | Role |
|---|---|
| nginx | The only entry point, accepting access to the site over HTTPS (port 443) |
| wordpress | Runs WordPress itself (the blog / CMS) |
| mariadb | The database that stores WordPress data |

## Starting and Stopping the Project

To start:
```bash
make
```
On any machine where `docker` and `docker compose` are available, this alone builds and starts the three containers.

To stop:
```bash
make down
```
This stops the stack while leaving the data (under `/home/<login>/data`) in place. To remove it completely, run:
```bash
make fclean
```

## Accessing the Website and the Administration Panel

Only the first time, name resolution has to be made available to the browser.

```bash
make hosts
```
This appends `127.0.0.1 <login>.42.fr` to `/etc/hosts` (sudo is required).

After that, open the following in a browser:
- The site itself: `https://<login>.42.fr`
- The administration panel: `https://<login>.42.fr/wp-admin`

Because the certificate is self-signed, the browser warning has to be accepted (for example through "Advanced > Proceed").

## Locating and Managing Credentials

Passwords are stored as plain files under the `secrets/` directory of the repository (they are not included in Git).

| File | Purpose |
|---|---|
| `secrets/db_root_password.txt` | MariaDB root password |
| `secrets/db_password.txt` | Password of the database user used by WordPress |
| `secrets/wp_admin_password.txt` | Password of the WordPress administrator account |
| `secrets/wp_user_password.txt` | Password of the regular WordPress user |

Non-confidential values such as the administrator's user name are written in `srcs/.env`.

## Verifying That Services Are Running Correctly

```bash
docker ps                                              # are the 3 containers Up and stable
docker compose -f srcs/docker-compose.yml logs         # logs of each container
curl -k https://<login>.42.fr/ -o /dev/null -s -w "%{http_code}\n"   # does it return 200
```

## Bonus Services

### Redis

Redis runs behind the scenes as the WordPress cache. There is no screen for the user to interact with directly. To check that it works, run:

```bash
docker exec wordpress wp redis status --path=/var/www/html --allow-root
```

It is working correctly if `Status: Connected` is displayed.

### Adminer

Opening `https://<login>.42.fr:8443/` brings up a login screen.

| Field | Value |
|---|---|
| System | MySQL |
| Server | mariadb |
| Username | wp_user |
| Password | the contents of `secrets/db_password.txt` |
| Database | wordpress |

### reversi

Opening `https://<login>.42.fr:8081/` lets you play a game against the self-written Reversi AI in the browser. No login is needed.

### FTP Server

The files of the WordPress site can be accessed over FTP/FTPS.

| Field | Value |
|---|---|
| Host | `<login>.42.fr` |
| Port | 21 (passive port range: 30000-30009) |
| Username | ftp_user |
| Password | the contents of `secrets/ftp_password.txt` |
| Connection | FTPS (explicit AUTH TLS) |

### Watcher

Opening `https://<login>.42.fr:8082/` shows a list of the liveness, uptime ratio and average response time of each service (refreshed automatically every 5 seconds). No authentication is in place, so anyone can view it.

- `/api/status`: an API returning the same content in JSON
- `/health`: an endpoint for checking the liveness of Watcher itself
