contract C {
    function f() public {
        require(payable(this).send(2));
        selfdestruct(payable(this));
    }
    receive() payable external {}
}
// ----
// ParserError 2314: (69-73): Expected identifier but got 'send'
