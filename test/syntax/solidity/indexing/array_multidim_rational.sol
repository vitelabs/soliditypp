contract C {
  function f() public {
    bytes[32] memory a;
    a[8**90][8**90][8**90*0.1];
  }
}
// ----
// TypeError 7407: (67-72): Type int_const 1897...(74 digits omitted)...1424 is not implicitly convertible to expected type uint256. Literal is too large to fit in uint256.
// TypeError 7407: (74-79): Type int_const 1897...(74 digits omitted)...1424 is not implicitly convertible to expected type uint256. Literal is too large to fit in uint256.
// TypeError 7407: (81-90): Type rational_const 9485...(73 digits omitted)...5712 / 5 is not implicitly convertible to expected type uint256.
// TypeError 6318: (65-91): Index expression cannot be represented as an unsigned integer.
