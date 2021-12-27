contract Test
{
  function uint256_to_uint256(uint256 x) internal pure returns (uint256) { return x; }
  function uint256_to_string(uint256 x) internal pure returns (string memory) { return x == 0 ? "a" : "b"; }
  function string_to_uint256(string memory x) internal pure returns (uint256) { return bytes(x).length; }
  function string_to_string(string memory x) internal pure returns (string memory) { return x; }

  function uint256_uint256_to_uint256(uint256 x, uint256 y) internal pure returns (uint256) { return x + y; }
  function uint256_uint256_to_string(uint256 x, uint256 y) internal pure returns (string memory) { return x == y ? "a" : "b"; }
  function string_uint256_to_string(string memory x, uint256 y) internal pure returns (string memory) { return y == 0 ? "a" : x; }
  function string_string_to_string(string memory x, string memory y) internal pure returns (string memory) { return bytes(x).length == 0 ? y : x; }
  function uint256_string_to_string(uint256 x, string memory y) internal pure returns (string memory) { return x == 0 ? "a" : y; }

  function tests() internal pure
  {
    function (uint256) internal pure returns (uint256) var_uint256_to_uint256 = uint256_to_uint256;
    function (uint256) internal pure returns (string memory) var_uint256_to_string = uint256_to_string;
    function (string memory) internal pure returns (uint256) var_string_to_uint256 = string_to_uint256;
    function (string memory) internal pure returns (string memory) var_string_to_string = string_to_string;

    function (uint256, uint256) internal pure returns (uint256) var_uint256_uint256_to_uint256 = uint256_uint256_to_uint256;
    function (uint256, uint256) internal pure returns (string memory) var_uint256_uint256_to_string = uint256_uint256_to_string;
    function (string memory, uint256) internal pure returns (string memory) var_string_uint256_to_string = string_uint256_to_string;
    function (string memory, string memory) internal pure returns (string memory) var_string_string_to_string = string_string_to_string;
    function (uint256, string memory) internal pure returns (string memory) var_uint256_string_to_string = uint256_string_to_string;

    // Avoid unused variable warnings:
    var_uint256_to_uint256(1);
    var_uint256_to_string(2);
    var_string_to_uint256("a");
    var_string_to_string("b");
    var_uint256_uint256_to_uint256(3, 4);
    var_uint256_uint256_to_string(5, 6);
    var_string_uint256_to_string("c", 7);
    var_string_string_to_string("d", "e");
    var_uint256_string_to_string(8, "f");
  }
}
// ----
