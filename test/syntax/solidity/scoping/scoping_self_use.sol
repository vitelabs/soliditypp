contract test {
    function f() pure public {
        uint a = a;
    }
}
// ----
// DeclarationError 7576: (64-65): Undeclared identifier. "a" is not (or not yet) visible at this point.
