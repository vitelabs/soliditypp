contract D {
    constructor () payable {}
}
contract C {
    function foo(int a) payable external {
        this.foo{token:"tti_5649544520544f4b454e6e40", token:"tti_5649544520544f4b454e6e40"};
        this.foo{value:2, value: 5};
        this.foo{token:"tti_5649544520544f4b454e6e40", value: 5, token:"tti_5649544520544f4b454e6e40", value:3};
        new D{value:2, value:2}();
    }
}
// ====
// EVMVersion: >=constantinople
// ----
// TypeError 9886: (109-193): Duplicate option "token".
// TypeError 9886: (203-230): Duplicate option "value".
// TypeError 9886: (240-343): Duplicate option "token".
// TypeError 9886: (240-343): Duplicate option "value".
// TypeError 9886: (353-376): Duplicate option "value".
