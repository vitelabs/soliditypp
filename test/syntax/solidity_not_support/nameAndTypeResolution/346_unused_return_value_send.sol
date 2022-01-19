contract test {
    function f() public {
        payable(address(0x12)).send(1);
    }
}
// ----
// ParserError 2314: (73-77): Expected identifier but got 'send'
