public class nr_same_lock_object {
    public static int x = 3;
    public static Object lock = new Object();

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

        System.out.println("OK");

    }

    public static void increment() {
        synchronized (lock) {
            x++;
        }
    }
}