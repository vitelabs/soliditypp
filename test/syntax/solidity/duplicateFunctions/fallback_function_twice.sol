contract C {
    uint x;
    fallback() external { x = 2; }
    fallback() external { x = 3; }
}
// ----
// DeclarationError 7301: (64-94): Only one fallback function is allowed.
