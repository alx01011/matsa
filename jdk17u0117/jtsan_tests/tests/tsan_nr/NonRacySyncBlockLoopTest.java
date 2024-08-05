public class NonRacySyncBlockLoopTest {
    private int x = 0;

    protected void run(int i) {
        synchronized (this) {
            x = x + 1;
          }
    }

    public static void main(String[] args) throws InterruptedException {
        NonRacySyncBlockLoopTest test = new NonRacySyncBlockLoopTest();

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
