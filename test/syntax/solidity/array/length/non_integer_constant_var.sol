contract C {
    bool constant LEN = true;
    uint[LEN] ids;
}
// ----
// TypeError 5462: (52-55): Invalid array length, expected integer literal or constant expression.
