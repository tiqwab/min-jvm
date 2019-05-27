class CallStaticMethodOneArg {
    public static int plusOne(int x) {
        return x + 1;
    }

    public static int main(String[] args) {
        int x = plusOne(42);
        return x;
    }
}
