contract C {
    function f() pure public {
        C c;
        c.delegatecall;
    }
}
// ----
// TypeError 3125: (65-79): Member "delegatecall" not found or not visible after argument-dependent lookup in contract C. Use "address(c).delegatecall" to access this address member.
