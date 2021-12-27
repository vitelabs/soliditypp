contract D {}
contract C {
    function foo() pure internal {
		new D{salt:"abc", value:3, gas: 4};
		new D{slt:5, value:3};
		new D{val:5};
		new D{salt:"xyz", salt:"aaf"};
		new D{value:3, value:4};
		new D{random:5+5};
		new D{what:2130+5};
		new D{gas: 2};
    }
}
// ====
// EVMVersion: >=constantinople
// ----
// TypeError 100201: (64-98): Cannot set option "salt" in Solidity++. Valid options are "token" and "value".
// TypeError 7006: (64-98): Cannot set option "value", since the constructor of contract D is not payable.
// TypeError 100201: (64-98): Cannot set option "gas" in Solidity++. Valid options are "token" and "value".
// TypeError 9318: (102-123): Unknown call option "slt". Valid options are "token" and "value".
// TypeError 7006: (102-123): Cannot set option "value", since the constructor of contract D is not payable.
// TypeError 9318: (127-139): Unknown call option "val". Valid options are "token" and "value".
// TypeError 100201: (143-172): Cannot set option "salt" in Solidity++. Valid options are "token" and "value".
// TypeError 100201: (143-172): Cannot set option "salt" in Solidity++. Valid options are "token" and "value".
// TypeError 7006: (176-199): Cannot set option "value", since the constructor of contract D is not payable.
// TypeError 7006: (176-199): Cannot set option "value", since the constructor of contract D is not payable.
// TypeError 9318: (203-220): Unknown call option "random". Valid options are "token" and "value".
// TypeError 9318: (224-242): Unknown call option "what". Valid options are "token" and "value".
// TypeError 100201: (246-259): Cannot set option "gas" in Solidity++. Valid options are "token" and "value".
