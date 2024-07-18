# Qt-Gomoku
[![EN](https://img.shields.io/badge/lang-EN-red.svg)](https://github.com/SXKA/Qt-Gomoku/blob/master/README.md)
[![ZH](https://img.shields.io/badge/lang-ZH--HANT--TW-green.svg)](https://github.com/SXKA/Qt-Gomoku/blob/master/README.zh-TW.md)

Free-style Gomoku rule.

Detail: https://github.com/SXKA/Qt-Gomoku/wiki (Traditional Chinese)

EXE file: https://github.com/SXKA/Qt-Gomoku/releases 
# Usage
Include src/search/engine.h to use search engine.


```C++
// Create a engine.
Search::Engine engine;

// Set search parameters.
Search::LIMIT_DEPTH = depth; // Extensions will not be limited.
Search::MC_C = mc_c; // Multi-Cut number of cutoffs.
Search::MC_M = mc_m; // Multi-Cut number of moves.
Search::MC_R = mc_r; // Multi-Cut depth reduction.
Search::VCF_DEPTH = vcf_depth; // VCF depth.

// Make a move for black.
engine.move({7, 7}, Black);

// Search for the best move for white.
const auto bestMove = engine.bestMove(White);

// Check the best move is legal. (Engine::bestMove return should be legal.)
const auto legal = Search::Engine::isLegal(bestMove);

if (legal) {
    // Make a move for white.
    engine.move(bestMove, White);
}

// Get Gomoku game status.
const auto status = engine.gameStatus(bestMove, White);

// Determine whether to continue the game based on the status.
switch (status) {
    case Draw:
        // Dealing with draw and don't continue use the engine.
        break;
    case Undecided:
        // Continue the game.
        break;
    case Win:
        // Dealing with White's victory and don't continue use the engine.
        break;
}

// Undo a move. Please make sure there are moves that can be undo.
engine.undo(1);
```
## Requirements
- Qt 6.5.2
## References
- Enhanced Forward Pruning (Search)
- https://github.com/kimlongli/FiveChess (Evaluation)
- https://github.com/Kenny-ting/Chess-Game-2020 (UI)
## Features
- Searching depth reaches 12 ply.
- Principal Variation Search (PVS)
- Victory of Continuous Four (VCF) search
- Transposition table
- Null Move Pruning
- Multi-Cut
- Extensions
## Homepage
![image](https://github.com/SXKA/Qt-Gomoku/blob/master/Qt-Gomoku/resource/picture/mainwindow.png)
## Game UI
<div align=center><img src=https://github.com/SXKA/Qt-Gomoku/blob/master/Qt-Gomoku/resource/picture/gamewindow.png></div>
