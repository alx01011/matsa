public class NonRacyStringArrayLoopTest {
    private String[] x = new String[] {"a", "b"};

    protected synchronized void run(int i) {
        char c = x[0].charAt(0);
        x[0] = Character.toString(c);
    }

    public static void main(String[] args) throws InterruptedException {
        NonRacyStringArrayLoopTest test = new NonRacyStringArrayLoopTest();

        final Thread t1 = new Thread(() -> {
            for (int i = 0; i < 50000; i++) {
                test.run(i);
            }
        });
        final Thread t2 = new Thread(() -> {
            for (int i = 0; i < 50000; i++) {
                test.run(i);
            }
        });

        t1.start();
        t2.start();

        t1.join();
        t2.join();
    }
}
