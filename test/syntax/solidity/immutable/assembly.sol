contract C {
    uint immutable x;
    function f() public view {
        uint t;
        assembly {
            t := x
        }
    }
}
// ----
// TypeError 3773: (118-119): Assembly access to immutable variables is not supported.
