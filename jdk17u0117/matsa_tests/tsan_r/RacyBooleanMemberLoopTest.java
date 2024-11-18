public class RacyBooleanMemberLoopTest {
    private boolean x = true;

    protected void run(int i) {
        x = !x;
    }

    public static void main(String[] args) throws InterruptedException {
        RacyBooleanMemberLoopTest test = new RacyBooleanMemberLoopTest();

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
