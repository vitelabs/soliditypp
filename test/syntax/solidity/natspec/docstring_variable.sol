contract C {
    function f() public pure returns (uint) {
        /// @title example of title
        /// @author example of author
        /// @notice example of notice
        /// @dev example of dev
        /// @param example of param
        /// @return example of return
        uint state = 42;
        return state;
    }
}
// ----
// ParserError 2837: (290-295): Only state variables or file-level variables can have a docstring.
