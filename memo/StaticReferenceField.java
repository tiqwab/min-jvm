class StaticReferenceField {
    public static StaticReferenceField srf = new StaticReferenceField();

    public int x;

    public StaticReferenceField() {
        this.x = 51;
    }

    public static int main(String[] args) {
        return srf.x;
    }
}
