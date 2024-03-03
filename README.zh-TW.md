# Qt五子棋
[![英文](https://img.shields.io/badge/語言-英文-red.svg)](https://github.com/SXKA/Qt-Gomoku/blob/master/README.md)
[![繁體中文](https://img.shields.io/badge/語言-繁體中文-green.svg)](https://github.com/SXKA/Qt-Gomoku/blob/master/README.zh-TW.md)

無禁手規則

詳細介紹：https://github.com/SXKA/Qt-Gomoku/wiki
# 使用方法
Include src/search/engine.h to use search engine.


```C++
// 建構一個引擎.
Search::Engine engine;

// 設定搜尋參數.
Search::LIMIT_DEPTH = depth; // Extensions will not be limited.
Search::MC_C = mc_c; // Multi-Cut number of cutoffs.
Search::MC_M = mc_m; // Multi-Cut number of moves.
Search::MC_R = mc_r; // Multi-Cut depth reduction.
Search::R = r;       // Null move pruning depth reduction.

// 黑方落子
engine.move({7, 7}, Black);

// 搜尋白方最佳著法
const auto bestMove = engine.bestMove(White);

// 確認最佳著法是合法的（Engine::bestMove回傳的move一定是合法的）
const auto legal = Search::Engine::isLegal(bestMove);

if (legal) {
    // 白方落子.
    engine.move(bestMove, white);
}

// 取得棋局狀態
const auto status = engine.gameStatus(bestMove, White);

// 根據狀態決定遊戲是否繼續
switch (status) {
    case Draw:
        // 處理平局並不再使用此引擎
        break;
    case Undecided:
        // 繼續遊戲
        break;
    case Win:
        // 處理白勝並不再使用此引擎
        break;
}

// 悔一步棋，請確定有棋可悔
engine.undo(1);
```
## 需求
- Qt 6.5.2
## 參考
- Enhanced Forward Pruning (搜尋)
- https://github.com/kimlongli/FiveChess (評估)
- https://github.com/Kenny-ting/Chess-Game-2020 (介面)
## 特色
- 使用非同步進行搜尋，避免主執行緒blocking
- 搜尋深度達到 12 ply
- 主要變體搜尋 (PVS)
- 同形表
- 空著裁剪
- Multi-Cut
- 延伸
## 主介面
![圖片](https://github.com/SXKA/Qt-Gomoku/blob/master/Qt-Gomoku/resource/picture/mainwindow.png)
## 遊戲介面
<div align=center><img src=https://github.com/SXKA/Qt-Gomoku/blob/master/Qt-Gomoku/resource/picture/gamewindow.png></div>
