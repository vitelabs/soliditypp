contract C {
    bool payable a;
    string payable b;
    int payable c;
    int256 payable d;
    uint payable e;
    uint256 payable f;
    bytes1 payable g;
    bytes payable h;
    bytes32 payable i;
    fixed payable j;
    fixed80x80 payable k;
    ufixed payable l;
    ufixed80x80 payable m;
}
// ----
// ParserError 9106: (22-29): State mutability can only be specified for address types.
// ParserError 9106: (44-51): State mutability can only be specified for address types.
// ParserError 9106: (63-70): State mutability can only be specified for address types.
// ParserError 9106: (85-92): State mutability can only be specified for address types.
// ParserError 9106: (105-112): State mutability can only be specified for address types.
// ParserError 9106: (128-135): State mutability can only be specified for address types.
// ParserError 9106: (150-157): State mutability can only be specified for address types.
// ParserError 9106: (171-178): State mutability can only be specified for address types.
// ParserError 9106: (194-201): State mutability can only be specified for address types.
// ParserError 9106: (215-222): State mutability can only be specified for address types.
// ParserError 9106: (241-248): State mutability can only be specified for address types.
// ParserError 9106: (263-270): State mutability can only be specified for address types.
// ParserError 9106: (290-297): State mutability can only be specified for address types.
