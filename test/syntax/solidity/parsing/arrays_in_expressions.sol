contract c {
    function f() public { c[10] storage a = 7; uint8[10 * 2] storage x; }
}
// ----
// TypeError 9574: (39-58): Type int_const 7 is not implicitly convertible to expected type contract c[10] storage pointer.
