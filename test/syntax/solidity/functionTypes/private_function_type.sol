contract C {
    function f() public {
        function(uint) private returns (uint) x;
    }
}
// ----
// TypeError 6012: (47-86): Invalid visibility, can only be "external" or "internal".
