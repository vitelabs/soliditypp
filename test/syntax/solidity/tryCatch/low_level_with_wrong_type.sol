contract C {
    function f() public returns (uint, uint) {
        try this.f() {

        } catch (uint) {
        }
    }
}
// ====
// EVMVersion: >=byzantium
// ----
// TypeError 6231: (94-118): Expected `catch (bytes memory ...) { ... }` or `catch { ... }`.
