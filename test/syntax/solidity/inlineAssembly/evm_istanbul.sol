contract C {
    function f() view external returns (uint id) {
        assembly {
            id := chainid()
        }
    }
    function g() view external returns (uint sb) {
        assembly {
            sb := selfbalance()
        }
    }
}
// ====
// EVMVersion: >=istanbul
// ----
