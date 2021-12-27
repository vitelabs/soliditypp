// This used to crash with some compiler versions.
contract SomeContract {

  uint public balance = 0;

  function balance(uint number) public {}

  function doSomething() public {
    balance(3);
  }
}
// ----
// DeclarationError 2333: (106-145): Identifier already declared.
// Warning 2319: (78-101): This declaration shadows a builtin symbol.
// TypeError 5704: (185-195): Type is not callable
