contract C {
    function f() pure public { abi.encode(2**500); }
}
// ----
// TypeError 8009: (55-61): Invalid rational number (too large or division by zero).
