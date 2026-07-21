#include <src/Board.hpp>

#include <iostream>
#include <iomanip>
#include <stdexcept>
#include <string>
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

Board::Board(const std::vector<std::string>& board) : _blackCount(0), _whiteCount(0) {
    if (!setBoard(board)) {
        throw std::invalid_argument("Invalid board configuration");
    }
}

Board::Player Board::opponent(Player player) {
    return (player == Player::BLACK) ? Player::WHITE : Player::BLACK;
}

bool Board::setBoard(const std::vector<std::string>& board) {
    if (board.size() != static_cast<std::size_t>(_height)) {
        return false;
    }
    int blackCount = 0;
    int whiteCount = 0;
    std::vector<std::vector<char>> newBoard(_height, std::vector<char>(_width, _emptyCell));
    for (int r = 0; r < _height; ++r) {
        if (board[r].size() != static_cast<std::size_t>(_width)) {
            return false;
        }
        for (int c = 0; c < _width; ++c) {
            char cell = board[r][c];
            if (cell == _blackCell) {
                ++blackCount;
            } else if (cell == _whiteCell) {
                ++whiteCount;
            } else if (cell != _emptyCell) {
                return false;
            }
            newBoard[r][c] = cell;
        }
    }
    std::swap(_board, newBoard);
    _history = std::deque<std::vector<std::tuple<int, int, char>>>();
    _blackCount = blackCount;
    _whiteCount = whiteCount;
    return true;
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

void Board::printBoard() const {
    std::cout << " ";
    for (int c = 0; c < _width; ++c) {
        std::cout << " " << static_cast<char>('A' + c);
    }
    std::cout << "\n";
    for (int r = 0; r < _height; ++r) {
        std::cout << r + 1;
        for (int c = 0; c < _width; ++c) {
            std::cout << " " << _board[r][c];
        }
        std::cout << "\n";
    }
    std::cout << "Black (x): " << _blackCount << "  White (o): " << _whiteCount << std::endl;
}

void Board::printHistory() const {
    std::cout << "Move History (most recent last):\n";
    for (const auto& changes : _history) {
        std::cout << 'a' + std::get<1>(changes[0]) << (std::get<0>(changes[0]) + 1);
    }
    std::cout << std::endl;
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

bool Board::operator==(const Board& other) const {
    return _board == other._board;
}

char Board::getCell(int row, int col) const {
    if (row < 0 || row >= _height || col < 0 || col >= _width) {
        throw std::out_of_range("Cell position out of range");
    }
    return _board[row][col];
}

std::string Board::toString() const {
    std::string result;
    for (int r = 0; r < _height; ++r) {
        for (int c = 0; c < _width; ++c) {
            result += _board[r][c];
        }
    }
    return result;
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
