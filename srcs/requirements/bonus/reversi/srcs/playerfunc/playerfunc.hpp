#pragma once

#include <utility>

#include <src/Board.hpp>

namespace playerfunc {
    using Move = std::pair<int,int>; // {-1,-1} == pass

    // humanMove は標準入力を使って人間から1手を受け取り Move を返す
    // player パラメータでどちらの手番かを受け取る
    // 入力フォーマットの例: "A3"（列A-H、行0-7）、または "3 4"（row col）、"p" でパス
    // ただし、着手可能箇所が存在する場合はパスできない。入力が不正・不可能なら再入力を要求する。
    Move humanMove(const Board& board, Board::Player player);
    // aiMove はアルファベータ法で次手を決める（デフォルト探索深さは内部で決める）
    Move aiMove(const Board& board, Board::Player player);
}