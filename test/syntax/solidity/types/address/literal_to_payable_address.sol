contract C {
    function f() public pure {
        // We allow an exception for 0
        address payable a = payable(0);
        a = payable(address("vite_61214664a1081e286152011570993a701735f5c2c12198ce63"));
        address payable b = payable("vite_61214664a1081e286152011570993a701735f5c2c12198ce63");
        b = payable("vite_61214664a1081e286152011570993a701735f5c2c12198ce63");
    }
}
