contract c {
    uint a = "abc";
}
// ----
// TypeError 7407: (26-31): Type literal_string "abc" is not implicitly convertible to expected type uint256.
