contract test {
    function f() public {
        address(0x12).delegatecall("abc");
    }
}
// ----
// Warning 9302: (50-83): Return value of low-level calls not used.
