contract C {
    function foo() pure internal {
        address(10).staticcall{value: 7, token: "tti_5649544520544f4b454e6e40"}("");
    }
}
// ====
// EVMVersion: >=byzantium
// ----
// TypeError 2842: (56-127): Cannot set option "value" for staticcall.
// TypeError 2842: (56-127): Cannot set option "token" for staticcall.
// Warning 9302: (56-131): Return value of low-level calls not used.
