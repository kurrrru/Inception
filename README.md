*This project has been created as part of the 42 curriculum by nkawaguc.*

# Inception

[![stack](https://github.com/kurrrru/Inception/actions/workflows/stack.yml/badge.svg)](https://github.com/kurrrru/Inception/actions/workflows/stack.yml)
[![docs](https://github.com/kurrrru/Inception/actions/workflows/docs.yml/badge.svg)](https://github.com/kurrrru/Inception/actions/workflows/docs.yml)
[![dockerfile-lint](https://github.com/kurrrru/Inception/actions/workflows/dockerfile-lint.yml/badge.svg)](https://github.com/kurrrru/Inception/actions/workflows/dockerfile-lint.yml)
[![secret-scan](https://github.com/kurrrru/Inception/actions/workflows/secret-scan.yml/badge.svg)](https://github.com/kurrrru/Inception/actions/workflows/secret-scan.yml)
[![final-newline](https://github.com/kurrrru/Inception/actions/workflows/final-newline.yml/badge.svg)](https://github.com/kurrrru/Inception/actions/workflows/final-newline.yml)
[![forbidden-patterns](https://github.com/kurrrru/Inception/actions/workflows/forbidden-patterns.yml/badge.svg)](https://github.com/kurrrru/Inception/actions/workflows/forbidden-patterns.yml)
[![compose-validate](https://github.com/kurrrru/Inception/actions/workflows/compose-validate.yml/badge.svg)](https://github.com/kurrrru/Inception/actions/workflows/compose-validate.yml)

## Description

Inception is a project that builds a web infrastructure using Docker, with the goal of deepening knowledge of system administration.
Its core setup uses Docker Compose to connect three containers — nginx, WordPress and MariaDB — so that together they host a WordPress site. The services added as bonus are described in the "Bonus" section.

- **nginx**: the only external entry point, accepting HTTPS (port 443) over TLSv1.2/1.3 only
- **WordPress**: a container that runs WordPress itself through PHP-FPM
- **MariaDB**: a container that holds the database used by WordPress

### Docker Usage and Included Sources

None of the services use a ready-made Docker image. Each one is built from a Dockerfile written by hand on top of `alpine:3.23`.
Each service keeps its own Dockerfile and configuration files (nginx.conf, entrypoint.sh and so on) under `srcs/requirements/<service>/`, and `srcs/docker-compose.yml` orchestrates them. Building and lifecycle management are driven from the `Makefile` at the repository root.

### Main Design Choices

- **Idempotency**: any step that performs initialization or changes configuration is designed to be safe to run any number of times.
- **Container lifetime = service lifetime**: no keep-alive tricks (dummy processes and the like) are used to hold a container open. A container is designed to live exactly as long as the service process it exists for.
- **Minimal exposure**: nothing is made reachable from the outside while it is not yet fully prepared.
- **No hardcoded values**: values specific to an environment or a user are derived at build time or at run time rather than being written into the sources.

### Comparisons

#### Virtual Machines vs Docker

VMs and containers offer similar benefits in terms of resource isolation, but they achieve them in different ways. A VM virtualizes hardware and runs as a complete operating system with its own kernel, drivers and full OS image, which makes it heavy when all that is wanted is to isolate a single application.
A container virtualizes the OS instead: it runs as an isolated process containing only the files needed to execute, and several containers share the same host kernel, so more applications can run on fewer resources. This project follows the same idea — the three services are separated into lightweight containers, and a single VM is used only as the ground on which Docker itself runs.

The two also sit at different layers from a theoretical standpoint. A virtual machine runs on top of a piece of software called a virtual machine monitor (VMM, also known as a hypervisor). Popek and Goldberg (1974) defined three properties a VMM must satisfy in order to be considered a correct implementation of virtualization:

- **Equivalence**: a program running under the VMM must behave essentially as it would if installed and run directly on real hardware
- **Resource control**: the VMM must be in complete control of the resources it allocates, such as CPU and memory
- **Efficiency**: the vast majority of instructions must execute directly on the real CPU without going through the VMM (otherwise performance collapses)

Containers (Docker) have no such VMM layer at all. They share the host kernel directly and isolate processes using **namespaces** (the mechanism that isolates which resources a process can see) and **cgroups** (the mechanism that limits per-process usage of CPU, memory and so on). In other words, containers achieve isolation by a fundamentally different mechanism than the "virtualization through a VMM" that Popek and Goldberg were reasoning about, and so they fall outside that theoretical framework.

#### Secrets vs Environment Variables

The official Docker Compose documentation states that environment variables are unsuitable for confidential values, because they are easily stored and read in plain text and may be visible through `docker inspect` or in logs. Secrets are mounted inside the container as files under `/run/secrets/<name>`, which exposes them less than environment variables do. In this project every password is handled through secrets, and only non-confidential configuration is placed in `.env`.

#### Docker Network vs Host Network

On a Docker bridge network, each container has its own network namespace (the Linux kernel mechanism that isolates network interfaces, IP addresses, routing tables and ports on a per-process basis), and containers attached to the same user-defined network can reach one another by container name through DNS. On the host network a container shares the host's network namespace, isolation is lost, and port mapping with `-p` no longer applies. This project creates its own bridge network (`inception`), has the containers talk to each other over internal DNS using service names, and exposes nothing to the outside except nginx on port 443.

#### Docker Volumes vs Bind Mounts

A volume is persistent storage managed by the Docker daemon; the data physically lives on the host, but inside Docker's managed area. A bind mount links an arbitrary host path directly into the container, so that processes outside Docker and the container itself can both access and modify the files at the same time. This project uses a hybrid of the two: named volumes whose actual location is pinned under `/home/<login>/data` on the host by means of `driver_opts`.

## Bonus

Five additional services are provided on top of the core setup.

### Overview

| Service | Role |
|---|---|
| Redis | Object cache for WordPress |
| Adminer | Browser-based database management GUI |
| reversi | A static site running a self-written C++ Reversi AI compiled to WebAssembly |
| FTP Server | An FTP/FTPS server giving access to the WordPress site volume |
| Watcher | A self-written health monitoring dashboard in Go |

### Main Design Choices

- **A single publishing scheme**: the core nginx container is the only TLS termination point. Adminer and reversi are published through dedicated `server` blocks in `nginx.bonus.conf`, which is selected via the `NGINX_CONF` build argument. Redis (internal only) and Watcher (published directly on its own port) are deliberate exceptions to this scheme.
- **Redis**: limited to caching for WordPress. It is given no dedicated volume, since its data can always be regenerated from the source of truth (MariaDB). The default `bind 127.0.0.1` cannot be reached from other containers, so the binding is relaxed and, in exchange, password authentication through `requirepass` is made mandatory. The WordPress side branches on a `USE_REDIS` environment flag and is enabled only when the bonus stack is started, so that starting without the bonus (`make` alone) is never broken.
- **Adminer**: since the tool is a single self-contained PHP file, it is given no web server of its own. Instead requests are forwarded over FastCGI from the core nginx straight to php-fpm. No secrets are given to the container, because the credentials are typed into the browser login form each time.
- **reversi (the simple static website)**: a self-written C++ Reversi AI compiled to WebAssembly with Emscripten and served as a static site that runs entirely in the browser. The heavy toolchain needed only at build time (the whole Emscripten SDK) is kept out of the final image by a multi-stage build. Serving itself is done by `python3 -m http.server`, and external publishing is unified under a `proxy_pass` from the core nginx.
- **FTP Server**: an FTP/FTPS server with direct access to the WordPress volume. Following the mitigation recommended by RFC 2577 (FTP Security Considerations), the `PORT` command (active mode) — the root cause of the bounce attack — is disabled, and only passive mode is allowed. FTPS is supported as well, encrypting both the control and the data connection with TLS.
- **Watcher (the service of choice)**: a self-written dashboard in Go that visualizes the liveness, uptime ratio and average response time of each container. Monitoring targets, check interval and notification settings are managed in an external YAML file and can be changed without rebuilding the code. The authoritative way to obtain a container's actual run state (reaching the Docker API through `docker.sock`) was considered and rejected: it effectively hands the container full control over the host's Docker, which is too large a security risk, and it does not justify itself at the scale of this project where Watcher would be its only consumer. Because of that, the status display is limited to "running normally" and "problem detected (or stopped)", and it never asserts that a container is stopped on the basis of network-level probing alone. The dashboard and the JSON API are protected with Basic authentication, and the password is supplied through a Docker secret (the connection itself is TLS with a self-signed certificate, so the credentials never travel in plain text). Only `/health` is left outside authentication, so that external monitoring tools can reach it without credentials.

### Running the Bonus Services

```bash
make bonus     # build and start everything (core + bonus)
```

See USER_DOC for how to reach each service.

## Instructions

```bash
make            # build and start the core stack, and add the domain to /etc/hosts
```

Open `https://<login>.42.fr` in a browser inside the VM.

## Resources

- Alpine Linux Wiki: MariaDB — https://wiki.alpinelinux.org/wiki/MariaDB
- WP-CLI Installing guide — https://make.wordpress.org/cli/handbook/guides/installing/
- WordPress.org Requirements — https://wordpress.org/about/requirements/
- WordPress Hosting Handbook: Server Environment — https://make.wordpress.org/hosting/handbook/handbook/server-environment/
- WP-CLI Command Reference: core download — https://developer.wordpress.org/cli/commands/core/download/
- What is a container? — https://docs.docker.com/get-started/docker-concepts/the-basics/what-is-a-container/
- Popek and Goldberg virtualization requirements — https://en.wikipedia.org/wiki/Popek_and_Goldberg_virtualization_requirements
- Formal Requirements for Virtualizable Third Generation Architectures (Popek & Goldberg, 1974) — https://www.cs.cornell.edu/courses/cs6411/2018sp/papers/popek-goldberg.pdf
- Secrets in Compose — https://docs.docker.com/compose/how-tos/use-secrets/
- Networking overview / Host network driver — https://docs.docker.com/engine/network/ , https://docs.docker.com/engine/network/drivers/host/
- Storage — https://docs.docker.com/engine/storage/

### Bonus Resources

- Redis configuration — https://redis.io/docs/latest/operate/oss_and_stack/management/config/
- Redis: Key eviction — https://redis.io/docs/latest/develop/reference/eviction/
- Redis FAQ — https://redis.io/docs/latest/develop/get-started/faq/
- redis/redis redis.conf (GitHub) — https://raw.githubusercontent.com/redis/redis/8.4/redis.conf
- WordPress Redis Object Cache — https://wordpress.org/plugins/redis-cache/
- Adminer — https://www.adminer.org/
- PHP: Built-in web server — https://www.php.net/manual/en/features.commandline.webserver.php
- Emscripten: emcc documentation — https://emscripten.org/docs/tools_reference/emcc.html
- emsdk README — https://github.com/emscripten-core/emsdk
- RFC 959: File Transfer Protocol — https://www.rfc-editor.org/rfc/rfc959.txt
- RFC 4217: Securing FTP with TLS — https://www.rfc-editor.org/rfc/rfc4217.txt
- RFC 2577: FTP Security Considerations — https://www.rfc-editor.org/rfc/rfc2577.txt
- vsftpd — https://security.appspot.com/vsftpd.html
- Linux man-pages: chroot(2) — https://man7.org/linux/man-pages/man2/chroot.2.html
- Go html/template — https://pkg.go.dev/html/template
- Docker Compose: multiple compose files merge — https://docs.docker.com/compose/how-tos/multiple-compose-files/merge/

## Use of AI

AI was used to discuss implementation approaches, to help find candidate official documentation, and to help identify the cause of unexpected behaviour. Every piece of official documentation AI pointed to was read directly and checked, rather than taking AI's interpretation at face value. Generated commands and configuration were likewise all verified and understood before being adopted.
