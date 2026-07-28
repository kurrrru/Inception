# ユーザードキュメント

## 提供されるサービスの概要

必須構成は次のコンテナです。ボーナス部分については「ボーナスサービス」を参照

| サービス | 役割 |
|---|---|
| nginx | HTTPS(443番)でサイトへのアクセスを受け付ける唯一の入口 |
| wordpress | WordPress本体(ブログ/CMS)を実行 |
| mariadb | WordPressのデータを保存するデータベース |

## プロジェクトの起動と停止

起動:
```bash
make
```
`docker`と`docker compose`が使える環境であれば、これだけで3つのコンテナがビルド・起動する。

停止:
```bash
make down
```
データ(`/home/<login>/data`配下)は残ったまま停止する。完全に削除したい場合は以下を実行する。
```bash
make fclean
```

## サイトと管理画面へのアクセス

初回のみ、ブラウザ環境から名前解決できるようにする必要がある。

```bash
make hosts
```
これは `/etc/hosts` に `127.0.0.1 <login>.42.fr` を追記する(sudoが必要)。

その後、ブラウザで以下を開く:
- サイト本体: `https://<login>.42.fr`
- 管理画面: `https://<login>.42.fr/wp-admin`

自己署名証明書のため、ブラウザの警告は「詳細設定 > このまま進む」等で許可する。

## 認証情報の場所と管理

パスワード類はリポジトリの`secrets/`ディレクトリ配下に平文ファイルとして置く(Gitには含まれない)。

| ファイル | 用途 |
|---|---|
| `secrets/db_root_password.txt` | MariaDBのrootパスワード |
| `secrets/db_password.txt` | WordPress用DBユーザーのパスワード |
| `secrets/wp_admin_password.txt` | WordPress管理者アカウントのパスワード |
| `secrets/wp_user_password.txt` | WordPress一般ユーザーのパスワード |

管理者ユーザー名等の非機密情報は`srcs/.env`に記載する。

## サービスが正しく動作しているかの確認方法

```bash
docker ps                                              # 3コンテナが Up で安定しているか
docker compose -f srcs/docker-compose.yml logs         # 各コンテナのログ
curl -k https://<login>.42.fr/ -o /dev/null -s -w "%{http_code}\n"   # 200 が返るか
```

## ボーナスサービス

### Redis

WordPressのキャッシュとして裏側で動作する。ユーザーが直接操作する画面は無い。動作確認は次のコマンドで行う。

```bash
docker exec wordpress wp redis status --path=/var/www/html --allow-root
```

`Status: Connected`と表示されれば正常。

### Adminer

`https://<login>.42.fr:8443/` にアクセスするとログイン画面が表示される。

| 項目 | 値 |
|---|---|
| System | MySQL |
| Server | mariadb |
| Username | wp_user |
| Password | `secrets/db_password.txt` の中身 |
| Database | wordpress |

### reversi

`https://<login>.42.fr:8081/` にアクセスすると、自作のリバーシAIとブラウザ上で対局できる。ログイン等は不要。

### FTP Server

WordPressサイトのファイルへFTP/FTPSでアクセスできる。

| 項目 | 値 |
|---|---|
| ホスト | `<login>.42.fr` |
| ポート | 21(パッシブポート範囲: 30000-30009) |
| ユーザー名 | ftp_user |
| パスワード | `secrets/ftp_password.txt` の中身 |
| 接続方式 | FTPS(明示的なAUTH TLS) |

### Watcher

`https://<login>.42.fr:8082/` にアクセスすると、各サービスの死活状態・稼働率・平均応答時間が一覧表示される(5秒間隔で自動更新)。認証は設けていないため、誰でも閲覧できる。

- `/api/status`: 同内容をJSON形式で取得できるAPI
- `/health`: watcher自身の死活確認用エンドポイント
