// The constructor of a base class should not be visible in the derived class
contract A { constructor(string memory) { } }
contract B is A {
  function f() pure public {
    A x = A(address(0)); // convert from address
    string memory y = "ab";
    A(y); // call as a function is invalid
    x;
  }
}
// ----
// TypeError 3656: (124-303): Contract "B" should be marked as abstract.
// TypeError 9640: (252-256): Explicit type conversion not allowed from "string memory" to "contract A".
