contract C {
    function (uint) external returns (uint) x;
    function f() public {
        x{gas: 2}(1);
    }
}
// ----
// TypeError 100201: (94-103): Cannot set option "gas" in Solidity++. Valid options are "token" and "value".

