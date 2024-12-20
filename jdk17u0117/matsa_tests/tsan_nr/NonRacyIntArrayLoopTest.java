public class NonRacyIntArrayLoopTest {
    private int[] x = new int[1];

    protected synchronized void run(int i) {
        x[0] = x[0] + 1;
    }

    public static void main(String[] args) throws InterruptedException {
        NonRacyIntArrayLoopTest test = new NonRacyIntArrayLoopTest();

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
