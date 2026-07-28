# 開発者ドキュメント

## 前提条件

- Docker Engine + Docker Compose plugin(v2。`depends_on`の`condition: service_healthy`がv2の機能のため)
- sudo権限を使えるLinux OS

## 設定ファイルとSecrets

`git clone`しただけでは動かない。以下の2種類のファイルはGitに含まれない(`.gitignore`)ため、手動で作成する必要がある。

**`srcs/.env`**(環境変数の設定):
```
MYSQL_DATABASE=wordpress
MYSQL_USER=wp_user
WP_TITLE=Inception
WORDPRESS_DB_HOST=mariadb
WORDPRESS_DB_NAME=wordpress
WORDPRESS_DB_USER=wp_user
WP_ADMIN_USER=<管理者のユーザー名>
WP_ADMIN_EMAIL=<管理者のメールアドレス>
WP_USER=<一般ユーザのユーザー名>
WP_USER_EMAIL=<一般ユーザのメールアドレス>
WP_REDIS_HOST=redis
```
`DOMAIN_NAME`と`DATA_PATH`はMakefileが`whoami`から自動算出するため、`.env`には書かない。

**`secrets/*.txt`**(パスワード、改行なしで保存):

以下のパスワードを設定する必要がある
- `secrets/db_root_password.txt`: MariaDBのrootユーザーのパスワード
- `secrets/db_password.txt`: MariaDB内のWordPress専用DBアカウントであるwp_userのパスワード
- `secrets/wp_admin_password.txt`: WordPressサイトの管理者アカウントのパスワード
- `secrets/wp_user_password.txt`: WordPressサイトの一般ユーザーのパスワード

ボーナスサービスを起動する場合(`make bonus`)は、以下も必要になる。

- `secrets/redis_password.txt`: Redisの`requirepass`に設定するパスワード
- `secrets/ftp_password.txt`: FTPユーザー`ftp_user`のパスワード
- `secrets/watcher_password.txt`: WatcherのBasic認証のパスワード

**注意**: これらは、対応する機能を使わない設定にしていてもファイル自体は存在している必要がある。Docker Composeはサービス定義に書かれたsecretを設定内容と無関係にマウントするため、ファイルが無いと`bind source path does not exist`でコンテナの作成に失敗する。特に`secrets/watcher_password.txt`は、`config/watcher.yml`で`auth.enabled: false`にしている場合でも必要になる。

## ビルドと起動(Makefile & Docker Compose)

```bash
make              # create_data_dir → build → up → hosts(mandatoryのみ起動)
make bonus        # create_data_dir → build → up → hosts(mandatory + bonus)
make build        # mandatoryのイメージのビルドのみ
make build_bonus  # mandatory + bonusのイメージのビルドのみ
make down         # コンテナを停止(mandatory + bonus)
make hosts        # /etc/hostsにドメインを追記(make / make bonus が自動で実行する)
make clean        # コンテナ・イメージ・ボリューム定義を削除
make fclean       # clean + ホスト上の実データ(/home/<login>/data)も削除
make re           # fclean + all
```

内部的には `Makefile` が `LOGIN`/`DATA_PATH`/`DOMAIN_NAME` を算出して`export`し、`docker compose -f srcs/docker-compose.yml build/up` を呼び出している。

## コンテナとボリュームの管理

```bash
docker compose -f srcs/docker-compose.yml ps        # 稼働状況
docker exec -it wordpress sh                        # コンテナに入る
docker exec wordpress wp user list --path=/var/www/html --allow-root   # WordPressユーザー一覧
docker volume ls                                     # ボリューム一覧
```

## 起動順序と初期化

- MariaDBは初期化SQLを`/run/mysqld/init.sql`(所有者`mysql`、パーミッション600)に書き出し、`mariadbd --init-file`に渡している。サーバーはこれを**接続を受け付ける前に**実行する。各文は`IF NOT EXISTS`または同じ値の再設定で構成された冪等なものなので、再起動しても安全。
- MariaDBとRedisはそれぞれ`healthcheck`を宣言し、WordPress側は両方に対して`depends_on: { condition: service_healthy }`を宣言している。entrypointスクリプトの中でループして待つ処理は無く、各コンテナのPID1はサービス本体のプロセスそのものになっている。
- Redisのhealthcheckが`grep -q PONG`を挟んでいるのは意図的。`redis-cli ping`はサーバーが`NOAUTH`や`WRONGPASS`を返した場合でも終了コード0になるため、これが無いと「認証が通ること」を確認しないままhealthyと判定されてしまう。
- **init-file内の文が失敗してもサーバーは止まらない**。MariaDBはエラーをログに書き、そのファイルの残りの文をスキップした上で、接続の受け付けまで進む。したがって初期化の失敗は`docker logs mariadb`で見つける必要があり、コンテナのクラッシュとしては現れない。

healthの状態を確認するには:

```bash
docker inspect --format '{{.State.Health.Status}}' mariadb redis
```

## データの保存場所と永続化

WordPressのファイルとMariaDBのデータは、named volume(`wordpress_vol`, `mariadb_vol`)として管理されているが、`driver_opts`によって実体はホストの `/home/<login>/data/wordpress_vol` と `/home/<login>/data/mariadb_vol` に固定されている。

