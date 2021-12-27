contract C {
    function f() view public {
        C c;
        c.balance;
    }
}
// ----
// TypeError 3125: (65-74): Member "balance" not found or not visible after argument-dependent lookup in contract C. Use "address(c).balance" to access this address member.
