class CallStaticMethodNoArg {
    public static int return42() {
        return 42;
    }

    public static int main(String[] args) {
        int x = return42();
        return x;
    }
}
