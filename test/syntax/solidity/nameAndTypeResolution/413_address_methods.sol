contract C {
    function f() public {
        address payable addr;
        uint b = addr.balance;
        (bool callSuc,) = addr.call("");
        (bool delegatecallSuc,) = addr.delegatecall("");
        addr.transfer("tti_5649544520544f4b454e6e40", 1);
        b; callSuc; delegatecallSuc;
    }
}
// ----
