extern int compute(int);
extern void sink(int);
int probe2() {
    int a = compute(1);
    int b = compute(2);
    int c = compute(3);
    sink(0);
    return c * 7 + b * 11 + a * 13;   // use order reversed
}
