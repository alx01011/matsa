public class RacyDoubleMemberLoopTest {
    private double x = 0.0;

    protected void run(int i) {
        x = x + 1.0;
    }

    public static void main(String[] args) throws InterruptedException {
        RacyDoubleMemberLoopTest test = new RacyDoubleMemberLoopTest();

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
