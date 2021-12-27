contract C {
    modifier costs(uint _amount) { require(msg.value >= _amount); _; }
    function f() costs(1 ether) public view {}
}
// ----
// TypeError 4006: (101-115): This modifier uses "msg.value" or "callvalue()" and thus the function has to be payable or internal.
