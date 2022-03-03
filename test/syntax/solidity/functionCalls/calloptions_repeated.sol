contract D {
    constructor () payable {}
}
contract C {
    function foo(int a) payable external {
        this.foo{value:2, token: "tti_5649544520544f4b454e6e40"}{value: 3};
        (this.foo{value:2, token: "tti_5649544520544f4b454e6e40"}){value: 3};
        this.foo{value:2, token: "tti_5649544520544f4b454e6e40"}{value:6};
        this.foo{token: "tti_5649544520544f4b454e6e40", value: 5}{value:2, token: "tti_5649544520544f4b454e6e40"};
        new D{token: "tti_5649544520544f4b454e6e40"}{token: "tti_5649544520544f4b454e6e40"}();
    }
}
// ====
// EVMVersion: >=constantinople
// ----
// TypeError 1645: (109-175): Function call options have already been set, you have to combine them into a single {...}-option.
// TypeError 1645: (185-253): Function call options have already been set, you have to combine them into a single {...}-option.
// TypeError 1645: (263-328): Function call options have already been set, you have to combine them into a single {...}-option.
// TypeError 1645: (338-443): Function call options have already been set, you have to combine them into a single {...}-option.
// TypeError 1645: (453-536): Function call options have already been set, you have to combine them into a single {...}-option.
