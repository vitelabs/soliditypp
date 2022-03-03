abstract contract a {
    function f() virtual public;
}
contract b is a {
    function f() public virtual override { super.f(); }
}
contract c is a,b {
    // No error here.
    function f() public override(a, b) { super.f(); }
}
// ----
// TypeError 9582: (118-125): Member "f" not found or not visible after argument-dependent lookup in type(contract super b).
