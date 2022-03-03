contract C {
    struct S { bool f; }
    S s;
    function f(uint256 a) internal pure {
        S storage c;
        assembly {
            switch a
            case 0 { c.slot := s.slot }
        }
        c;
    }
    function g(bool flag) internal pure {
        S storage c;
        assembly {
            switch flag
            case 0 { c.slot := s.slot }
            case 1 { c.slot := s.slot }
        }
        c;
    }
    function h(uint256 a) internal pure {
        S storage c;
        assembly {
            switch a
            case 0 { c.slot := s.slot }
            default { return(0,0) }
        }
        c;
    }
}
// ----
// TypeError 3464: (208-209): This variable is of storage pointer type and can be accessed without prior assignment, which would lead to undefined behaviour.
// TypeError 3464: (421-422): This variable is of storage pointer type and can be accessed without prior assignment, which would lead to undefined behaviour.
