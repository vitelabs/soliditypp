contract C
{
	function f(bytes memory data) public pure {
		suicide;
	}
}
// ----
// TypeError 8050: (60-67): "suicide" has been deprecated in favour of "selfdestruct".
