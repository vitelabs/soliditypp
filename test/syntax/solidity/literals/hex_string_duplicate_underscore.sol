contract C {
    function f() public pure {
        hex"12__34";
    }
}
// ----
// ParserError 8936: (52-60): Invalid use of number separator '_'.
