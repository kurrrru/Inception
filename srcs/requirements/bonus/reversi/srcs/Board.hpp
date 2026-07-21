#pragma once

#include <vector>
#include <string>
#include <deque>
#include <stack>

class Board {
 public:
    enum class Player { BLACK = 0, WHITE = 1, OTHER = 2 };

    Board();
    Board(const Board& other);
    Board& operator=(const Board& other);
    ~Board();

    explicit Board(const std::vector<std::string>& board);

    static Player opponent(Player player);

    bool setBoard(const std::vector<std::string>& board);


    bool isValidMove(int row, int col, Player player) const;
    bool placePiece(int row, int col, Player player);

    void printBoard() const;
    void printHistory() const;

    int getBlackCount() const;
    int getWhiteCount() const;
    int getTotalCount() const;

    bool operator==(const Board& other) const;

    char getCell(int row, int col) const;

    std::string toString() const;

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

bool reachable(Board& now, const Board& goal, Board::Player player, std::deque<std::tuple<int, int, Board::Player>>& path);
