public class NonRacyFloatArrayLoopTest {
    private float[] x = new float[1];

    protected synchronized void run(int i) {
        x[0] = x[0] + 1.0f;
    }

    public static void main(String[] args) throws InterruptedException {
        NonRacyFloatArrayLoopTest test = new NonRacyFloatArrayLoopTest();

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
