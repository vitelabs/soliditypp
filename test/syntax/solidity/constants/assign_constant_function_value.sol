contract C {
    function () pure returns (uint) x;
    uint constant y = x();
}
// ----
// TypeError 8349: (74-77): Initial value for constant variable has to be compile-time constant.
