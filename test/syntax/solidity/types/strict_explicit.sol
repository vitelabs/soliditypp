contract B{}
contract C
{
    function f() public pure {

        uint16 a = uint16(uint8(int8(-1)));
        a;

        int8 b = -1;
        b;
        uint16 c = uint16(uint8(b));
        c;

        int8 d = int8(int16(uint16(type(uint16).max)));
        d;

        uint16 e = type(uint16).max;
        e;
        int8 g = int8(uint8(e));
        g;

        address h = address(uint160(uint(type(uint).max)));
        h;

        uint i = uint(uint160(address(0)));
        i;

        uint j = type(uint).max;
        j;
        address k = address(uint160(j));
        k;

        int80 l = int80(uint80(bytes10("h")));
        l;
        bytes10 m = bytes10(uint80(int80(-1)));
        m;

        B n = B(address(uint160(uint(int(100)))));
        n;

        uint8 o = 1;
        int16 p = int16(o);
    }
}
// ----
// TypeError 9640: (376-414): Explicit type conversion not allowed from "uint160" to "address".
// TypeError 9640: (450-469): Explicit type conversion not allowed from "address" to "uint160".
// TypeError 9640: (548-567): Explicit type conversion not allowed from "uint160" to "address".
// TypeError 9640: (715-747): Explicit type conversion not allowed from "uint160" to "address".
// TypeError 9640: (801-809): Explicit type conversion not allowed from "uint8" to "int16".
