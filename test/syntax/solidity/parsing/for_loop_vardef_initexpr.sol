contract test {
    function fun(uint256 a) public {
        for (uint256 i = 0; i < 10; i++) {
            uint256 x = i; break; continue;
        }
    }
}
// ----
// Warning 5740: (89-92): Unreachable code.
// Warning 5740: (130-138): Unreachable code.
// Warning 5667: (33-42): Unused function parameter. Remove or comment out the variable name to silence this warning.
// Warning 2072: (108-117): Unused local variable.
// Warning 2018: (20-155): Function state mutability can be restricted to pure
