*このプロジェクトは nkawaguc によって、42 のカリキュラムの一環として作成されました。*

# Inception

## 概要
Inceptionは、Dockerを用いてWebインフラを構築し、システム管理に関する知識を深めることを目的としたプロジェクトである。
必須構成として、Docker Composeでnginx・WordPress・MariaDBの3コンテナを連携させ、WordPressサイトをホストする。bonusとして追加したサービスについては、「ボーナス」セクションに記載する。

- **nginx**: TLSv1.2/1.3のみでHTTPS(443番)を受け付ける唯一の外部入口
- **WordPress**: PHP-FPMでWordPress本体を実行するコンテナ
- **MariaDB**: WordPress用のデータベースを保持するコンテナ

## プロジェクト概要

### Dockerの使い方と含まれるソース

各サービスは既製のDockerイメージを使わず、`alpine:3.23`をベースに自分で書いたDockerfileからビルドしている。
`srcs/requirements/<service>/`配下にそれぞれのDockerfileと設定ファイル(nginx.conf、entrypoint.sh等)を配置し、`srcs/docker-compose.yml`でオーケストレーションしている。ビルドとライフサイクル管理はルートの`Makefile`から行う。

### 主な設計判断
- **冪等性**: 初期化や設定変更を伴う処理は、何度実行しても安全であるように設計している。
- **コンテナの寿命 = サービス本体の寿命**: コンテナを生かし続けるための延命策(ダミープロセス等)を避け、サービス本体プロセスの生死とコンテナの生死が一致するように設計している。
- **露出の最小化**: 準備が完了していない状態を、外部からアクセス可能にしない。
- **決め打ちを避ける**: 環境やユーザー固有の値をハードコードせず、実行時・ビルド時に導出する設計にしている。
- **起動順序はhealthcheckで保証する**: entrypointの中でループして待つのではなく、準備が整ったことを`healthcheck`で表明し、`depends_on: condition: service_healthy`で受け取る形にしている。MariaDBは初期化SQLを`--init-file`で流しており、これはサーバーが接続を受け付ける前に実行されるため、healthyになった時点でデータベースとユーザーは既に存在している。

### 比較

#### 仮想マシン vs Docker

VMとコンテナは、リソースの隔離という点で似た利点を持つが、実現方法が異なる。VMはハードウェアを仮想化し、独自のカーネル・ドライバ・OS一式を含む完全なOSとして動作するため、単一アプリの隔離のためだけに使うにはオーバーヘッドが大きい。
一方コンテナはOSを仮想化する形で、実行に必要なファイルだけを含む隔離されたプロセスとして動き、複数のコンテナが同じホストカーネルを共有するため、より少ないリソースで多くのアプリを実行できる。本プロジェクトも、3つのサービスをそれぞれ軽量なコンテナとして分離し、VMは「その上でDockerを動かす土台」として1つだけ用意している。

理論的な観点でも、VMとコンテナは別レイヤーにある。仮想マシンは「仮想マシンモニタ(VMM、ハイパーバイザとも呼ばれる)」というソフトウェアの上で動く。Popek と Goldberg(1974)は、あるVMMが正しく仮想化を実現していると言えるために満たすべき3つの性質を定義した。

- **等価性**: VMM上で動くプログラムは、実機に直接インストールして動かした場合と本質的に同じ結果になること
- **資源の管理**: VMMが、割り当てたCPUやメモリなどの資源を完全に管理下に置いていること
- **効率性**: 命令の大部分が、VMMを介さず実機のCPUで直接実行されること(そうでないと極端に遅くなる)

コンテナ(Docker)はこのVMMという層をそもそも持たない。ホストのカーネルを直接共有し、**名前空間**(プロセスごとに見える資源を隔離する仕組み)と**cgroups**(CPU・メモリ等の使用量をプロセスごとに制限する仕組み)によってプロセスを隔離している。つまりコンテナは、Popek-Goldbergが議論の対象とした「VMM経由の仮想化」とはそもそも別の方式で隔離を実現しており、この理論の枠組みの外側にある。

#### Secrets vs 環境変数
Docker Composeの公式ドキュメントは、環境変数は値が平文で保持・参照されやすく`docker inspect`やログから見える可能性があるため、機密値には適さないとしている。secretsはコンテナ内の`/run/secrets/<name>`にファイルとしてマウントされ、環境変数より露出が少ない。本プロジェクトではパスワード類をすべてsecretsで扱い、非機密の設定のみ`.env`に置いている。

#### Dockerネットワーク vs ホストネットワーク
Dockerのbridgeネットワークでは、各コンテナが独自のネットワーク名前空間(ネットワークインターフェース・IP・ルーティングテーブル・ポートをプロセスごとに隔離するLinuxカーネルの仕組み)を持ち、同じユーザー定義ネットワークに接続したコンテナ同士はコンテナ名(DNS)で通信できる。ホストネットワークではコンテナがホストのネットワーク名前空間を共有し、隔離が失われ、`-p`によるポートマッピングも無効になる。本プロジェクトは独自のブリッジネットワーク(`inception`)を作り、コンテナ間はサービス名による内部DNSで通信し、外部に公開するのはnginxの443番のみに限定している。

