public class NonRacyFloatMemberLoopTest {
    private float x = 0.0f;

    protected synchronized void run(int i) {
        x = x + 1.0f;
    }

    public static void main(String[] args) throws InterruptedException {
        NonRacyFloatMemberLoopTest test = new NonRacyFloatMemberLoopTest();

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
