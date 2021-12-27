contract C {
    struct S { uint a; }
    S s;
    function f() internal returns (S storage r)
    {
        r.a = 0;
        r = s;
    }
}
// ----
// TypeError 3464: (109-110): This variable is of storage pointer type and can be accessed without prior assignment, which would lead to undefined behaviour.
