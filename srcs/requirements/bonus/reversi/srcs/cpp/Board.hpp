#pragma once

#include <vector>
#include <deque>

class Board {
 public:
    enum class Player { BLACK = 0, WHITE = 1, OTHER = 2 };

    Board();
    Board(const Board& other);
    Board& operator=(const Board& other);
    ~Board();

    static Player opponent(Player player);

    bool isValidMove(int row, int col, Player player) const;
    bool placePiece(int row, int col, Player player);

    int getBlackCount() const;
    int getWhiteCount() const;
    int getTotalCount() const;

    char getCell(int row, int col) const;

    void undo();

 private:
    std::vector<std::vector<char>> _board;
    std::deque<std::vector<std::tuple<int, int, char>>> _history; // (row, col, previous state)
    const int _width = 8;
    const int _height = 8;
    const char _emptyCell = '.';
    const char _blackCell = 'x';
    const char _whiteCell = 'o';
    int _blackCount;
    int _whiteCount;
};
