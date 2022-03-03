contract test {
	function f() pure public {
		bytes32 x = sha3("");
		x;
	}
	function g() public {
		suicide(payable("vite_55c92b33cfcb3726405844155ad76a6dccf25f240ad2aaf786"));
	}
}
// ----
// TypeError 3557: (58-62): "sha3" has been deprecated in favour of "keccak256".
// TypeError 8050: (101-108): "suicide" has been deprecated in favour of "selfdestruct".
