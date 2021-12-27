// This used to be a test for a.transfer to generate a warning
// because A does not have a payable fallback function.

contract A {
    receive() payable external {}
}

contract B {
    A a;

    fallback() external {
        payable(a).transfer("tti_5649544520544f4b454e6e40", 100);
    }
}
// ----
