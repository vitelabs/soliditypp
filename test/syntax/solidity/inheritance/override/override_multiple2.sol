abstract contract A {
	int public testvar;
	function foo() internal virtual returns (uint256);
	function test(uint8 _a) internal virtual returns (uint256);
}
abstract contract B {
	function foo() internal virtual returns (uint256);
}

abstract contract C is A {
}
abstract contract D is A, B, C {
	function foo() internal override(A, B) virtual returns (uint256);
}
// ----
