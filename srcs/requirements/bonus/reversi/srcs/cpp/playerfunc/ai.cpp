#include "playerfunc.hpp"
#include <array>
#include <queue>
#include <vector>
#include <limits>
#include <algorithm>

namespace playerfunc {

static const std::array<std::array<int, 8>, 8> WEIGHTS = {{
    { 120, -20,  10,   5,   5,  10, -20,  120 },
    { -20, -40,  -5,  -5,  -5,  -5, -40,  -20 },
    {  10,  -5,   5,   3,   3,   5,  -5,   10 },
    {   5,  -5,   3,   3,   3,   3,  -5,    5 },
    {   5,  -5,   3,   3,   3,   3,  -5,    5 },
    {  10,  -5,   5,   3,   3,   5,  -5,   10 },
    { -20, -40,  -5,  -5,  -5,  -5, -40,  -20 },
    { 120, -20,  10,   5,   5,  10, -20,  120 }
}};

static int getLayer(int r, int c) {
    int d_row = std::min(r, 7 - r);
    int d_col = std::min(c, 7 - c);
    return std::min(d_row, d_col);
}

static bool isStableOrWall(int r, int c, Board::Player player, 
        const std::vector<std::vector<Board::Player> >& stableMap) {
    if (r < 0 || r >= 8 || c < 0 || c >= 8) {
        return true;
    }
    return stableMap[r][c] == player;
}

static int evaluate(const Board& board, Board::Player maximizing) {
    int black = board.getBlackCount();
    int white = board.getWhiteCount();

    // 1. 合法手（開放度）の計算
    int blackValidateNum = 0;
    int whiteValidateNum = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (board.isValidMove(r, c, Board::Player::BLACK)) blackValidateNum++;
            if (board.isValidMove(r, c, Board::Player::WHITE)) whiteValidateNum++;
        }
    }

    // 2. 終局判定 (両者打つ手がない)
    if (blackValidateNum == 0 && whiteValidateNum == 0) {
        // 終局時は「石の差」をスコアとする。
        // ご指摘の通り、単なる WIN_SCORE では「最善の負け」を選べないため、
        // 石の差をスコアに反映させる。

        // 中盤の評価値が取りうる最大値（例: 隅4つと開放度で+5000点）よりも
        // 終局スコアが「絶対に」大きくなるように重みを設定する。
        const int ENDGAME_WEIGHT = 100000; // 10万点 (中盤スコアはこれを超えない前提)

        int black_stone_diff = black - white; // 黒基準の石差
        int white_stone_diff = white - black; // 白基準の石差

        if (maximizing == Board::Player::BLACK) {
            if (black_stone_diff > 0) {
                // 勝ち: 10万点 + 石差 (例: 2石勝ちなら 100002点)
                return ENDGAME_WEIGHT + black_stone_diff;
            } else if (black_stone_diff < 0) {
                // 負け: -10万点 + 石差 (例: 2石負けなら -100000 + (-2) = -100002点)
                // (40石負けなら -100000 + (-40) = -100040点)
                // AIはスコアが高い方(-100002)を選ぶため、2石負けを選ぶ。
                return -ENDGAME_WEIGHT + black_stone_diff;
            } else {
                return 0; // 引き分け
            }
        } else { // maximizing == Board::Player::WHITE
            if (white_stone_diff > 0) {
                return ENDGAME_WEIGHT + white_stone_diff;
            } else if (white_stone_diff < 0) {
                return -ENDGAME_WEIGHT + white_stone_diff;
            } else {
                return 0; // 引き分け
            }
        }
    }

    // --- ここから中盤の評価 ---

    // 3. 盤面評価テーブル (Positional Score)
    // 一般的なオセロの評価値テーブル

    int blackPositionalScore = 0;
    int whitePositionalScore = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            char cell = board.getCell(r, c);
            if (cell == 'x') {
                blackPositionalScore += WEIGHTS[r][c];
            } else if (cell == 'o') {
                whitePositionalScore += WEIGHTS[r][c];
            }
        }
    }
    // 黒視点での盤面評価値
    int positionalScore = blackPositionalScore - whitePositionalScore;

    // 4. 開放度 (Mobility Score)
    // 黒視点での開放度の評価値
    int mobilityScore = blackValidateNum - whiteValidateNum;

    // 5. 確定石 (Fixed Stones Score)
    int blackStableCount = 0;
    int whiteStableCount = 0;
    std::vector<std::vector<Board::Player> > stableMap(
        8, std::vector<Board::Player>(8, Board::Player::OTHER)
    );
    const int corners[4][2] = {{0,0}, {0,7}, {7,0}, {7,7}};
    const int drs[] = {-1, -1, 0, 1, 1, 1, 0, -1};
    const int dcs[] = {0, 1, 1, 1, 0, -1, -1, -1};
    for (int i = 0; i < 4; i++) {
        int r = corners[i][0];
        int c = corners[i][1];
        char cell = board.getCell(r, c);
        if (cell == '.') continue;
        Board::Player player = (cell == 'x') ? Board::Player::BLACK : Board::Player::WHITE;
        std::priority_queue<std::pair<int, std::pair<int, int>>,
            std::vector<std::pair<int, std::pair<int, int>>>,
            std::greater<std::pair<int, std::pair<int, int>>>> pq;
        std::vector<std::vector<bool>> visited(8, std::vector<bool>(8, false));
        int layer = getLayer(r, c);  // 0
        pq.push({layer, {r, c}});
        while (!pq.empty()) {
            auto top = pq.top(); pq.pop();
            auto pos = top.second;
            int cr = pos.first;
            int cc = pos.second;
            if (visited[cr][cc]) continue;
            visited[cr][cc] = true;
            bool axis1 = isStableOrWall(cr - 1, cc, player, stableMap) ||
                        isStableOrWall(cr + 1, cc, player, stableMap);
            bool axis2 = isStableOrWall(cr, cc - 1, player, stableMap) ||
                        isStableOrWall(cr, cc + 1, player, stableMap);
            bool axis3 = isStableOrWall(cr - 1, cc - 1, player, stableMap) ||
                        isStableOrWall(cr + 1, cc + 1, player, stableMap);
            bool axis4 = isStableOrWall(cr - 1, cc + 1, player, stableMap) ||
                        isStableOrWall(cr + 1, cc - 1, player, stableMap);
            bool isStable = axis1 && axis2 && axis3 && axis4;
            if (!isStable) continue;
            if (stableMap[cr][cc] == Board::Player::OTHER) {
                stableMap[cr][cc] = player;
                if (player == Board::Player::BLACK) blackStableCount++;
                else whiteStableCount++;
            }
            for (int d = 0; d < 8; d++) {
                int nr = cr + drs[d];
                int nc = cc + dcs[d];
                if (nr < 0 || nr >= 8 || nc < 0 || nc >= 8) continue;
                if (visited[nr][nc]) continue;
                char ncell = board.getCell(nr, nc);
                if ((player == Board::Player::BLACK && ncell != 'x') ||
                    (player == Board::Player::WHITE && ncell != 'o')) {
                    continue;
                }
                int nlayer = getLayer(nr, nc);
                pq.push({nlayer, {nr, nc}});
            }
        }
    }

    int stableScore = blackStableCount - whiteStableCount;


    // 6. 総合評価
    // 各スコアの重み付け (この比率はAIの棋風を決め、調整が必要です)
    int POSITIONAL_WEIGHT;  // 盤面評価
    int MOBILITY_WEIGHT;    // 開放度
    int CORNER_WEIGHT;     // 隅のボーナス (WEIGHTSの120と合わせて約1000点)

    int totalStones = black + white;
    if (totalStones < 20) { // 序盤
        POSITIONAL_WEIGHT = 10;
        MOBILITY_WEIGHT = 200;
        CORNER_WEIGHT = 400;
    } else if (totalStones < 50) { // 中盤
        POSITIONAL_WEIGHT = 10;
        MOBILITY_WEIGHT = 75;
        CORNER_WEIGHT = 800;
    } else { // 終盤
        POSITIONAL_WEIGHT = 10;
        MOBILITY_WEIGHT = 50;
        CORNER_WEIGHT = 1000;
    }

    int finalScore = (POSITIONAL_WEIGHT * positionalScore) +
                     (MOBILITY_WEIGHT * mobilityScore) +
                     (CORNER_WEIGHT * stableScore);

    // maximizing プレイヤーの視点に変換して返す
    if (maximizing == Board::Player::BLACK) {
        return finalScore;
    } else {
        return -finalScore; // 白視点の場合は、黒基準のスコアを反転させる
    }
}

