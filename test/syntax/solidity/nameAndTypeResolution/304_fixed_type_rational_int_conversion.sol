contract test {
    function f() public {
        fixed c = 3;
        ufixed d = 4;
        c; d;
    }
}
// ----
// UnimplementedFeatureError: Not yet implemented - FixedPointType.
// Warning 2018: (20-104): Function state mutability can be restricted to pure
