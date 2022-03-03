contract C {
    function f() view public {
        this.send;
    }
}
// ----
// ParserError 2314: (57-61): Expected identifier but got 'send'
