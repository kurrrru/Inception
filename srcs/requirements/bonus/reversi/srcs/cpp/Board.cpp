#include "Board.hpp"

#include <stdexcept>
#include <vector>

Board::Board() : _blackCount(2), _whiteCount(2) {
    _board.assign(_height, std::vector<char>(_width, _emptyCell));
    _board[3][3] = _whiteCell;
    _board[3][4] = _blackCell;
    _board[4][3] = _blackCell;
    _board[4][4] = _whiteCell;
}

Board::Board(const Board& other) : _board(other._board), _history(other._history), _blackCount(other._blackCount),
    _whiteCount(other._whiteCount) {}

Board& Board::operator=(const Board& other) {
    if (this != &other) {
        _board = other._board;
        _history = other._history;
        _blackCount = other._blackCount;
        _whiteCount = other._whiteCount;
    }
    return *this;
}

Board::~Board() {}

Board::Player Board::opponent(Player player) {
    return (player == Player::BLACK) ? Player::WHITE : Player::BLACK;
}

bool Board::isValidMove(int row, int col, Player player) const {
    if (row < 0 || row >= _height || col < 0 || col >= _width || _board[row][col] != _emptyCell) {
        return false;
    }
    char playerCell = (player == Player::BLACK) ? _blackCell : _whiteCell;
    char opponentCell = (player == Player::BLACK) ? _whiteCell : _blackCell;
    const int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},          {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    for (int d = 0; d < 8; ++d) {
        int r = row + directions[d][0];
        int c = col + directions[d][1];
        bool foundOpponent = false;
        while (r >= 0 && r < _height && c >= 0 && c < _width) {
            if (_board[r][c] == opponentCell) {
                foundOpponent = true;
            } else if (_board[r][c] == playerCell) {
                if (foundOpponent) {
                    return true;
                } else {
                    break;
                }
            } else {
                break;
            }
            r += directions[d][0];
            c += directions[d][1];
        }
    }
    return false;
}

bool Board::placePiece(int row, int col, Player player) {
    if (!isValidMove(row, col, player)) {
        return false;
    }
    char playerCell = (player == Player::BLACK) ? _blackCell : _whiteCell;
    char opponentCell = (player == Player::BLACK) ? _whiteCell : _blackCell;
    _board[row][col] = playerCell;
    if (player == Player::BLACK) {
        ++_blackCount;
    } else {
        ++_whiteCount;
    }
    const int directions[8][2] = {
        {-1, -1}, {-1, 0}, {-1, 1},
        {0, -1},          {0, 1},
        {1, -1},  {1, 0},  {1, 1}
    };
    std::vector<std::tuple<int, int, char>> changes;
    changes.push_back(std::make_tuple(row, col, _emptyCell));
    for (int d = 0; d < 8; ++d) {
        int r = row + directions[d][0];
        int c = col + directions[d][1];
        std::vector<std::pair<int, int>> toFlip;
        while (r >= 0 && r < _height && c >= 0 && c < _width) {
            if (_board[r][c] == opponentCell) {
                toFlip.push_back(std::make_pair(r, c));
            } else if (_board[r][c] == playerCell) {
                for (std::size_t i = 0; i < toFlip.size(); ++i) {
                    std::pair<int, int> p = toFlip[i];
                    changes.push_back(std::make_tuple(p.first, p.second, opponentCell));
                    _board[p.first][p.second] = playerCell;
                    if (player == Player::BLACK) {
                        ++_blackCount;
                        --_whiteCount;
                    } else {
                        ++_whiteCount;
                        --_blackCount;
                    }
                }
                break;
            } else {
                break;
            }
            r += directions[d][0];
            c += directions[d][1];
        }
    }
    _history.push_back(changes);
    return true;
}

int Board::getBlackCount() const {
    return _blackCount;
}

int Board::getWhiteCount() const {
    return _whiteCount;
}

int Board::getTotalCount() const {
    return _blackCount + _whiteCount;
}

char Board::getCell(int row, int col) const {
    if (row < 0 || row >= _height || col < 0 || col >= _width) {
        throw std::out_of_range("Cell position out of range");
    }
    return _board[row][col];
}

void Board::undo() {
    if (_history.empty()) {
        return;
    }
    std::vector<std::tuple<int, int, char>> changes = _history.back();
    _history.pop_back();
    for (const auto& change : changes) {
        int r = std::get<0>(change);
        int c = std::get<1>(change);
        char prevState = std::get<2>(change);
        char currentState = _board[r][c];
        if (currentState == _blackCell) {
            --_blackCount;
        } else if (currentState == _whiteCell) {
            --_whiteCount;
        }
        if (prevState == _blackCell) {
            ++_blackCount;
        } else if (prevState == _whiteCell) {
            ++_whiteCount;
        }
        _board[r][c] = prevState;
    }
}