**注意**: `docker compose down -v` は Docker 上のボリューム定義を削除するだけで、上記のホストディレクトリの中身は削除されない(意図的な設計で、コンテナを作り直してもデータは残る)。本当に初期状態からやり直したい場合は、ホストディレクトリを明示的に削除する必要がある。

```bash
sudo rm -rf /home/$(whoami)/data/wordpress_vol
sudo rm -rf /home/$(whoami)/data/mariadb_vol
```

## ボーナスサービス

bonusサービスは`srcs/requirements/bonus/<service>/`配下にそれぞれ配置し、`srcs/docker-compose.bonus.yml`で定義する。mandatoryとは別ファイルにすることで、`make`(mandatoryのみ)の挙動に一切影響を与えない構成にしている。

```bash
make bonus       # mandatory + bonus 全部をビルド・起動
make down        # mandatory + bonus 全部を停止(bonus起動時も安全に停止できる)
```

`Makefile`は`COMPOSE_FILE`(mandatory用)と`COMPOSE_FILE_BONUS`(bonus差分)を分けて保持しており、`bonus`系のターゲットでは両方を`-f`で重ねて`docker compose`を呼び出す。

### 公開方式

mandatoryのnginxコンテナのビルド引数(`NGINX_CONF`)を切り替えることで、`nginx.bonus.conf`(bonus用の追加`server`ブロックを含む設定)を読み込ませる。Adminer(8443番)・reversi(8081番)はこの経由で公開している。Redis(内部専用、ポート非公開)とWatcher(8082番を直接公開)はこの方式を取らない意図的な例外としている。

### Redis

- 配置: `srcs/requirements/bonus/redis/`
- 専用ボリュームなし(キャッシュデータは正データから再生成可能なため)
- `bind`を緩め、代わりに`requirepass`でパスワード認証(`secrets/redis_password.txt`)
- WordPress側は`entrypoint.sh`内で`USE_REDIS`環境変数によって分岐し、bonus起動時(`make bonus`)のみ`wp redis enable`等を実行する

### Adminer

- 配置: `srcs/requirements/bonus/adminer/`(php-fpmのみ、Webサーバーは内蔵しない)
- nginxから`fastcgi_pass adminer:9000`で転送。単一PHPファイルのため`try_files`は不要
- 単一のPHPファイルは、Adminer公式が案内している常設URL(`https://www.adminer.org/latest-ja.php`)からビルド時に取得している。この"latest"はAdminer自身のリリースチャンネルを指すもので、Dockerで禁止されているイメージタグの`latest`とは無関係
- secretsは使用しない(接続情報はブラウザで都度入力する設計)

### reversi

- 配置: `srcs/requirements/bonus/reversi/`
- マルチステージビルド: builderステージでAlpine上にemsdkを自前でインストールし、C++ソースをWebAssemblyへコンパイル。最終ステージには生成された`.wasm`/`.js`と静的ファイルのみを配置
- emsdkのバージョンは`ARG EMSDK_VERSION`で固定し、cloneは`--depth 1`で行っている(全履歴を取得せずに再現性を保つため)
- 配信は`python3 -m http.server`。外部公開はnginxの`proxy_pass http://reversi:8000;`経由

### FTP Server

- 配置: `srcs/requirements/bonus/ftp-server/`
- vsftpdを使用。`port_enable=NO`でアクティブモード(bounce attackの原因)を無効化し、パッシブモードのみ許可
- `ssl_enable=YES`でFTPSに対応(自己署名証明書、mandatoryのnginxと同じ生成パターン)
- ユーザーは`ftp_user`のみ(UID/GID 65534、`nobody`と一致させ、`/var/www/html`へのアクセス権限を揃えている)
- パスワードは`secrets/ftp_password.txt`、コンテナ起動時に`entrypoint.sh`が読み込み反映する

### Watcher

- 配置: `srcs/requirements/bonus/watcher/`(Go製)
- マルチステージビルド: builderステージで`go build`により静的バイナリを生成し、最終ステージにバイナリのみコピー
- 監視対象・チェック間隔・タイムアウト・通知設定は`config/watcher.yml`(YAML)で管理し、`internal/config`パッケージが読み込む
- 死活判定はTCP接続確認またはHTTP GETによるネットワークプローブのみで行い、Docker APIへの`docker.sock`経由アクセスは採用していない(理由はREADMEの「ボーナス」セクション参照)
- 各対象の稼働率(累積%)・Checkerの種類ごとの平均応答時間を集計して表示する
- ステータスが変化した際、Discord Webhookへ通知する(`config/watcher.yml`の`alert.webhook`で有効化・通知先URLを設定)
- ダッシュボード(`/`)とJSON API(`/api/status`)はBasic認証で保護している。有効化とユーザー名は`config/watcher.yml`の`auth`セクションで設定し、パスワードは起動時に`/run/secrets/watcher_password`から読み込む
- 資格情報の比較は、`crypto/sha256`で固定長にしてから`crypto/subtle.ConstantTimeCompare`で行う(比較対象の長さが漏れることと、比較にかかる時間から内容を推測されることの両方を避けるため)。ユーザー名とパスワードの判定を`&&`の短絡評価に任せず、両方を必ず評価してから結合している
- `/health`は認証の対象外(死活確認用エンドポイントとして、外部の監視ツールから無認証で叩けるようにするため)
