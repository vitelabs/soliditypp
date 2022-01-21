contract C {
    function balance() public returns (uint) {
        this.balance; // to avoid pureness warning
        return 1;
    }
    function transfer(vitetoken _token, uint amount) public {
        payable(this).transfer(_token, amount); // to avoid pureness warning
    }
    receive() payable external {
    }
}
contract D {
    function f() public {
        uint x = (new C()).balance();
        x;
        (new C()).transfer("tti_5649544520544f4b454e6e40", 5);
    }
}
// ----
// Warning 2319: (17-134): This declaration shadows a builtin symbol.
// Warning 2018: (17-134): Function state mutability can be restricted to view
