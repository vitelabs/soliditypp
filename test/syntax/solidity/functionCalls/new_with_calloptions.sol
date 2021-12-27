contract D { constructor() payable {} }
contract C {
    function foo() pure internal {
		new D{token:"tti_5649544520544f4b454e6e40", value:3};
		new D{value:5+5};
    }
}
// ====
// EVMVersion: >=constantinople
// ----
