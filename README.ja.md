*このプロジェクトは nkawaguc によって、42 のカリキュラムの一環として作成されました。*

# Inception

[![stack](https://github.com/kurrrru/Inception/actions/workflows/stack.yml/badge.svg)](https://github.com/kurrrru/Inception/actions/workflows/stack.yml)
[![docs](https://github.com/kurrrru/Inception/actions/workflows/docs.yml/badge.svg)](https://github.com/kurrrru/Inception/actions/workflows/docs.yml)
[![dockerfile-lint](https://github.com/kurrrru/Inception/actions/workflows/dockerfile-lint.yml/badge.svg)](https://github.com/kurrrru/Inception/actions/workflows/dockerfile-lint.yml)
[![secret-scan](https://github.com/kurrrru/Inception/actions/workflows/secret-scan.yml/badge.svg)](https://github.com/kurrrru/Inception/actions/workflows/secret-scan.yml)
[![final-newline](https://github.com/kurrrru/Inception/actions/workflows/final-newline.yml/badge.svg)](https://github.com/kurrrru/Inception/actions/workflows/final-newline.yml)
[![forbidden-patterns](https://github.com/kurrrru/Inception/actions/workflows/forbidden-patterns.yml/badge.svg)](https://github.com/kurrrru/Inception/actions/workflows/forbidden-patterns.yml)
[![compose-validate](https://github.com/kurrrru/Inception/actions/workflows/compose-validate.yml/badge.svg)](https://github.com/kurrrru/Inception/actions/workflows/compose-validate.yml)

## 概要
Inceptionは、システム管理に関する知識を深めることを目的としたプロジェクトで、Dockerを用いてWebインフラを構築するプロジェクトである。
必須構成として、Docker Composeでnginx・WordPress・MariaDBの3コンテナを連携させ、WordPressサイトをホストする。bonusとして追加したサービスについては、「ボーナス」セクションに記載する。

- **nginx**: TLSv1.2/1.3のみでHTTPS(443番)を受け付ける唯一の外部入口
- **WordPress**: PHP-FPMでWordPress本体を実行するコンテナ
- **MariaDB**: WordPress用のデータベースを保持するコンテナ


### Dockerの使い方と含まれるソース

各サービスは既製のDockerイメージを使わず、`alpine:3.23`をベースに自分で書いたDockerfileからビルドしている。
`srcs/requirements/<service>/`配下にそれぞれのDockerfileと設定ファイル(nginx.conf、entrypoint.sh等)を配置し、`srcs/docker-compose.yml`でオーケストレーションしている。ビルドとライフサイクル管理はルートの`Makefile`から行う。

### 主な設計判断
- **冪等性**: 初期化や設定変更を伴う処理は、何度実行しても安全であるように設計している。
- **コンテナの寿命 = サービス本体の寿命**: コンテナを生かし続けるための延命策(ダミープロセス等)を避け、サービス本体プロセスの生死とコンテナの生死が一致するように設計している。
- **露出の最小化**: 準備が完了していない状態を、外部からアクセス可能にしない。
- **決め打ちを避ける**: 環境やユーザー固有の値をハードコードせず、実行時・ビルド時に導出する設計にしている。

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
Dockerのbridgeネットワークでは、各コンテナが独自のネットワーク名前空間(ネットワークインターフェース・IP・ルーティングテーブル・ポートをプロセスごとに隔離するLinuxカーネルの仕組み)を持ち、同じユーザー定義ネットワークに接続したコンテナ同士はコンテナ名(DNS)で通信できる。host網ではコンテナがホストのネットワーク名前空間を共有し、隔離が失われ、`-p`によるポートマッピングも無効になる。本プロジェクトは独自のブリッジネットワーク(`inception`)を作り、コンテナ間はサービス名による内部DNSで通信し、外部に公開するのはnginxの443番のみに限定している。

#### Dockerボリューム vs バインドマウント
ボリュームはDockerデーモンが管理する永続ストレージで、データの実体はホスト上にあるがDocker管理領域に置かれる。bind mountはホストの任意のパスをコンテナに直接リンクし、Docker外のプロセスとコンテナの両方が同時にファイルへアクセス・変更できる。本プロジェクトは named volume でありながら、`driver_opts`で実体をホストの`/home/<login>/data`配下に固定するハイブリッド構成にしている。

## ボーナス

## 使い方

```bash
make            # ビルド + 起動(mandatoryのみ)
make hosts      # /etc/hosts にドメインを追記(要sudo)
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

AIの利用について: 実装方針の相談、公式ドキュメントの候補探し、想定外の挙動に対する原因特定にAIを利用した。AIが挙げた公式ドキュメントは実際に自分で読みに行って内容を確認しており、AIの解釈をそのまま鵜呑みにしたわけではない。生成されたコマンド・設定についても、すべて自分で検証し理解した上で採用している。
