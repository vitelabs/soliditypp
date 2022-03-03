contract C {
	function f(address a) external view returns (bool success) {
		(success,) = a.call{gas: 42}("");
	}
	function h() external payable {}
	function i() external view {
		this.h{gas: 42}();
	}
}
// ----
// TypeError 100201: (90-105): Cannot set option "gas" in Solidity++. Valid options are "token" and "value".
// TypeError 100201: (180-195): Cannot set option "gas" in Solidity++. Valid options are "token" and "value".
