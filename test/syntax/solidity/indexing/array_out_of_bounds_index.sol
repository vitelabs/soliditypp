contract C {
  function f() public {
    bytes[32] memory a;
    a[64];
  }
}
// ----
// TypeError 3383: (65-70): Out of bounds array access.
