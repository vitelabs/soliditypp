contract C {
    function f() public view {
        address a1 = address(uint160(0));
        address a2 = address(bytes20(0));
        address a3 = address(this);

        // Trivial conversions
        address payable a4 = payable(address(uint160(0)));
        address payable a5 = payable(address(bytes20(0)));
        address payable a6 = payable(address(this));

        a1; a2; a3; a4; a5; a6;
    }

    // to make payable(this) work
    receive() payable external {
    }
}
// ----
// TypeError 9640: (65-84): Explicit type conversion not allowed from "uint160" to "address".
// TypeError 9640: (107-126): Explicit type conversion not allowed from "bytes20" to "address".
// TypeError 9640: (233-252): Explicit type conversion not allowed from "uint160" to "address".
// TypeError 9640: (292-311): Explicit type conversion not allowed from "bytes20" to "address".
