contract C {
    struct S { uint x; uint[] y; }
    S constant x = S(5, new uint[](4));
}
// ----
// DeclarationError 9259: (52-86): Constants of non-value type not yet implemented.
