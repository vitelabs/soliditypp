contract c {
    enum validEnum { Value1, Value2, Value3, Value4 }
    constructor() {
        a = validEnum.Value3;
    }
    validEnum a;
}
