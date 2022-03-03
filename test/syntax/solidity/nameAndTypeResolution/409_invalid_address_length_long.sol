contract C {
    function f() pure public {
        address x = "vite_61214664a1081e286152011570993a701735f5c2c12198ce6300";
        x;
    }
}
// ----
// SyntaxError 9429: (64-123): This looks like a Vite address but is not exactly 42 hex digits. It is 52 hex digits.
