contract C {
    function f() view public {
        payable(this).transfer("tti_5649544520544f4b454e6e40", 1);
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
// TypeError 8961: (52-109): Function declared as view, but this expression (potentially) modifies the state and thus requires non-payable (the default) or payable.
// TypeError 8961: (156-183): Function declared as view, but this expression (potentially) modifies the state and thus requires non-payable (the default) or payable.
// TypeError 8961: (248-278): Function declared as view, but this expression (potentially) modifies the state and thus requires non-payable (the default) or payable.
// TypeError 8961: (369-391): Function declared as view, but this expression (potentially) modifies the state and thus requires non-payable (the default) or payable.
