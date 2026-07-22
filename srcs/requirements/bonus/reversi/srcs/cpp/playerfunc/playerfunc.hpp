#pragma once

#include <utility>

#include "../Board.hpp"

namespace playerfunc {
    using Move = std::pair<int,int>; // {-1,-1} == pass

    // aiMove はアルファベータ法で次手を決める（デフォルト探索深さは内部で決める）
    Move aiMove(const Board& board, Board::Player player);
}