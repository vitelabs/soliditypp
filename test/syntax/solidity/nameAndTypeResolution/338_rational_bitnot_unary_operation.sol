contract test {
    function f() public {
        ~fixed(3.5);
    }
}
// ----
// TypeError 4907: (50-61): Unary operator ~ cannot be applied to type fixed128x18
