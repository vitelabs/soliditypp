==== Source: B.sol ====
contract X {}
==== Source: b ====
import * as msg from "B.sol";
contract C {
}
// ----
// Warning 2319: (b:0-29): This declaration shadows a builtin symbol.
