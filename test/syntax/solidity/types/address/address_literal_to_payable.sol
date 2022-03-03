contract C {
    function f() public pure {
        address payable a = payable(address("vite_61214664a1081e286152011570993a701735f5c2c12198ce63"));
        address payable b = payable("vite_61214664a1081e286152011570993a701735f5c2c12198ce63");
        a = b;
        b = a;
    }
}
// ----
