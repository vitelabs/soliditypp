contract C {
	uint[] r;
    function f() internal view returns (uint[] storage s) {
        assembly { pop(s.slot) }
        s = r;
    }
}
// ----
// TypeError 3464: (107-113): This variable is of storage pointer type and can be accessed without prior assignment, which would lead to undefined behaviour.
