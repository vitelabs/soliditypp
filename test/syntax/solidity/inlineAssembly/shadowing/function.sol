contract C {
    function f() public pure {
        assembly {
            let f := 1
        }
    }
}
// ----
// DeclarationError 3859: (79-80): This declaration shadows a declaration outside the inline assembly block.
