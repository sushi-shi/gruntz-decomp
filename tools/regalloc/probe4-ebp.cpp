extern int compute(int); extern void sink(int);
int probe4() {
    int a=compute(1); int b=compute(2); int c=compute(3); int d=compute(4);
    sink(0);
    return a*7 + b*11 + c*13 + d*17;
}
