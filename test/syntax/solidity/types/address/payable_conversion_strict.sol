contract C {
    function f() public pure {
        address payable a = payable(address(uint160(0)));
        address payable b = payable(address(bytes20(0)));
        address payable c = payable(address(2));
        // hex literal that is only 15 bytes long
        address payable d = payable(address(0x002190356cBB839Cbe05303d7705Fa));

        uint160 a1 = uint160(address(payable(0)));
        bytes20 b1 = bytes20(address(payable(0)));

        a; b; c; d; a1; b1;
    }
}
// ----
// TypeError 9640: (80-99): Explicit type conversion not allowed from "uint160" to "address".
// TypeError 9640: (138-157): Explicit type conversion not allowed from "bytes20" to "address".
// TypeError 9640: (361-389): Explicit type conversion not allowed from "address" to "uint160".
// TypeError 9640: (412-440): Explicit type conversion not allowed from "address" to "bytes20".
