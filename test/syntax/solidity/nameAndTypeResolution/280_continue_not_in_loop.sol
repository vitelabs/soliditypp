contract C {
    function f() public {
        if (true)
            continue;
    }
}
// ----
// SyntaxError 4123: (69-77): "continue" has to be in a "for" or "while" loop.
