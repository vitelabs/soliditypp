contract C {
  function f() internal {
    {
      (uint a, uint b, uint c) = (1, 2, 3);
    }
    a;
  }
}
// ----
// DeclarationError 7576: (99-100): Undeclared identifier.