#### Dockerボリューム vs バインドマウント
ボリュームはDockerデーモンが管理する永続ストレージで、データの実体はホスト上にあるがDocker管理領域に置かれる。bind mountはホストの任意のパスをコンテナに直接リンクし、Docker外のプロセスとコンテナの両方が同時にファイルへアクセス・変更できる。本プロジェクトは named volume でありながら、`driver_opts`で実体をホストの`/home/<login>/data`配下に固定するハイブリッド構成にしている。

## ボーナス

必須構成に加えて、以下5つのサービスを追加した。

### 概要

| サービス | 役割 |
|---|---|
| Redis | WordPressのオブジェクトキャッシュ |
| Adminer | ブラウザから使えるDB管理GUI |
| reversi | 自作C++製リバーシAIをWebAssembly化した静的サイト |
| FTP Server | WordPressサイトのボリュームへアクセスするFTP/FTPSサーバー |
| Watcher | 自作Go製の死活監視ダッシュボード |

### 主な設計判断

- **公開方式の統一**: mandatoryのnginxコンテナを唯一のTLS終端点とし、Adminer・reversiはビルド引数(`NGINX_CONF`)で切り替える`nginx.bonus.conf`内の専用`server`ブロック経由で公開する。Redis(内部専用)・Watcher(専用ポート直接公開)はこの方式の意図的な例外とした。
- **Redis**: WordPressのキャッシュ用途に限定し、正データ(MariaDB)から再生成可能なため専用ボリュームは持たせていない。デフォルトの`bind 127.0.0.1`は他コンテナから届かないため緩めており、その代わり`requirepass`によるパスワード認証を必須にしている。WordPress側の設定は`USE_REDIS`という環境変数フラグで分岐し、bonus起動時のみ有効化することで、bonus抜き(`make`のみ)での起動を壊さないようにしている。
- **Adminer**: 単一PHPファイルで完結するツールのため専用のWebサーバーを持たせず、mandatoryのnginxからphp-fpmへ直接fastcgi転送する構成にした。認証情報はブラウザのログイン画面で毎回入力する設計のため、コンテナ側にsecretsを持たせていない。
- **reversi(単純な静的サイト枠)**: 自作のC++製リバーシAIをEmscriptenでWebAssembly化し、ブラウザ完結で遊べる静的サイトとして提供する。ビルド時だけ必要な重いツールチェーン(Emscripten一式)をマルチステージビルドで最終イメージから除外している。配信自体は`python3 -m http.server`で行い、外部公開はmandatoryのnginxからの`proxy_pass`に統一した。
- **FTP Server**: WordPressのボリュームに直接アクセスできるFTP/FTPSサーバー。RFC 2577(FTP Security Considerations)が推奨する対策に従い、bounce attackの原因となる`PORT`コマンド(アクティブモード)を無効化し、パッシブモードのみを許可している。制御・データ両方の接続をTLSで暗号化するFTPSにも対応している。
- **Watcher(任意サービス枠)**: 各コンテナの死活状態・稼働率・平均応答時間を可視化する自作のGo製ダッシュボード。監視対象・チェック間隔・通知設定は外部のYAMLファイルで管理し、コードの再ビルドなしに変更できる。コンテナの実行状態を権威的に取得する手段(Docker APIへの`docker.sock`経由アクセス)は、コンテナに実質ホスト操作の全権限を渡すことになりセキュリティ上のリスクが大きく、監視対象がwatcher1つに閉じるこのプロジェクトの規模に見合わないと判断し、採用しなかった。そのため状態表示は「正常に稼働中」「異常あり(・停止中)」の2つに絞り、ネットワークレベルの疎通結果だけからコンテナの停止を断定するような表示はしていない。ダッシュボードとJSON APIはBasic認証で保護し、パスワードはDocker secret経由で渡している(通信自体が自己署名証明書によるTLSなので、資格情報が平文で流れることはない)。死活確認用の`/health`だけは、外部の監視ツールから無認証で叩けるよう認証の対象外にしている。

### ボーナスの使い方

```bash
make bonus     # ビルド + 起動(mandatory + bonus全部)
```

各サービスへのアクセス方法はUSER_DOCを参照。

## 使い方

```bash
make            # ビルド + 起動(mandatoryのみ) + /etc/hosts にドメインを追記
```

VM内のブラウザで `https://<login>.42.fr` を開く。

## 参考資料
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

### ボーナスの参考資料

- Redis configuration — https://redis.io/docs/latest/operate/oss_and_stack/management/config/
- Redis: Key eviction — https://redis.io/docs/latest/develop/reference/eviction/
- Redis FAQ — https://redis.io/docs/latest/develop/get-started/faq/
- redis/redis redis.conf(GitHub) — https://raw.githubusercontent.com/redis/redis/8.4/redis.conf
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

### AI利用について

AIの利用について: 実装方針の相談、公式ドキュメントの候補探し、想定外の挙動に対する原因特定にAIを利用した。AIが挙げた公式ドキュメントは実際に自分で読みに行って内容を確認しており、AIの解釈をそのまま鵜呑みにしたわけではない。生成されたコマンド・設定についても、すべて自分で検証し理解した上で採用している。
