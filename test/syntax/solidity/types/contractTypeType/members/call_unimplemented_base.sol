abstract contract B {
    function f() public virtual;
}
contract C is B {
    function f() public override {
        B.f();
    }
}
// ----
// TypeError 7501: (118-123): Cannot call unimplemented base function.
