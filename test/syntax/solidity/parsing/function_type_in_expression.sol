contract test {
    function f(uint x, uint y) public returns (uint a) {}
    function g() public {
        function (uint, uint) internal returns (uint) f1 = f;
    }
}
// ----
// Warning 2072: (108-156): Unused local variable.
// Warning 2018: (78-167): Function state mutability can be restricted to pure
