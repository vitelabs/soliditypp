contract B {
    uint immutable x;

    constructor() {
        x = xInit();
    }

    function xInit() internal virtual returns(uint) {
        return 3;
    }
}

contract C is B {
    function xInit() internal override returns(uint) {
        return x;
    }
}
// ----
// TypeError 7733: (253-254): Immutable variables cannot be read during contract creation time, which means they cannot be read in the constructor or any function or modifier called from it.
