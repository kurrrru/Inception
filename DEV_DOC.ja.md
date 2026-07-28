# 開発者ドキュメント

## 前提条件

- Docker Engine + Docker Compose plugin
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
WP_USER_EMAIL=<一般ユーザのユーザー名>
```
`DOMAIN_NAME`と`DATA_PATH`はMakefileが`whoami`から自動算出するため、`.env`には書かない。

**`secrets/*.txt`**(パスワード、改行なしで保存):

以下のパスワードを設定する必要がある
- `secrets/db_root_password.txt`: MariaDBのrootユーザーのパスワード
- `secrets/db_password.txt`: MariaDB内のWordPress専用DBアカウントであるwp_userのパスワード
- `secrets/wp_admin_password.txt`: WordPressサイトの管理者アカウントのパスワード
- `secrets/wp_user_password.txt`: WordPressサイトの一般ユーザーのパスワード

## ビルドと起動(Makefile & Docker Compose)

```bash
make          # create_data_dir → build → up(mandatoryのみ起動)
make clean    # コンテナ・イメージ・ボリューム定義を削除
make fclean   # clean + ホスト上の実データ(/home/<login>/data)も削除
make re       # fclean + all
```

内部的には `Makefile` が `LOGIN`/`DATA_PATH`/`DOMAIN_NAME` を算出して`export`し、`docker compose -f srcs/docker-compose.yml build/up` を呼び出している。

## コンテナとボリュームの管理

```bash
docker compose -f srcs/docker-compose.yml ps        # 稼働状況
docker exec -it wordpress sh                        # コンテナに入る
docker exec wordpress wp user list --path=/var/www/html --allow-root   # WordPressユーザー一覧
docker volume ls                                     # ボリューム一覧
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
- secretsは使用しない(接続情報はブラウザで都度入力する設計)

### reversi

- 配置: `srcs/requirements/bonus/reversi/`
- マルチステージビルド: builderステージでAlpine上にemsdkを自前でインストールし、C++ソースをWebAssemblyへコンパイル。最終ステージには生成された`.wasm`/`.js`と静的ファイルのみを配置
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
- 認証機能は未実装(bonusの「任意サービス」枠として、他の実装(config駆動・稼働率集計・アラート通知)を優先したため)
