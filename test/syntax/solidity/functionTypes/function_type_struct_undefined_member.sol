library L
{
	struct Nested
	{
		Non y;
	}
	function f(function(Nested memory) external) external pure {}
}
// ----
// DeclarationError 7920: (32-35): Identifier not found or not unique.
