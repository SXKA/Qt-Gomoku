# Qt-Gomoku
Game UI is from https://github.com/Kenny-ting/Chess-Game-2020, Free-style Gomoku rule.
## Requirements
- Qt 6.5.2
## Algorithm references
- https://github.com/kimlongli/FiveChess
- https://hci.iwr.uni-heidelberg.de/system/files/private/downloads/1935772097/report_qingyang-cao_enhanced-forward-pruning.pdf
## Features
- Written in modern C++.
- Using async way to call engine to avoid main thread blocking.
- Introducing null move pruning and PVS.
- Searching depth reaches 10 ply.
## Homepage
![image](https://github.com/SXKA/Qt-Gomoku/blob/master/Qt-Gomoku/resource/picture/mainwindow.png)
## Game UI
<div align=center><img src=https://github.com/SXKA/Qt-Gomoku/blob/master/Qt-Gomoku/resource/picture/gamewindow.png></div>
