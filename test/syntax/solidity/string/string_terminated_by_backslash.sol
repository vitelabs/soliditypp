contract test {
    function f() public pure returns (bytes32) {
        bytes32 escapeCharacters = "text \";
        return escapeCharacters;
    }
}
// ----
// ParserError 8936: (100-109): Expected string end-quote.
