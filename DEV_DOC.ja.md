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
WP_USER_EMAIL=<一般ユーザのメールアドレス>
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
make down     # コンテナを停止(イメージ・ボリューム・データは残す)
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
