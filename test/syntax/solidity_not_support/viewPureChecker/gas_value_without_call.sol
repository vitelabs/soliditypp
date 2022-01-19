contract C {
    function f() external payable {}
	function g(address a) external pure {
		a.call{value: 42};
		a.call{gas: 42};
		a.staticcall{gas: 42};
		a.delegatecall{gas: 42};
	}
	function h() external view {
		this.f{value: 42};
		this.f{gas: 42};
	}
}
// ----
// TypeError 100201: (112-127): Cannot set option "gas" in Solidity++. Valid options are "token" and "value".
// TypeError 100201: (131-152): Cannot set option "gas" in Solidity++. Valid options are "token" and "value".
// TypeError 100201: (156-179): Cannot set option "gas" in Solidity++. Valid options are "token" and "value".
// TypeError 100201: (237-252): Cannot set option "gas" in Solidity++. Valid options are "token" and "value".