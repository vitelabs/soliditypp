contract Test {
    function f() public { Test x = new Test(); }
}
// ----
// TypeError 4579: (51-59): Circular reference for contract creation (cannot create instance of derived or same contract).
