contract C {
    event SomeEvent();
    function a() public {
        (emit SomeEvent(), 7);
    }
}
// ----
// ParserError 6933: (71-75): Expected primary expression.
