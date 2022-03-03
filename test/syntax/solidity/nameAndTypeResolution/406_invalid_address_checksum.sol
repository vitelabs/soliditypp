contract C {
    function f() pure public {
        address x = "vite_61214664a1081e286152011570993a701735f5c2c12198ce00";
        x;
    }
}
// ----
// SyntaxError 9429: (64-121): This looks like a Vite address but has an invalid checksum.
