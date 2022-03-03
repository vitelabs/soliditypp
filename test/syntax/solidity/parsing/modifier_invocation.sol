contract c {
    modifier mod1(address a) { if (msg.sender == a) _; }
    modifier mod2 { if (msg.sender == "vite_0ab5b9c50b27647538cbb7918980c1dd4c281b1a53b2a7c4a1") _; }
    function f() public mod1("vite_591e456aa84fccd65e4c916c258ef3b80fadd94eab6f37518c") mod2 { }
}
// ----
