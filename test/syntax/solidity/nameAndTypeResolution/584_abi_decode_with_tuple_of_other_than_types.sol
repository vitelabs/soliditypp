contract C {
    function f() pure public { abi.decode("", (0)); }
}
// ----
// TypeError 1039: (60-61): Argument has to be a type name.
