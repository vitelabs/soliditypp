contract test {
    function f(uint[] memory constant a) public { }
}
// ----
// DeclarationError 1788: (31-55): The "constant" keyword can only be used for state variables or variables at file level.
// DeclarationError 9259: (31-55): Constants of non-value type not yet implemented.
