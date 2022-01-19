contract C {
    function f() external view {}
	function test(address a) external view returns (bool status) {
		// This used to incorrectly raise an error about violating the view mutability.
		(status,) = a.staticcall{gas: 42}("");
		this.f{gas: 42}();
	}
}
// ----
// TypeError 100201: (207-228): Cannot set option "gas" in Solidity++. Valid options are "token" and "value".
// TypeError 100201: (236-251): Cannot set option "gas" in Solidity++. Valid options are "token" and "value".
