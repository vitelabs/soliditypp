contract C {
    uint x;
    fallback() external view { x = 2; }
}
// ----
// TypeError 4575: (29-64): Fallback function must be payable or non-payable, but is "view".
