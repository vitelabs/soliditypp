contract test {
	function a(uint a, uint b, uint c) returns (uint r) { r = a * 100 + b * 10 + c * 1; }
	function b() returns (uint r) { r = a({a: 1, b: 2, c: 3, }); }
}
// ----
// ParserError 2074: (159-160): Unexpected trailing comma.
