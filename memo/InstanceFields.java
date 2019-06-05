class InstanceFields {
    private int x;

    public InstanceFields() {
        this.x = 1;
    }

    public int getX() {
        return this.x;
    }

    public void setX(int n) {
        this.x = n;
    }

    public static int main(String[] args) {
        InstanceFields ci = new InstanceFields();
        ci.setX(50);
        return ci.getX();
    }
}
