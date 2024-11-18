public class NonRacyVolatileLoopTest {
    private volatile byte b;
    private volatile String s;
    int data1;
    int data2;

    protected void syncSetup() {
        b = 1;
        s = "a";
    }


    protected void run(int i) {
        data1 = 42;
        b = 2;
    
        data2 = 43;
        s = "b";
    }

    protected void run2(int i) {
        while (b != 2) {
          ;
        }
        int x = data1;
        while (!s.equals("b")) {
          ;
        }
        x = data2;
    }

    public static void main(String[] args) throws InterruptedException {
        NonRacyVolatileLoopTest test = new NonRacyVolatileLoopTest();
        test.syncSetup();

        final Thread t1 = new Thread(() -> {
            for (int i = 0; i < 50000; i++) {
                test.run(i);
            }
        });
        final Thread t2 = new Thread(() -> {
            for (int i = 0; i < 50000; i++) {
                test.run2(i);
            }
        });

        t1.start();
        t2.start();

        t1.join();
        t2.join();
    }    
}
