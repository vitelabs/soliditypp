contract C {
    function f() view public {
        this.send;
    }
}
// ----
// TypeError 3125: (52-61): Member "send" not found or not visible after argument-dependent lookup in contract C. Use "address(this).send" to access this address member.
