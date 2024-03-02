# Qt五子棋
[![英文](https://img.shields.io/badge/語言-英文-red.svg)](https://github.com/SXKA/Qt-Gomoku/blob/master/README.md)
[![繁體中文](https://img.shields.io/badge/語言-繁體中文-green.svg)](https://github.com/SXKA/Qt-Gomoku/blob/master/README.zh-TW.md)

無禁手規則

詳細介紹：https://github.com/SXKA/Qt-Gomoku/wiki
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
