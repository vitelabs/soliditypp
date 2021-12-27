contract C {
    function f() public pure {
        address payable a = address(uint160(0));
        address payable b = address(bytes20(0));
        address payable c = address(this);
    }
}
// ----
// TypeError 9640: (72-91): Explicit type conversion not allowed from "uint160" to "address".
// TypeError 9574: (52-91): Type address is not implicitly convertible to expected type address payable.
// TypeError 9640: (121-140): Explicit type conversion not allowed from "bytes20" to "address".
// TypeError 9574: (101-140): Type address is not implicitly convertible to expected type address payable.
// TypeError 9574: (150-183): Type address is not implicitly convertible to expected type address payable.
