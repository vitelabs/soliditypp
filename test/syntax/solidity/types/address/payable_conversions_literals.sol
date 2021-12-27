contract C {
    function f() public pure {
        // 0 is okay, because it's an exception
        address payable a = payable(0);

        // address literals have type address
        address payable b = payable("vite_61214664a1081e286152011570993a701735f5c2c12198ce63");

        address payable c = payable(address(2));

        a; b; c;
    }
}
