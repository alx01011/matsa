public class nr_volatile {
    public static volatile int x = 3;

    public static void main(String... args) {
        Thread t1 = new Thread(() -> {
            increment();
        });

        Thread t2 = new Thread(() -> {
            increment();
        });

        t1.start();
        t2.start();

        try {
            t1.join();
            t2.join();
        } catch (InterruptedException e) {
            throw new RuntimeException(e);
        }

        x = 3;
    }

    public static void increment() {
        x++;
    }
}