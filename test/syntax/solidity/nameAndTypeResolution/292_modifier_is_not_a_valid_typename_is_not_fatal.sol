contract test {
    modifier mod() { _; }

    function f() public {
        mod g;
        g = f;
    }
}
// ----
// TypeError 5172: (77-80): Name has to refer to a struct, enum or contract.
