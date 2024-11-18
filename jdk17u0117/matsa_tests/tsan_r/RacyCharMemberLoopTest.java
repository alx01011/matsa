public class RacyCharMemberLoopTest {
    private char x = 0;

    protected void run(int i) {
        x = (char) (x + 1);
    }

    public static void main(String[] args) throws InterruptedException {
        RacyCharMemberLoopTest test = new RacyCharMemberLoopTest();

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
