contract test {
	function a(uint a, uint b, uint c) returns (uint r) { r = a * 100 + b * 10 + c * 1; }
	function b() returns (uint r) { r = a({: 1, : 2, : 3}); }
}
// ----
// ParserError 2314: (143-144): Expected identifier but got ':'
