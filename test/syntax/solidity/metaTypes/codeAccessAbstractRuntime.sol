contract Test {
    function runtime() public pure returns (bytes memory) {
        return type(Other).runtimeCode;
    }
}
abstract contract Other {
    function f(uint) public returns (uint);
}
// ----
// TypeError 9582: (91-114): Member "runtimeCode" not found or not visible after argument-dependent lookup in type(contract Other).
