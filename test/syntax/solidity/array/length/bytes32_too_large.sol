contract C {
    bytes32[8**90] ids;
}
// ----
// TypeError 5462: (25-30): Invalid array length, expected integer literal or constant expression.
