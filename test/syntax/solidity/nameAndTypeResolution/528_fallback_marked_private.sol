contract C {
    fallback () private { }
}
// ----
// TypeError 1159: (17-40): Fallback function must be defined as "external".
