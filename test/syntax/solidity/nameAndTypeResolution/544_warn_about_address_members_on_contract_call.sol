contract C {
    function f() view public {
        this.call;
    }
}
// ----
// TypeError 3125: (52-61): Member "call" not found or not visible after argument-dependent lookup in contract C. Use "address(this).call" to access this address member.
