contract C {
    function f() view public {
        C c;
        c.send;
    }
}
// ----
// ParserError 2314: (67-71): Expected identifier but got 'send'
