contract C {
    function f() public {
        (msg.sender).transfer("tti_5649544520544f4b454e6e40", 10);
    }
}
// ----
// TypeError 9862: (47-68): "send" and "transfer" are only available for objects of type "address payable", not "address".
