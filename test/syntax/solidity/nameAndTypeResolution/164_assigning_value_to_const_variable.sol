contract Foo {
    function changeIt() public { x = 9; }
    uint constant x = 56;
}
// ----
// TypeError 6520: (48-49): Cannot assign to a constant variable.
