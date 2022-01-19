contract C {
    function f() public {
        (tx.origin).send(10);
    }
}
// ----
// ParserError 2314: (59-63): Expected identifier but got 'send'
