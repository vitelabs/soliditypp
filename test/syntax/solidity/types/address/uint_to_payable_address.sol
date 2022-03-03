contract C {
    function f(uint x) public pure returns (address payable) {
        return payable(address(uint160(x)));
    }
}
// ----
// TypeError 9640: (99-118): Explicit type conversion not allowed from "uint160" to "address".
