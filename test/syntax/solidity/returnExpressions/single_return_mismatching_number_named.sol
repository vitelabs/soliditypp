contract C
{
    function f() public pure returns (uint a)
    {
        return;
    }
    function g() public pure returns (uint a)
    {
        return (1, 2);
    }
}
// ----
// TypeError 6777: (73-80): Return arguments required.
// TypeError 5132: (147-160): Different number of arguments in return statement than in returns declaration.
