contract C {
    function f() public payable {
		abi.encode(this.f{value: 2});
		abi.encode(this.f{"tti_5649544520544f4b454e6e40"});
		abi.encode(this.f{value: 2,"tti_5649544520544f4b454e6e40"});
    }
}
// ----
// TypeError 2056: (60-76): This type cannot be encoded.
// TypeError 2056: (92-106): This type cannot be encoded.
// TypeError 2056: (122-146): This type cannot be encoded.
