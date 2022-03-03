contract C {
    modifier m(uint _amount, uint _avail) { require(_avail >= _amount); _; }
    function f() m(1 ether, msg.value) public view {}
}
// ----
// TypeError 5887: (118-127): "msg.value" and "callvalue()" can only be used in payable public functions. Make the function "payable" or use an internal function to avoid this error.
