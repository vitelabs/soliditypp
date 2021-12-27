contract C {
    function f(bytes20 x) public pure returns (address payable) {
        return payable(address(x));
    }
}
// ----
// TypeError 9640: (102-112): Explicit type conversion not allowed from "bytes20" to "address".
