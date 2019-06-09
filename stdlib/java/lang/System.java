package java.lang;

class System {
    static native void halt0(int status);

    public static void exit(int status) {
        halt0(status);
    }
}
