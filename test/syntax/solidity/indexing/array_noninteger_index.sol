contract C {
  function f() public {
    bytes[32] memory a;
    a[888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888888];
  }
}
// ----
// TypeError 7407: (67-178): Type int_const 8888...(103 digits omitted)...8888 is not implicitly convertible to expected type uint256. Literal is too large to fit in uint256.
