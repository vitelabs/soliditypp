contract C {
    function f(uint x) pure public {
        if (x > 7)
            revert;
    }
}
// ----
// TypeError 2144: (81-87): No matching declaration found after variable lookup.
