#ifndef TYPES_H
#define TYPES_H

enum Score {
    One = 20,
    Two = 120,
    Three = 720,
    Four = 720,
    OpenFour = 4320,
    Five = 50000,
    Max = 10000000,
    Min = -Max
};

enum Status { Draw = -1, Undecided, Win };

enum Stone { Black = -1, Empty, White };

#endif
