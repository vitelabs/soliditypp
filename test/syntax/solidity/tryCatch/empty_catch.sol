contract C {
    function f() public returns (uint, uint) {
        try this.f() {

        } catch () {

        }
    }
}
// ----
// ParserError 3546: (101-102): Expected type name
