contract C {
    function f() public {
        (msg.sender).send(10);
    }
}
// ----
// ParserError 2314: (60-64): Expected identifier but got 'send'
