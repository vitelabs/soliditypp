contract C {
    function f() view public {
        payable(this).transfer(1);
    }
    function g() view public {
        require(payable(this).send(2));
    }
    function h() view public {
        selfdestruct(payable(this));
    }
    function i() view public {
        (bool success,) = address(this).delegatecall("");
        require(success);
    }
    function j() view public {
        (bool success,) = address(this).call("");
        require(success);
    }
    receive() payable external {
    }
}
// ----
// TypeError 8961: (52-77): Function declared as view, but this expression (potentially) modifies the state and thus requires non-payable (the default) or payable.
// TypeError 8961: (132-153): Function declared as view, but this expression (potentially) modifies the state and thus requires non-payable (the default) or payable.
// TypeError 8961: (201-228): Function declared as view, but this expression (potentially) modifies the state and thus requires non-payable (the default) or payable.
// TypeError 8961: (293-323): Function declared as view, but this expression (potentially) modifies the state and thus requires non-payable (the default) or payable.
// TypeError 8961: (414-436): Function declared as view, but this expression (potentially) modifies the state and thus requires non-payable (the default) or payable.
