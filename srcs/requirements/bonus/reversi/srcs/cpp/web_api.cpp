#include "Board.hpp"
#include "playerfunc/playerfunc.hpp"

namespace {
    Board g_board;
    Board::Player g_human = Board::Player::BLACK;
    Board::Player g_ai = Board::Player::WHITE;
    Board::Player g_current = Board::Player::BLACK;
    int g_passCount = 0;
    bool g_gameOver = false;

    Board::Player fromInt(int value) {
        return value == 0 ? Board::Player::BLACK : Board::Player::WHITE;
    }

    bool inRange(int value) {
        return value >= 0 && value < 8;
    }

    bool hasAnyMove(Board::Player player) {
        for (int r = 0; r < 8; ++r) {
            for (int c = 0; c < 8; ++c) {
                if (g_board.isValidMove(r, c, player)) {
                    return true;
                }
            }
        }
        return false;
    }

    void evaluateGameState() {
        if (g_gameOver) {
            return;
        }
        if (g_board.getTotalCount() == 64 || g_passCount >= 2) {
            g_gameOver = true;
            return;
        }
        if (!hasAnyMove(Board::Player::BLACK) && !hasAnyMove(Board::Player::WHITE)) {
            g_gameOver = true;
        }
    }
}

extern "C" {

void web_reset(int humanColor) {
    g_board = Board();
    g_human = fromInt(humanColor);
    g_ai = Board::opponent(g_human);
    g_current = Board::Player::BLACK;
    g_passCount = 0;
    g_gameOver = false;
}

int web_get_cell(int row, int col) {
    if (!inRange(row) || !inRange(col)) {
        return -1;
    }
    char cell = g_board.getCell(row, col);
    if (cell == 'x') {
        return 1;
    }
    if (cell == 'o') {
        return 2;
    }
    return 0;
}

int web_get_current_player() {
    return g_current == Board::Player::BLACK ? 0 : 1;
}

int web_get_human_player() {
    return g_human == Board::Player::BLACK ? 0 : 1;
}

int web_is_game_over() {
    return g_gameOver ? 1 : 0;
}

int web_get_black_count() {
    return g_board.getBlackCount();
}

int web_get_white_count() {
    return g_board.getWhiteCount();
}

int web_has_any_move(int player) {
    Board::Player p = fromInt(player);
    return hasAnyMove(p) ? 1 : 0;
}

int web_is_valid_move(int row, int col, int player) {
    if (!inRange(row) || !inRange(col)) {
        return 0;
    }
    Board::Player p = fromInt(player);
    return g_board.isValidMove(row, col, p) ? 1 : 0;
}

enum {
    HUMAN_INVALID = 0,
    HUMAN_OK = 1,
    HUMAN_FORCED_PASS = 2,
    HUMAN_NOT_TURN = 3,
    HUMAN_GAME_OVER = 4
};

int web_human_move(int row, int col) {
    if (g_gameOver) {
        return HUMAN_GAME_OVER;
    }
    if (g_current != g_human) {
        return HUMAN_NOT_TURN;
    }
    if (!hasAnyMove(g_human)) {
        ++g_passCount;
        g_current = g_ai;
        evaluateGameState();
        return HUMAN_FORCED_PASS;
    }
    if (!inRange(row) || !inRange(col) || !g_board.isValidMove(row, col, g_human)) {
        return HUMAN_INVALID;
    }
    g_board.placePiece(row, col, g_human);
    g_passCount = 0;
    g_current = g_ai;
    evaluateGameState();
    return HUMAN_OK;
}

int web_human_pass() {
    if (g_gameOver) {
        return HUMAN_GAME_OVER;
    }
    if (g_current != g_human) {
        return HUMAN_NOT_TURN;
    }
    if (hasAnyMove(g_human)) {
        return HUMAN_INVALID;
    }
    ++g_passCount;
    g_current = g_ai;
    evaluateGameState();
    return HUMAN_FORCED_PASS;
}

int web_ai_move() {
    if (g_gameOver) {
        return -3;
    }
    if (g_current != g_ai) {
        return -2;
    }
    if (!hasAnyMove(g_ai)) {
        ++g_passCount;
        g_current = g_human;
        evaluateGameState();
        return -1;
    }
    playerfunc::Move move = playerfunc::aiMove(g_board, g_ai);
    if (move.first == -1 && move.second == -1) {
        ++g_passCount;
        g_current = g_human;
        evaluateGameState();
        return -1;
    }
    g_board.placePiece(move.first, move.second, g_ai);
    g_passCount = 0;
    g_current = g_human;
    evaluateGameState();
    return move.first * 8 + move.second;
}

int web_get_winner() {
    if (!g_gameOver) {
        return -1;
    }
    int black = g_board.getBlackCount();
    int white = g_board.getWhiteCount();
    if (black > white) {
        return 0;
    }
    if (white > black) {
        return 1;
    }
    return 2;
}

}
