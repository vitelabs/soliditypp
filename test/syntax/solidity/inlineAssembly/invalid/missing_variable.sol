contract C {
  function f() public pure {
    assembly {
      x := 1
    }
  }
}
// ----
// DeclarationError 4634: (63-64): Variable not found or variable not lvalue.
