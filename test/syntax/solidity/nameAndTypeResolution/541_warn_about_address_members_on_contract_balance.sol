contract C {
    function f() view public {
        this.balance;
    }
}
// ----
// TypeError 3125: (52-64): Member "balance" not found or not visible after argument-dependent lookup in contract C. Use "address(this).balance" to access this address member.
