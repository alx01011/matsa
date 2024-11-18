public class NonRacyByteArrayLoopTest {
    private byte[] x = new byte[2];

    protected synchronized void run(int i) {
        x[0] = (byte) (x[0] + 1);
    }

    public static void main(String[] args) throws InterruptedException {
        NonRacyByteArrayLoopTest test = new NonRacyByteArrayLoopTest();

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
