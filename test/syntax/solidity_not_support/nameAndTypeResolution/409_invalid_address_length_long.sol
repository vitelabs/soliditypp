contract C {
    function f() pure public {
        address x = "vite_61214664a1081e286152011570993a701735f5c2c12198ce6300";
        x;
    }
}
// ----
// TypeError 9574: (52-123): Type literal_string "vite_61214664a1081e286152011570993a701735f5c2c12198ce6300" is not implicitly convertible to expected type address.
