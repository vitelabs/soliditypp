contract C {
    function f() public returns (bool success) {
        (success, ) = (address("vite_61214664a1081e286152011570993a701735f5c2c12198ce63")).call{value: 30}("");
    }
}
