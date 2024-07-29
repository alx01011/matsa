public class NonRacySyncBlockExceptionLoopTest {
    private static class LoopException extends Exception {}
    private int x = 0;

    protected void run(int i) {
        try {
          synchronized (this) {
            x = x + 1;
            throw new LoopException();
          }
        } catch (LoopException e) {
          // expected
        }
      }

    public static void main(String[] args) throws InterruptedException {
        NonRacySyncBlockExceptionLoopTest test = new NonRacySyncBlockExceptionLoopTest();

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
