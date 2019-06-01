class InitializeClass {
    public static int count = 1;

    static {
        count += 46;
    }

    public static int main(String[] args) {
        count++;
        return count;
    }
}
