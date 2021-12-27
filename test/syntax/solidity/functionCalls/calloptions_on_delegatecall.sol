contract C {
    function foo() pure internal {
        address(10).delegatecall{value: 7}("");
    }
}
// ----
// TypeError 6189: (56-90): Cannot set option "value" for delegatecall.
// Warning 9302: (56-94): Return value of low-level calls not used.
