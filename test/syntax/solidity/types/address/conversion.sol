contract C {
    function f() public pure returns (address) {
        return address(2**160 -1);
    }
    function g() public pure returns (address) {
        return address(type(uint160).max);
    }
}
// ----
// TypeError 9640: (167-193): Explicit type conversion not allowed from "uint160" to "address".
