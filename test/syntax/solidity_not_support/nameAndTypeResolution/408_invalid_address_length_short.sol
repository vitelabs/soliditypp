contract C {
    function f() pure public {
        address x = "vite_214664a1081e286152011570993a701735f5c2c12198ce63";
        x;
    }
}
// ----
// SyntaxError 9429: (64-105): This looks like an address but is not exactly 40 hex digits. It is 39 hex digits. If this is not used as an address, please prepend '00'. For more information please see https://docs.soliditylang.org/en/develop/types.html#address-literals
