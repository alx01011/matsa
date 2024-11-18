public class RacyCharArrayLoopTest {
    private char[] x = new char[2];

    protected void run(int i) {
        x[0] = (char) (x[0] + 1);
    }

    public static void main(String[] args) throws InterruptedException {
        RacyCharArrayLoopTest test = new RacyCharArrayLoopTest();

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
