contract C {
    function f(uint require) pure public {
        require = 2;
    }
}
// ----
// Warning 2319: (28-40): This declaration shadows a builtin symbol.
