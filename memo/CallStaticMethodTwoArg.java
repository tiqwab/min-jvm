class CallStaticMethodTwoArg {
    public static int sub(int x, int y) {
        return x - y;
    }

    public static int main(String[] args) {
        int x = sub(45, 1);
        return x;
    }
}
