contract C {
    function f() public payable {
		abi.encode(this.f{value: 2});
		abi.encode(this.f{token: "tti_5649544520544f4b454e6e40"});
		abi.encode(this.f{value: 2, token: "tti_5649544520544f4b454e6e40"});
    }
}
// ----
// TypeError 2056: (60-76): This type cannot be encoded.
// TypeError 2056: (92-137): This type cannot be encoded.
// TypeError 2056: (153-208): This type cannot be encoded.