static Board::Player opponent(Board::Player p) { return Board::opponent(p); }

// alpha-beta 実装。board は現在の局面（状態を変更して undo で戻すことを前提）。
static int alphabeta(Board& board, Board::Player toMove, Board::Player maximizing, int depth, int alpha, int beta) {
    if (depth <= 0) {
        return evaluate(board, maximizing);
    }

    // 合法手列挙
    std::vector<std::pair<int,int>> moves;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (board.isValidMove(r, c, toMove)) moves.emplace_back(r, c);
        }
    }

    // パス判定: 自分にも相手にも手がなければ終局
    if (moves.empty()) {
        bool oppHas = false;
        Board::Player opp = opponent(toMove);
        for (int r = 0; r < 8 && !oppHas; ++r) {
            for (int c = 0; c < 8; ++c) {
                if (board.isValidMove(r, c, opp)) { oppHas = true; break; }
            }
        }
        if (!oppHas) {
            return evaluate(board, maximizing);
        }
        // パス: 手番を相手に渡す（深さは消費する）
        return alphabeta(board, opp, maximizing, depth - 1, alpha, beta);
    }

    std::sort(moves.begin(), moves.end(), [&](const std::pair<int, int>& a, const std::pair<int, int>& b) {
        return WEIGHTS[a.first][a.second] > WEIGHTS[b.first][b.second];
    });

    const int INF = std::numeric_limits<int>::max() / 4;

    if (toMove == maximizing) {
        int value = -INF;
        for (auto &m : moves) {
            board.placePiece(m.first, m.second, toMove);
            int v = alphabeta(board, opponent(toMove), maximizing, depth - 1, alpha, beta);
            board.undo();
            if (v > value) value = v;
            if (v > alpha) alpha = v;
            if (alpha >= beta) break; // Beta cutoff
        }
        return value;
    } else {
        int value = INF;
        for (auto &m : moves) {
            board.placePiece(m.first, m.second, toMove);
            int v = alphabeta(board, opponent(toMove), maximizing, depth - 1, alpha, beta);
            board.undo();
            if (v < value) value = v;
            if (v < beta) beta = v;
            if (beta <= alpha) break; // Alpha cutoff
        }
        return value;
    }
}

Move aiMove(const Board& boardConst, Board::Player player) {
    // 局面をコピーしてその上で探索（コピー中の place/undo を使用）
    Board board = boardConst;

    // 探索深さはここでは固定（必要なら調整可能）
    const int MAX_DEPTH = 8;
    const int INF = std::numeric_limits<int>::max() / 4;

    // 合法手の取得
    std::vector<std::pair<int,int>> moves;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (board.isValidMove(r, c, player)) moves.emplace_back(r, c);
        }
    }

    if (moves.empty()) {
        // パス
        return {-1, -1};
    }

    int bestScore = -INF;
    Move bestMove = moves.front();

    for (auto &m : moves) {
        board.placePiece(m.first, m.second, player);
        int remainingCells = 64 - board.getTotalCount();
        int score = 0;
        if (remainingCells <= 14) {
            score = alphabeta(board, opponent(player), player, remainingCells, -INF, INF);
        } else {
            score = alphabeta(board, opponent(player), player, MAX_DEPTH - 1, -INF, INF);
        }
        board.undo();
        if (score > bestScore) {
            bestScore = score;
            bestMove = m;
        }
    }

    return bestMove;
}

} // namespace playerfunc
