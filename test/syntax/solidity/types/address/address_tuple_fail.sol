contract C {
    function f() public view returns (address payable a, address b) {
        (address c, address payable d) = (address(this), payable(0));
        (a,b) = (c,d);
    }
}
// ----
// TypeError 7407: (169-174): Type tuple(address,address payable) is not implicitly convertible to expected type tuple(address payable,address).
