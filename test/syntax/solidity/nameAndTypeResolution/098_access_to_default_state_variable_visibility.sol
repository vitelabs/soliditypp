contract c {
    uint a;
}
contract d {
    function g() public { c(address(0)).a(); }
}
// ----
// TypeError 9582: (66-81): Member "a" not found or not visible after argument-dependent lookup in contract c.
