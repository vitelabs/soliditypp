contract C {
    function f() public {
        address(this).send(10);
    }
}

// ----
// ParserError 2314: (61-65): Expected identifier but got 'send'
