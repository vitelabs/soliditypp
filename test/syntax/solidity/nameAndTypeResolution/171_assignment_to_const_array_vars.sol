contract C {
    uint[3] constant x = [uint(1), 2, 3];
}
// ----
// DeclarationError 9259: (17-53): Constants of non-value type not yet implemented.
