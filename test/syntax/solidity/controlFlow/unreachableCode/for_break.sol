contract C {
    function f() public pure {
        for (uint a = 0; a < 1; a++) {
            break;
            uint b = 42; b;
        }
        return;
    }
}
// ----
// Warning 5740: (76-79): Unreachable code.
// Warning 5740: (114-128): Unreachable code.
