# Qt-Gomoku
Game UI is from https://github.com/Kenny-ting/Chess-Game-2020

Algorithm reference to https://github.com/kimlongli/FiveChess and https://hci.iwr.uni-heidelberg.de/system/files/private/downloads/1935772097/report_qingyang-cao_enhanced-forward-pruning.pdf
## Features
- Written in modern C++.
- Use async way to call AI to avoid main thread blocking.
- Introduce null move pruning and PVS.
- On the basis of the limited number of game tree branches, the depth penalty is added.
- Search depth reaches 12 layers.
## Homepage
![image](https://github.com/SXKA/Qt-Gomoku/blob/master/Qt-Gomoku/resource/picture/mainwindow.png)
## Game UI
<div align=center><img src=https://github.com/SXKA/Qt-Gomoku/blob/master/Qt-Gomoku/resource/picture/gamewindow.png></div>
