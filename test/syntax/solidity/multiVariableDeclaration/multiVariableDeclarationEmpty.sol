contract C {
    function f() public pure {
        (uint a, uint b) = f();
        (uint c) = f();
        uint d = f();
    }
}
// ----
// TypeError 7364: (52-74): Different number of components on the left hand side (2) than on the right hand side (0).
// TypeError 7364: (84-98): Different number of components on the left hand side (1) than on the right hand side (0).
// TypeError 7364: (108-120): Different number of components on the left hand side (1) than on the right hand side (0).
