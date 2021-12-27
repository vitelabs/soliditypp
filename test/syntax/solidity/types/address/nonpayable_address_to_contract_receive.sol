contract C {
  function f() public pure returns (C c) {
    address a = address(2);
    c = C(a);
  }
  receive() external payable {
  }
}
// ----
// TypeError 7398: (92-96): Explicit type conversion not allowed from non-payable "address" to "contract C", which has a payable fallback function.
