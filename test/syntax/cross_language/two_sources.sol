==== Source: A ====
pragma solidity >=0.8.0;

contract A {
    function set(uint _item) external {}
}
==== Source: B ====
pragma soliditypp >=0.8.0;
import "A";

contract B {
    A a = A(address(0x00));
    function test() public {
        await a.set(123);
    }
}
// ----