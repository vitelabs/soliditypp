// This restriction might be lifted in the future
contract C {
  function f() {
    abi.decode("abc", (bytes storage));
  }
}
// ----
// ParserError 2314: (109-116): Expected ',' but got 'storage'
