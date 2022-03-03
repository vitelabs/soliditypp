contract C {
    function f() pure public {
        address x = "vite_214664a1081e286152011570993a701735f5c2c12198ce63";
        x;
    }
}
// ----
// SyntaxError 9429: (64-119): This looks like a Vite address but is not exactly 42 hex digits. It is 48 hex digits.
