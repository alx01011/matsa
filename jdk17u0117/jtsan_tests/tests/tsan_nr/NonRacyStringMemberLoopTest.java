public class NonRacyStringMemberLoopTest {
    private String x = "a";

    protected synchronized void run(int i) {
        char c = x.charAt(0);
        x = Character.toString(c);
    }

    public static void main(String[] args) throws InterruptedException {
        NonRacyStringMemberLoopTest test = new NonRacyStringMemberLoopTest();

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
